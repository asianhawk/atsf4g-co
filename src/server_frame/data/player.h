#ifndef DATA_PLAYER_H
#define DATA_PLAYER_H

#pragma once

#include <bitset>

#include <config/compiler_features.h>
#include <design_pattern/noncopyable.h>
#include <std/smart_ptr.h>

#include <protocol/pbdesc/com.protocol.pb.h>
#include <protocol/pbdesc/svr.table.pb.h>


#include <dispatcher/task_manager.h>

class session;

/**
 * @brief 用户数据包装，自动标记写脏
 * @note 能够隐式转换到只读类型，手动使用get或ref函数提取数据会视为即将写脏
 */
template <typename Ty>
class player_dirty_wrapper {
public:
    typedef Ty value_type;

    player_dirty_wrapper() : dirty_(false) {}

    inline bool is_dirty() const { return dirty_; }

    inline void mark_dirty() { dirty_ = true; }

    inline void clear_dirty() { dirty_ = false; }

    const value_type *operator->() const UTIL_CONFIG_NOEXCEPT { return &real_data_; }

    operator const value_type &() const UTIL_CONFIG_NOEXCEPT { return real_data_; }

    const value_type &operator*() const UTIL_CONFIG_NOEXCEPT { return real_data_; }

    const value_type *get() const { return &real_data_; }

    value_type *get() {
        mark_dirty();
        return &real_data_;
    }

    const value_type &ref() const { return real_data_; }

    value_type &ref() {
        mark_dirty();
        return real_data_;
    }

private:
    value_type real_data_;
    bool dirty_;
};

/**
 * @brief 用户数据缓存包装，析构时自动还原
 * @note 注意只能用作局部变量
 */
template <typename Ty>
class player_cache_ptr_holder : public util::design_pattern::noncopyable {
public:
    typedef Ty value_type;
    typedef value_type *pointer_type;

    player_cache_ptr_holder(pointer_type &holded, pointer_type ptr_addr) : ptr_addr_(NULL) {
        if (NULL != holded || NULL == ptr_addr) {
            return;
        }

        ptr_addr_ = &holded;
        holded = ptr_addr;
    }

    ~player_cache_ptr_holder() UTIL_CONFIG_NOEXCEPT {
        if (NULL != ptr_addr_) {
            *ptr_addr_ = NULL;
        }
    }

    bool available() const UTIL_CONFIG_NOEXCEPT { return NULL != ptr_addr_ && NULL != *ptr_addr_; }

protected:
    pointer_type *ptr_addr_;
};


class player : public std::enable_shared_from_this<player> {
private:
    static const uint32_t PLAYER_DATA_LOGIC_VERSION = 1;
    struct inner_flag {
        enum type {
            EN_IFT_FEATURE_INVALID = 0,
            EN_IFT_IS_INITED,                 // 是否已初始化
            EN_IFT_NEED_PATCH_REMOTE_COMMAND, // 是否需要启动远程命令任务
            EN_IFT_MAX
        };
    };

public:
    typedef std::shared_ptr<player> ptr_t;
    friend class player_manager;
    friend class task_action_player_remote_patch_jobs;

    struct heartbeat_t {
        time_t last_recv_time;       // 上一次收到心跳包时间
        size_t continue_error_times; // 连续错误次数
        size_t sum_error_times;      // 总错误次数
    };
    /** 因为会对其进行memset，所以内部不允许出现非POD类型 **/


    struct cache_t {
        // hello::SCPlayerLevelupSyn player_level_up_syn;
    };

private:
    struct fake_constructor {};

public:
    player(fake_constructor &);
    ~player();

    // 初始化，默认数据
    void init(uint64_t user_id, const std::string &openid);

    static ptr_t create(uint64_t user_id, const std::string &openid);

    // 创建默认角色数据
    void create_init(uint32_t version_type);

    // 登入读取用户数据
    void login_init();

    // 刷新功能限制次数
    void refresh_feature_limit();

    // 登入事件
    void on_login();

    // 登出事件
    void on_logout();

    // 移除事件
    void on_remove();

    /**
     * @brief 监视关联的Session
     * @param session_ptr 关联的Session
     */
    void set_session(std::shared_ptr<session> session_ptr);

    /**
     * @brief 获取关联的Session
     * @return 关联的Session
     */
    std::shared_ptr<session> get_session();

    bool has_session() const;

    inline const std::string &get_open_id() const { return openid_id_; };
    inline uint64_t get_user_id() const { return user_id_; };
    inline unsigned long long get_user_id_llu() const { return static_cast<unsigned long long>(get_user_id()); };

    const std::string &get_version() const { return version_; };
    std::string &get_version() { return version_; };
    void set_version(const std::string &version) { version_ = version; };

    const hello::DClientDeviceInfo &get_client_info() const { return client_info_; }
    void set_client_info(const hello::DClientDeviceInfo &info) { client_info_.CopyFrom(info); }

    /**
     * @brief 是否完整执行过初始化
     * @note 如果完整执行了登入流程，则会走完整初始化流程。这个flag还有一个含义是玩家数据仅仅在此进程内可写。
     *       比如如果一个玩家对象是缓存，则不会走完整的登入流程，也不会被完全初始化，那么这个数据就是只读的。
     *        这时候如果登出或者移除玩家对象的时候清理就不能写数据库。
     */
    bool is_inited() const { return inner_flags_.test(inner_flag::EN_IFT_IS_INITED); }
    /**
     * @brief 标记为完全初始化，也表示在此进程中玩家数据是可写的。
     * @note 这个flag用于标记玩家实时数据必须最多唯一存在于一个进程中，其他进程的数据都是缓存。
     *       缓存可以升级为实时数据，但是不能降级。如果需要降级，则直接移除玩家对象，下一次需要的时候重新拉取缓存
     */
    void set_inited() { inner_flags_.set(inner_flag::EN_IFT_IS_INITED, true); }

    // 从table数据初始化
    void init_from_table_data(const hello::table_user &stTablePlayer);

    /**
     * @brief 转储数据
     * @param user 转储目标
     * @param always 是否忽略脏数据
     * @return 0或错误码
     */
    int dump(hello::table_user &user, bool always);

    // GM操作
    bool gm_init();

    // 是否GM操作
    bool is_gm() const;

    static ptr_t get_global_gm_player();

    // 关系链
    int on_fake_profile();

    /**
     * @brief 获取大区号
     */
    inline uint32_t get_zone_id() const { return platform_info_->zone_id(); }

    /**
     * @brief 获取玩家等级
     * @return 玩家等级
     */
    uint32_t get_player_level() const;

    /**
     * @brief  设置玩家等级
     * @param level 玩家等级
     */
    void set_player_level(uint32_t level);

    /**
     * @brief 获取心跳包统计数据
     * @return 心跳包统计数据
     */
    inline const heartbeat_t &get_heartbeat_data() const { return heartbeat_data_; }

    /**
     * @brief 更新心跳包统计数据
     */
    void update_heartbeat();

    /**
     * @brief 获取缓存信息
     * @return 缓存信息
     */
    inline const cache_t &get_cache_data() const { return cache_data_; }

    /**
     * @brief 获取缓存信息
     * @return 缓存信息
     */
    inline cache_t &get_cache_data() { return cache_data_; }

    /**
     * @brief 开始执行远程命令，如果已经有一个命令正在运行中了，会等待那个命令完成后再启动一次
     * @note 远程命令一般用于多写入方，利用数据库插入命令。然后再通知玩家对象，单点执行读入数据。
     * @note 比如说多个玩家对一个玩家发送消息或邮件，可以插入消息或邮件的command到数据库，然后这里拉取后append到玩家数据里
     */
    void start_patch_remote_command();

    /**
     * @brief 下发同步消息
     */
    void send_all_syn_msg();
    void clear_dirty_cache();

    inline const hello::table_login &get_login_info() const { return login_info_; }
    inline hello::table_login &get_login_info() { return login_info_; }

    inline const std::string &get_login_version() const { return login_info_version_; }
    inline std::string &get_login_version() { return login_info_version_; }

    inline const hello::platform_information &get_platform_info() const { return platform_info_; }
    inline hello::platform_information &get_platform_info() { return platform_info_.ref(); }

    inline const hello::player_options &get_player_options() const { return player_options_; }
    inline hello::player_options &get_player_options() { return player_options_.ref(); }

    inline const hello::player_data &get_player_data() const { return player_data_; }

    inline uint32_t get_data_version() const { return data_version_; }

private:
    inline hello::player_data &mutable_player_data() { return player_data_.ref(); }

    /**
     * @brief 尝试开始执行远程命令，如果已经有一个命令正在运行中了，会忽略并返回
     */
    void try_patch_remote_command();

private:
    std::string openid_id_;
    uint64_t user_id_;
    hello::table_login login_info_;
    std::string login_info_version_;

    std::string version_;
    uint32_t data_version_;

    mutable std::bitset<inner_flag::EN_IFT_MAX> inner_flags_;

    std::weak_ptr<session> session_;

    hello::DClientDeviceInfo client_info_;

    player_dirty_wrapper<hello::platform_information> platform_info_;
    player_dirty_wrapper<hello::player_data> player_data_;
    player_dirty_wrapper<hello::player_options> player_options_;

    // =======================================================
    heartbeat_t heartbeat_data_;
    cache_t cache_data_;
    task_manager::task_ptr_t remote_command_patch_task_;
    // -------------------------------------------------------
};

/**
 * @brief 用户回包数据缓存包装，析构时自动还原发送数据
 * @note 注意只能用作局部变量
 */
class player_cs_syn_msg_holder {
public:
    typedef hello::CSMsg value_type;

public:
    player_cs_syn_msg_holder(player::ptr_t u);
    ~player_cs_syn_msg_holder();

    const value_type *operator->() const UTIL_CONFIG_NOEXCEPT { return &msg_; }

    operator const value_type &() const UTIL_CONFIG_NOEXCEPT { return msg_; }

    const value_type *get() const { return &msg_; }

    value_type *get() { return &msg_; }

    const value_type &ref() const { return msg_; }

    value_type &ref() { return msg_; }

private:
    player::ptr_t owner_;
    value_type msg_;
};

#endif