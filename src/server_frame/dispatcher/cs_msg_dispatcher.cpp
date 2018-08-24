//
// Created by owt50 on 2016/9/27.
//

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <WinSock2.h>
#endif

#include <log/log_wrapper.h>

#include <atframe/atapp.h>
#include <config/atframe_service_types.h>
#include <libatgw_server_protocol.h>
#include <proto_base.h>

#include <protocol/pbdesc/com.protocol.pb.h>
#include <protocol/pbdesc/svr.const.err.pb.h>


#include <logic/action/task_action_player_logout.h>
#include <logic/session_manager.h>

#include "cs_msg_dispatcher.h"
#include "task_manager.h"


cs_msg_dispatcher::cs_msg_dispatcher() {}
cs_msg_dispatcher::~cs_msg_dispatcher() {}

int32_t cs_msg_dispatcher::init() { return 0; }

uint64_t cs_msg_dispatcher::pick_msg_task_id(msg_raw_t &raw_msg) {
    // cs msg not allow resume task
    return 0;
}

cs_msg_dispatcher::msg_type_t cs_msg_dispatcher::pick_msg_type_id(msg_raw_t &raw_msg) {
    hello::CSMsg *real_msg = get_protobuf_msg<hello::CSMsg>(raw_msg);
    if (NULL == real_msg) {
        return 0;
    }

    return static_cast<msg_type_t>(real_msg->body().body_oneof_case());
}

int32_t cs_msg_dispatcher::dispatch(const atbus::protocol::msg &msg, const void *buffer, size_t len) {
    if (::atframe::component::service_type::EN_ATST_GATEWAY != msg.head.type) {
        WLOGERROR("message type %d invalid", msg.head.type);
        return hello::err::EN_SYS_PARAM;
    }

    if (NULL == msg.body.forward || 0 == msg.head.src_bus_id) {
        WLOGERROR("receive a message from unknown source");
        return hello::err::EN_SYS_PARAM;
    }

    ::atframe::gw::ss_msg req_msg;
    msgpack::unpacked result;
    msgpack::unpack(result, reinterpret_cast<const char *>(buffer), len);
    msgpack::object obj = result.get();
    if (obj.is_nil()) {
        return 0;
    }
    obj.convert(req_msg);

    int ret = hello::err::EN_SUCCESS;
    switch (req_msg.head.cmd) {
    case ATFRAME_GW_CMD_POST: {
        hello::CSMsg cs_msg;
        session::key_t session_key;
        session_key.bus_id = msg.body.forward->from;
        session_key.session_id = req_msg.head.session_id;

        if (!session_manager::me()->find(session_key)) {
            WLOGERROR("session [0x%llx, 0x%llx] not found, try to kickoff", static_cast<unsigned long long>(session_key.bus_id),
                      static_cast<unsigned long long>(session_key.session_id));
            ret = hello::err::EN_SYS_NOTFOUND;

            send_kickoff(session_key.bus_id, session_key.session_id, hello::EN_CRT_SESSION_NOT_FOUND);
            break;
        }

        start_data_t start_data;
        start_data.private_data = NULL;
        ret = unpack_protobuf_msg(cs_msg, start_data.message, req_msg.body.post->content.ptr, req_msg.body.post->content.size);
        if (ret != 0) {
            WLOGERROR("%s unpack received message from 0x%llx, session id:0x%llx failed, res: %d", name(), static_cast<unsigned long long>(session_key.bus_id),
                      static_cast<unsigned long long>(session_key.session_id), ret);
            return ret;
        }

        cs_msg.mutable_head()->set_session_bus_id(session_key.bus_id);
        cs_msg.mutable_head()->set_session_id(session_key.session_id);

        ret = on_recv_msg(start_data.message, start_data.private_data);
        if (ret < 0) {
            WLOGERROR("%s on receive message callback from to 0x%llx, session id:0x%llx failed, res: %d", name(),
                      static_cast<unsigned long long>(session_key.bus_id), static_cast<unsigned long long>(session_key.session_id), ret);
        }
        break;
    }
    case ATFRAME_GW_CMD_SESSION_ADD: {
        session::key_t session_key;
        session_key.bus_id = msg.body.forward->from;
        session_key.session_id = req_msg.head.session_id;

        WLOGINFO("create new session [0x%llx, 0x%llx], address: %s:%d", static_cast<unsigned long long>(session_key.bus_id),
                 static_cast<unsigned long long>(session_key.session_id), req_msg.body.session->client_ip.c_str(), req_msg.body.session->client_port);

        session_manager::sess_ptr_t sess = session_manager::me()->create(session_key);
        if (!sess) {
            WLOGERROR("malloc failed");
            ret = hello::err::EN_SYS_MALLOC;
            send_kickoff(session_key.bus_id, session_key.session_id, ::atframe::gateway::close_reason_t::EN_CRT_SERVER_BUSY);
            break;
        }

        break;
    }
    case ATFRAME_GW_CMD_SESSION_REMOVE: {
        session::key_t session_key;
        session_key.bus_id = msg.body.forward->from;
        session_key.session_id = req_msg.head.session_id;

        WLOGINFO("remove session [0x%llx, 0x%llx]", static_cast<unsigned long long>(session_key.bus_id),
                 static_cast<unsigned long long>(session_key.session_id));

        // logout task
        task_manager::id_t logout_task_id = 0;
        task_action_player_logout::ctor_param_t task_param;
        task_param.atgateway_session_id = session_key.session_id;
        task_param.atgateway_bus_id = session_key.bus_id;

        ret = task_manager::me()->create_task<task_action_player_logout>(logout_task_id, COPP_MACRO_STD_MOVE(task_param));
        if (0 == ret) {
            start_data_t start_data;
            start_data.private_data = NULL;
            start_data.message.msg_type = 0;
            start_data.message.msg_addr = NULL;

            ret = task_manager::me()->start_task(logout_task_id, start_data);
            if (0 != ret) {
                WLOGERROR("run logout task failed, res: %d", ret);
                session_manager::me()->remove(session_key);
            }
        } else {
            WLOGERROR("create logout task failed, res: %d", ret);
            session_manager::me()->remove(session_key);
        }
        break;
    }
    default:
        WLOGERROR("receive a unsupport atgateway message of invalid cmd:%d", static_cast<int>(req_msg.head.cmd));
        break;
    }

    return ret;
}

int32_t cs_msg_dispatcher::send_kickoff(uint64_t bus_id, uint64_t session_id, int32_t reason) {
    atapp::app *owner = get_app();
    if (NULL == owner) {
        WLOGERROR("not in a atapp");
        return hello::err::EN_SYS_INIT;
    }

    ::atframe::gw::ss_msg msg;
    msg.init(ATFRAME_GW_CMD_SESSION_KICKOFF, session_id);
    msg.head.error_code = reason;

    std::stringstream ss;
    msgpack::pack(ss, msg);
    std::string packed_buffer;
    ss.str().swap(packed_buffer);

    return owner->get_bus_node()->send_data(bus_id, 0, packed_buffer.data(), packed_buffer.size());
}

int32_t cs_msg_dispatcher::send_data(uint64_t bus_id, uint64_t session_id, const void *buffer, size_t len) {
    atapp::app *owner = get_app();
    if (NULL == owner) {
        WLOGERROR("not in a atapp");
        return hello::err::EN_SYS_INIT;
    }

    ::atframe::gw::ss_msg msg;
    msg.init(ATFRAME_GW_CMD_POST, session_id);
    msg.body.make_post(buffer, len);
    if (NULL == msg.body.post) {
        if (0 == session_id) {
            WLOGERROR("broadcast data to atgateway 0x%llx failed when malloc post", static_cast<unsigned long long>(bus_id));
        } else {
            WLOGERROR("send data to session [0x%llx, 0x%llx] failed when malloc post", static_cast<unsigned long long>(bus_id),
                      static_cast<unsigned long long>(session_id));
        }
        return hello::err::EN_SYS_MALLOC;
    }

    std::stringstream ss;
    msgpack::pack(ss, msg);
    std::string packed_buffer;
    ss.str().swap(packed_buffer);

    int ret = owner->get_bus_node()->send_data(bus_id, ::atframe::component::service_type::EN_ATST_GATEWAY, packed_buffer.data(), packed_buffer.size());
    if (ret < 0) {
        if (0 == session_id) {
            WLOGERROR("broadcast data to atgateway 0x%llx failed, res: %d", static_cast<unsigned long long>(bus_id), ret);
        } else {
            WLOGERROR("send data to session [0x%llx, 0x%llx] failed, res: %d", static_cast<unsigned long long>(bus_id),
                      static_cast<unsigned long long>(session_id), ret);
        }
    }

    return ret;
}

int32_t cs_msg_dispatcher::broadcast_data(uint64_t bus_id, const void *buffer, size_t len) { return send_data(bus_id, 0, buffer, len); }

int32_t cs_msg_dispatcher::broadcast_data(uint64_t bus_id, const std::vector<uint64_t> &session_ids, const void *buffer, size_t len) {
    atapp::app *owner = get_app();
    if (NULL == owner) {
        WLOGERROR("not in a atapp");
        return hello::err::EN_SYS_INIT;
    }

    ::atframe::gw::ss_msg msg;
    msg.init(ATFRAME_GW_CMD_POST, 0);
    msg.body.make_post(buffer, len);
    if (NULL == msg.body.post) {
        WLOGERROR("broadcast data to atgateway 0x%llx failed when malloc post", static_cast<unsigned long long>(bus_id));
        return hello::err::EN_SYS_MALLOC;
    }
    msg.body.post->session_ids.assign(session_ids.begin(), session_ids.end());

    std::stringstream ss;
    msgpack::pack(ss, msg);
    std::string packed_buffer;
    ss.str().swap(packed_buffer);

    int ret = owner->get_bus_node()->send_data(bus_id, ::atframe::component::service_type::EN_ATST_GATEWAY, packed_buffer.data(), packed_buffer.size());
    if (ret < 0) {
        WLOGERROR("broadcast data to atgateway 0x%llx failed, res: %d", static_cast<unsigned long long>(bus_id), ret);
    }

    return ret;
}
