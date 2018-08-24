//
// Created by owent on 2018/05/01.
//

#ifndef ROUTER_ROUTER_MANAGER_SET_H
#define ROUTER_ROUTER_MANAGER_SET_H

#pragma once

#include <cstddef>
#include <list>
#include <stdint.h>

#include <design_pattern/singleton.h>
#include <std/smart_ptr.h>

#include <protocol/pbdesc/svr.const.pb.h>
#include <protocol/pbdesc/svr.protocol.pb.h>

#include <utility/environment_helper.h>

#include <dispatcher/task_manager.h>

class router_object_base;
class router_manager_base;
class task_action_auto_save_objects;
class router_manager_set : public util::design_pattern::singleton<router_manager_set> {
public:
    struct timer_t {
        uint32_t timer_sequence;
        uint32_t type_id;
        time_t timeout;
        std::weak_ptr<router_object_base> obj_watcher;
    };

    enum auto_save_action_t {
        EN_ASA_SAVE = 0,
        EN_ASA_REMOVE_OBJECT,
        EN_ASA_REMOVE_CACHE,
    };

    struct auto_save_data_t {
        auto_save_action_t action;
        uint32_t type_id;

        std::shared_ptr<router_object_base> object;
    };

public:
    router_manager_set();

    int init();

    int tick();

    bool insert_timer(router_manager_base *mgr, const std::shared_ptr<router_object_base> &obj, bool is_fast = false);

    router_manager_base *get_manager(uint32_t type);

    int register_manager(router_manager_base *b);
    int unregister_manager(router_manager_base *b);

private:
    bool is_save_task_running() const;

    int tick_timer(time_t cache_expire, time_t object_expire, time_t object_save, std::list<timer_t> &timer_list, bool is_fast);

private:
    struct timer_set_t {
        std::list<timer_t> default_timer_list;
        std::list<timer_t> fast_timer_list;
    };
    timer_set_t timers_;
    time_t last_proc_time_;
    router_manager_base *mgrs_[hello::EnRouterObjectType_ARRAYSIZE];
    std::list<auto_save_data_t> save_list_;
    task_manager::task_ptr_t save_task_;

    friend class task_action_auto_save_objects;
};


#endif //_ROUTER_ROUTER_MANAGER_SET_H
