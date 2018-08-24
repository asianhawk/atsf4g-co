//
// Created by owt50 on 2016/9/26.
//

#include <log/log_wrapper.h>
#include <logic/player_manager.h>
#include <time/time_utility.h>


#include <dispatcher/ss_msg_dispatcher.h>

#include <config/logic_config.h>

#include <router/router_manager_base.h>
#include <router/router_manager_set.h>
#include <router/router_object_base.h>

#include <rpc/router/router_object_base.h>

#include "task_action_ss_req_base.h"

task_action_ss_req_base::task_action_ss_req_base(dispatcher_start_data_t COPP_MACRO_RV_REF start_param) {
    msg_type *ss_msg = ss_msg_dispatcher::me()->get_protobuf_msg<msg_type>(start_param.message);
    if (NULL != ss_msg) {
        get_request().Swap(ss_msg);

        set_player_id(get_request().head().player_user_id());
    }
}

task_action_ss_req_base::~task_action_ss_req_base() {}

int task_action_ss_req_base::hook_run() {
    // 路由对象系统支持
    if (get_request().head().has_router()) {
        const hello::SSRouterHead &router = get_request().head().router();

        do {
            // find router manager in router set
            router_manager_base *mgr = router_manager_set::me()->get_manager(router.object_type_id());
            if (NULL == mgr) {
                WLOGERROR("router manager %u not found", router.object_type_id());
                return hello::err::EN_ROUTER_TYPE_INVALID;
            }

            router_manager_base::key_t key(router.object_type_id(), router.object_inst_id());
            std::shared_ptr<router_object_base> obj = mgr->get_base_cache(key);

            // 如果正在迁移，追加到pending队列，本task直接退出
            if (obj && obj->check_flag(router_object_base::flag_t::EN_ROFT_TRANSFERING)) {
                obj->get_transfer_pending_list().push_back(hello::SSMsg());
                obj->get_transfer_pending_list().back().Swap(&get_request());

                return hello::err::EN_SUCCESS;
            }

            // 如果和本地的路由缓存匹配则break直接开始消息处理
            if (obj && (logic_config::me()->get_self_bus_id() == obj->get_router_server_id() || 0 == obj->get_router_server_id())) {
                break;
            }

            // 如果本地版本号低于来源服务器，刷新一次路由表
            if (!obj || obj->get_router_version() < router.router_version()) {
                mgr->mutable_object(obj, key, NULL);
            }

            // mutable路由对象，检查是否成功
            if (!obj) {
                WLOGERROR("router object key=%u:0x%llx not found", key.type_id, key.object_id_ull());
                return hello::err::EN_ROUTER_NOT_FOUND;
            }

            // 如果本地路由版本号大于来源，通知来源更新路由表
            if (obj->get_router_version() > router.router_version()) {
                hello::SSRouterUpdateSync sync_msg;
                hello::SSRouterHead *router_head = sync_msg.mutable_object();
                if (NULL != router_head) {
                    router_head->set_router_src_bus_id(obj->get_router_server_id());
                    router_head->set_router_version(obj->get_router_version());
                    router_head->set_object_type_id(key.type_id);
                    router_head->set_object_inst_id(key.object_id);
                }

                // 只通知直接来源
                rpc::router::robj::send_update_sync(get_request_bus_id(), sync_msg);
            }

            // 如果和本地的路由缓存匹配则break直接开始消息处理
            if (logic_config::me()->get_self_bus_id() == obj->get_router_server_id() || 0 == obj->get_router_server_id()) {
                break;
            }


            // 路由消息转发
            int32_t res = 0;
            if (0 != obj->get_router_server_id()) {
                int32_t res = mgr->send_msg(*obj, get_request());

                // 如果路由转发成功，需要禁用掉回包和通知事件，也不需要走逻辑处理了
                if (res < 0) {
                    WLOGERROR("try to transfer router object %u:x0%llx to 0x%llx failed, res: %d", key.type_id, key.object_id_ull(),
                              static_cast<unsigned long long>(obj->get_router_server_id()), res);
                }
            } else {
                // 如果路由对象不在任何节点上，返回错误
                res = hello::err::EN_ROUTER_NOT_IN_SERVER;
            }

            if (res >= 0) {
                disable_rsp_msg();
                disable_finish_evt();
            } else {
                // 失败则要回发转发失败
                set_rsp_code(res);
                obj->send_transfer_msg_failed(COPP_MACRO_STD_MOVE(get_request()));
            }

            return res;
        } while (false);
    }

    return base_type::hook_run();
}

uint64_t task_action_ss_req_base::get_request_bus_id() const {
    msg_cref_type msg = get_request();
    return msg.head().bus_id();
}

task_action_ss_req_base::msg_ref_type task_action_ss_req_base::add_rsp_msg(uint64_t dst_pd) {
    rsp_msgs_.push_back(msg_type());
    msg_ref_type msg = rsp_msgs_.back();

    msg.mutable_head()->set_error_code(get_rsp_code());
    dst_pd = 0 == dst_pd ? get_request_bus_id() : dst_pd;

    init_msg(msg, dst_pd, get_request());
    return msg;
}


int32_t task_action_ss_req_base::init_msg(msg_ref_type msg, uint64_t dst_pd) {
    msg.mutable_head()->set_bus_id(dst_pd);
    msg.mutable_head()->set_timestamp(util::time::time_utility::get_now());

    return 0;
}

int32_t task_action_ss_req_base::init_msg(msg_ref_type msg, uint64_t dst_pd, msg_cref_type req_msg) {
    msg.mutable_head()->CopyFrom(req_msg.head());
    init_msg(msg, dst_pd);

    // set task information
    if (0 != req_msg.head().src_task_id()) {
        msg.mutable_head()->set_dst_task_id(req_msg.head().src_task_id());
    } else {
        msg.mutable_head()->set_dst_task_id(0);
    }

    if (0 != req_msg.head().dst_task_id()) {
        msg.mutable_head()->set_src_task_id(req_msg.head().dst_task_id());
    } else {
        msg.mutable_head()->set_src_task_id(0);
    }

    return 0;
}

void task_action_ss_req_base::send_rsp_msg() {
    if (rsp_msgs_.empty()) {
        return;
    }

    for (std::list<msg_type>::iterator iter = rsp_msgs_.begin(); iter != rsp_msgs_.end(); ++iter) {
        if (0 == (*iter).head().bus_id()) {
            WLOGERROR("task %s [0x%llx] send message to unknown server", name(), get_task_id_llu());
            continue;
        }
        (*iter).mutable_head()->set_error_code(get_rsp_code());

        // send message using ss dispatcher
        int32_t res = ss_msg_dispatcher::me()->send_to_proc((*iter).head().bus_id(), *iter);
        if (res) {
            WLOGERROR("task %s [0x%llx] send message to server 0x%llx failed, res: %d", name(), get_task_id_llu(),
                      static_cast<unsigned long long>((*iter).head().bus_id()), res);
        }
    }

    rsp_msgs_.clear();
}