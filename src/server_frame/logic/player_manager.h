#ifndef LOGIC_PLAYER_MANAGER_H
#define LOGIC_PLAYER_MANAGER_H

#pragma once

#include <list>

#include <design_pattern/singleton.h>

#include <protocol/pbdesc/svr.table.pb.h>

#include <utility/environment_helper.h>

#include <data/player.h>
#include <data/session.h>

class player_manager : public util::design_pattern::singleton<player_manager> {
public:
    typedef player::ptr_t player_ptr_t;

public:
    /**
     * @brief 移除用户
     * @param user user指针
     * @param force 强制移除，不进入离线缓存
     */
    bool remove(player_ptr_t user, bool force_kickoff = false);


    /**
     * @brief 保存用户数据
     * @param user_id user_id
     */
    bool save(uint64_t user_id);

    /**
     * @brief 加载指定玩家数据。
     * @note 注意这个函数只是读数据库做缓存。
     * @note gamesvr 请不要强制拉去数据 会冲掉玩家数据
     * @note 返回的 user 指针不能用于改写玩家数据，不做保存。
     * @param user_id
     * @return null 或者 user指针
     */
    player_ptr_t load(uint64_t user_id, bool force = false);

    size_t size() const;

    player_ptr_t create(uint64_t user_id, const std::string &openid, hello::table_login &login_tb, std::string &login_ver);

    const player_ptr_t find(uint64_t user_id) const;
    player_ptr_t find(uint64_t user_id);
};

#endif