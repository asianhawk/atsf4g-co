#include <log/log_wrapper.h>

#include <protocol/pbdesc/com.protocol.pb.h>
#include <protocol/pbdesc/svr.table.pb.h>

#include <config/logic_config.h>
#include <time/time_utility.h>

#include <logic/player_manager.h>

#include <logic/action/task_action_player_remote_patch_jobs.h>

#include "player.h"
#include "session.h"


player::player(fake_constructor &) : user_id_(0), data_version_(0) {
    heartbeat_data_.continue_error_times = 0;
    heartbeat_data_.last_recv_time = 0;
    heartbeat_data_.sum_error_times = 0;
}

player::~player() { WLOGDEBUG("player %s destroyed", openid_id_.c_str()); }

void player::init(uint64_t user_id, const std::string &openid) {
    user_id_ = user_id;
    openid_id_ = openid;
    data_version_ = 0;

    // all manager init
    // ptr_t self = shared_from_this();
}

player::ptr_t player::create(uint64_t user_id, const std::string &openid) {
    fake_constructor ctorp;
    ptr_t ret = std::make_shared<player>(ctorp);
    if (ret) {
        ret->init(user_id, openid);
    }

    return std::move(ret);
}

void player::create_init(uint32_t version_type) {
    set_player_level(1);
    data_version_ = PLAYER_DATA_LOGIC_VERSION;
    version_.assign("0");

    // TODO all module create init
    // TODO init all interval checkpoint

    // TODO init items
    // if (hello::EN_VERSION_GM != version_type) {
    //     excel::player_init_items::me()->foreach ([this](const excel::player_init_items::value_type &v) {
    //         if (0 != v->id()) {
    //             add_entity(v->id(), v->number(), hello::EN_ICMT_INIT, hello::EN_ICST_DEFAULT);
    //         }
    //     });
    // }
}

void player::login_init() {
    // refresh platform parameters
    {
        hello::platform_information &platform = get_platform_info();
        platform.set_access(get_login_info().platform().access());
        platform.set_platform_id(get_login_info().platform().platform_id());
        platform.set_version_type(get_login_info().platform().version_type());

        // 冗余的key字段
        platform.mutable_profile()->set_open_id(get_open_id());
        platform.mutable_profile()->set_user_id(get_user_id());
    }

    // TODO check all interval checkpoint

    // TODO all module login init

    set_inited();
    on_login();

    // TODO sync messages
    // TODO 重试失败的订单
}

void player::refresh_feature_limit() {
    // refresh daily limit
}

void player::on_login() {}

void player::on_logout() {}

void player::on_remove() {}

void player::set_session(std::shared_ptr<session> session_ptr) {
    std::shared_ptr<session> old_sess = session_.lock();
    if (old_sess == session_ptr) {
        return;
    }

    session_ = session_ptr;

    // 如果为置空Session，则要加入登出缓存排队列表
    if (!session_ptr) {
        // 移除Session时触发Logout
        if (old_sess) {
            on_logout();
        }
    }
}

std::shared_ptr<session> player::get_session() { return session_.lock(); }

bool player::has_session() const { return false == session_.expired(); }

void player::init_from_table_data(const hello::table_user &tb_player) {
    data_version_ = tb_player.data_version();

    // TODO data patch, 这里用于版本升级时可能需要升级玩家数据库，做版本迁移
    // hello::table_user tb_patch;
    const hello::table_user *src_tb = &tb_player;
    // if (data_version_ < PLAYER_DATA_LOGIC_VERSION) {
    //     tb_patch.CopyFrom(tb_player);
    //     src_tb = &tb_patch;
    //     //GameUserPatchMgr::Instance()->Patch(tb_patch, m_iDataVersion, GAME_USER_DATA_LOGIC);
    //     data_version_ = PLAYER_DATA_LOGIC_VERSION;
    // }

    if (src_tb->has_platform()) {
        platform_info_.ref().CopyFrom(src_tb->platform());
    }

    if (src_tb->has_player()) {
        player_data_.ref().CopyFrom(src_tb->player());
    }

    if (src_tb->has_options()) {
        player_options_.ref().CopyFrom(src_tb->options());
    }

    // TODO all modules load from DB
}

int player::dump(hello::table_user &user, bool always) {
    user.set_open_id(get_open_id());
    user.set_user_id(get_user_id());
    user.set_data_version(data_version_);

    if (always || player_data_.is_dirty()) {
        user.mutable_player()->CopyFrom(player_data_);
    }

    if (always || platform_info_.is_dirty()) {
        user.mutable_platform()->CopyFrom(platform_info_);
    }

    if (always || player_options_.is_dirty()) {
        user.mutable_options()->CopyFrom(player_options_);
    }

    return 0;
}

bool player::gm_init() { return true; }

bool player::is_gm() const { return get_platform_info().platform_id() == hello::EN_OS_WINDOWS; }

player::ptr_t player::get_global_gm_player() {
    static ptr_t shared_gm_player;
    if (shared_gm_player) {
        return shared_gm_player;
    }

    fake_constructor ctorp;
    shared_gm_player = std::make_shared<player>(ctorp);
    shared_gm_player->init(0, logic_config::me()->get_cfg_logic().player_default_openid);
    shared_gm_player->create_init(hello::EN_VERSION_GM);

    WLOGINFO("init gm defaule user %s(%llu)\n", shared_gm_player->get_open_id().c_str(), shared_gm_player->get_user_id_llu());
    return shared_gm_player;
}

int player::on_fake_profile() {
    // TODO 假账号的profile
    return 0;
}

uint32_t player::get_player_level() const { return player_data_->player_level(); }

void player::set_player_level(uint32_t level) {
    // 由1级开始，和玩家等级略有差别
    uint32_t ori_lv = get_player_level();
    if (ori_lv >= level) {
        return;
    }

    // TODO 每一级的触发事件和奖励
    // TODO 同步协议
    // get_cache_data().p

    player_data_.ref().set_player_level(level);
    // TODO 任务事件
}

void player::update_heartbeat() {
    const logic_config::LC_LOGIC &logic_cfg = logic_config::me()->get_cfg_logic();
    time_t heartbeat_interval = logic_cfg.heartbeat_interval;
    time_t heartbeat_tolerance = logic_cfg.heartbeat_tolerance;
    time_t tol_dura = heartbeat_interval - heartbeat_tolerance;
    time_t now_time = util::time::time_utility::get_now();

    // 小于容忍值得要统计错误次数
    if (now_time - heartbeat_data_.last_recv_time < tol_dura) {
        ++heartbeat_data_.continue_error_times;
        ++heartbeat_data_.sum_error_times;
    } else {
        heartbeat_data_.continue_error_times = 0;
    }

    heartbeat_data_.last_recv_time = now_time;

    // 顺带更新login_code的有效期
    get_login_info().set_login_code_expired(now_time + logic_cfg.session_login_code_valid_sec);
}

void player::start_patch_remote_command() {
    inner_flags_.set(inner_flag::EN_IFT_NEED_PATCH_REMOTE_COMMAND, true);

    try_patch_remote_command();
}

void player::try_patch_remote_command() {
    if (remote_command_patch_task_ && remote_command_patch_task_->is_exiting()) {
        remote_command_patch_task_.reset();
    }

    if (!inner_flags_.test(inner_flag::EN_IFT_NEED_PATCH_REMOTE_COMMAND)) {
        return;
    }

    // 队列化，同时只能一个任务执行
    if (remote_command_patch_task_) {
        return;
    }

    inner_flags_.set(inner_flag::EN_IFT_NEED_PATCH_REMOTE_COMMAND, false);

    task_manager::id_t tid = 0;
    task_action_player_remote_patch_jobs::ctor_param_t params;
    params.user = shared_from_this();
    params.timeout_duration = logic_config::me()->get_cfg_logic().task_nomsg_timeout;
    params.timeout_timepoint = util::time::time_utility::get_now() + params.timeout_duration;
    task_manager::me()->create_task_with_timeout<task_action_player_remote_patch_jobs>(tid, params.timeout_duration, COPP_MACRO_STD_MOVE(params));

    if (0 == tid) {
        WLOGERROR("create task_action_player_remote_patch_jobs failed");
    } else {
        remote_command_patch_task_ = task_manager::me()->get_task(tid);

        dispatcher_start_data_t start_data;
        start_data.private_data = NULL;
        start_data.message.msg_addr = NULL;
        start_data.message.msg_type = 0;

        int res = task_manager::me()->start_task(tid, start_data);
        if (res < 0) {
            WLOGERROR("start task_action_player_remote_patch_jobs for player %s(%llu) failed, res: %d", get_open_id().c_str(), get_user_id_llu(), res);
            remote_command_patch_task_.reset();
            return;
        }
    }
}

void player::send_all_syn_msg() {
    // TODO 升级通知
    // TODO 道具变更通知
    // TODO 任务/成就变更通知
}

void player::clear_dirty_cache() {
    // TODO 清理要推送的脏数据
}

player_cs_syn_msg_holder::player_cs_syn_msg_holder(player::ptr_t u) : owner_(u) {}
player_cs_syn_msg_holder::~player_cs_syn_msg_holder() {
    if (!owner_) {
        return;
    }

    std::shared_ptr<session> sess = owner_->get_session();
    if (!sess) {
        return;
    }

    if (msg_.has_body()) {
        sess->send_msg_to_client(msg_);
    }
}