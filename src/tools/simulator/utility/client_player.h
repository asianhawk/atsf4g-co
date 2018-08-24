//
// Created by owt50 on 2016/10/11.
//

#ifndef ATFRAMEWORK_LIBSIMULATOR_UTILITY_CLIENT_PLAYER_H
#define ATFRAMEWORK_LIBSIMULATOR_UTILITY_CLIENT_PLAYER_H

#pragma once

#include <config/compiler_features.h>

#include <protocol/pbdesc/com.protocol.pb.h>

#include <libatgw_inner_v1_c.h>

#include <simulator_player_impl.h>

class client_player : public simulator_player_impl {
public:
    typedef simulator_player_impl::libuv_ptr_t libuv_ptr_t;

public:
    static void init_handles();

    client_player();
    virtual ~client_player();

    virtual int connect(const std::string &host, int port) UTIL_CONFIG_OVERRIDE;

    virtual int on_connected(libuv_ptr_t net, int status) UTIL_CONFIG_OVERRIDE;
    virtual void on_alloc(libuv_ptr_t net, size_t suggested_size, uv_buf_t *buf) UTIL_CONFIG_OVERRIDE;
    virtual void on_read_data(libuv_ptr_t net, ssize_t nread, const uv_buf_t *buf) UTIL_CONFIG_OVERRIDE;
    virtual void on_read_message(libuv_ptr_t net, const void *buffer, size_t sz) UTIL_CONFIG_OVERRIDE;
    virtual void on_written_data(libuv_ptr_t net, int status) UTIL_CONFIG_OVERRIDE;
    virtual int on_write_message(libuv_ptr_t net, void *buffer, uint64_t sz) UTIL_CONFIG_OVERRIDE;
    virtual int on_disconnected(libuv_ptr_t net) UTIL_CONFIG_OVERRIDE;

    virtual void on_close() UTIL_CONFIG_OVERRIDE;
    virtual void on_closed() UTIL_CONFIG_OVERRIDE;

    inline const hello::DPlatformData &get_platform() const { return platform_; }
    inline hello::DPlatformData &get_platform() { return platform_; }

    inline int32_t get_system_id() const { return system_id_; }
    inline void set_system_id(int32_t id) { system_id_ = id; }

    inline int32_t get_version() const { return version_; }
    inline void set_version(int32_t id) { version_ = id; }

    inline int32_t get_proto_version() const { return proto_version_; }
    inline void set_proto_version(int32_t id) { proto_version_ = id; }

    inline uint64_t get_user_id() const { return user_id_; }
    inline void set_user_id(uint64_t id) { user_id_ = id; }

    uint32_t alloc_sequence();

    inline const std::string &get_gamesvr_addr() const { return gamesvr_addr_; }
    inline void set_gamesvr_addr(const std::string &addr) { gamesvr_addr_ = addr; }

    inline const std::string &get_login_code() const { return login_code_; }
    inline void set_login_code(const std::string &code) { login_code_ = code; }

    inline int get_gamesvr_index() const { return gamesvr_index_; }
    inline void set_gamesvr_index(int index) { gamesvr_index_ = index; }

    libatgw_inner_v1_c_context mutable_proto_context(libuv_ptr_t net);
    void destroy_proto_context(libuv_ptr_t net);

    libuv_ptr_t find_network(libatgw_inner_v1_c_context ctx);
    using simulator_player_impl::find_network;

    void connect_done(libatgw_inner_v1_c_context ctx);

    inline bool is_connecting() const { return is_connecting_; }

private:
    std::map<uint32_t, libatgw_inner_v1_c_context> proto_handles_;
    hello::DPlatformData platform_;
    int32_t system_id_;
    int32_t version_;
    int32_t proto_version_;
    uint64_t user_id_;

    uint32_t sequence_;
    std::string gamesvr_addr_;
    std::string login_code_;
    int gamesvr_index_;

    std::vector<std::vector<unsigned char> > pending_msg_;
    bool is_connecting_;
};


#endif // ATFRAMEWORK_LIBSIMULATOR_UTILITY_CLIENT_PLAYER_H
