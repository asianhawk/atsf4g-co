//
// Created by owt50 on 2016/10/9.
//

#include <config/compiler_features.h>

#include <log/log_wrapper.h>
#include <random/uuid_generator.h>

#include <protocol/config/com.const.config.pb.h>
#include <protocol/pbdesc/svr.const.err.pb.h>
#include <protocol/pbdesc/svr.table.pb.h>


#include <dispatcher/db_msg_dispatcher.h>
#include <dispatcher/task_manager.h>


#include "../rpc_utils.h"
#include "db_utils.h"

#include "uuid.h"

namespace rpc {
    namespace db {
        namespace uuid {
            int generate_standard_uuid(std::string &uuid) {
                uuid = util::random::uuid_generator::generate_string();
                return hello::err::EN_SUCCESS;
            }

            static int64_t generate_global_unique_id_pool(uint8_t type_id) {
                task_manager::task_t *task = task_manager::task_t::this_task();
                if (!task) {
                    WLOGERROR("current not in a task");
                    return hello::err::EN_SYS_RPC_NO_TASK;
                }

                // 这个算法比许固定
                char keyvar[64];
                size_t keylen = sizeof(keyvar) - 1;
                int __snprintf_writen_length = UTIL_STRFUNC_SNPRINTF(keyvar, static_cast<int>(keylen), "guid:%x", type_id);
                if (__snprintf_writen_length < 0) {
                    keyvar[sizeof(keyvar) - 1] = '\0';
                    keylen = 0;
                } else {
                    keylen = static_cast<size_t>(__snprintf_writen_length);
                    keyvar[__snprintf_writen_length] = '\0';
                }

                redis_args args(2);
                {
                    args.push("INCR");
                    args.push(keyvar);
                }

                int res = db_msg_dispatcher::me()->send_msg(db_msg_dispatcher::channel_t::CLUSTER_DEFAULT, keyvar, keylen, task->get_id(),
                                                            logic_config::me()->get_self_bus_id(), detail::unpack_integer, static_cast<int>(args.size()),
                                                            args.get_args_values(), args.get_args_lengths());

                if (res < 0) {
                    return res;
                }

                hello::table_all_message msg;
                // 协程操作
                res = rpc::wait(msg);
                if (res < 0) {
                    return res;
                }

                if (!msg.has_simple()) {
                    return hello::err::EN_DB_RECORD_NOT_FOUND;
                }

                return msg.simple().msg_i64();
            }

            int64_t generate_global_unique_id(uint8_t type_id) {
                if (NULL == task_manager::task_t::this_task()) {
                    return hello::err::EN_SYS_RPC_NO_TASK;
                }

                // POOL => 1 | 50 | 13
                UTIL_CONFIG_CONSTEXPR int64_t bits_off = 13;
                UTIL_CONFIG_CONSTEXPR int64_t bits_range = 1 << bits_off;
                UTIL_CONFIG_CONSTEXPR int64_t bits_mask = bits_range - 1;
                UTIL_CONFIG_CONSTEXPR size_t type_id_array = 256;

                // static std::atomic<int64_t> unique_id_index_pool[type_id_array] = 0;
                // static std::atomic<int64_t> unique_id_base_pool[type_id_array] = 0;
                static int64_t unique_id_index_pool[type_id_array] = {0};
                static int64_t unique_id_base_pool[type_id_array] = {0};

                task_manager::task_ptr_t alloc_task;

                int64_t ret = 0;
                int try_left = 5;

                while (try_left-- > 0 && ret <= 0) {
                    int64_t &unique_id_index = unique_id_index_pool[type_id];
                    int64_t &unique_id_base = unique_id_base_pool[type_id];

                    // must in task
                    assert(task_manager::task_t::this_task());
                    unique_id_index &= bits_mask;

                    //                    ret = (unique_id_base.load(std::memory_order_acquire) << bits_off) |
                    //                          (unique_id_index.fetch_add(1, std::memory_order_acq_rel));

                    ret = (unique_id_base << bits_off) | (unique_id_index++);

                    // call rpc to allocate a id pool
                    if (0 == (ret >> bits_off) || 0 == (ret & bits_mask)) {
                        task_manager::task_t *this_task = task_manager::task_t::this_task();
                        // 任务已经失败或者不在任务中
                        if (nullptr == this_task || this_task->is_completed()) {
                            ret = 0;
                            break;
                        }

                        // 如果已有分配请求，仅仅排队即可
                        if (alloc_task) {
                            alloc_task->next(task_manager::task_ptr_t(this_task));
                            this_task->yield(NULL); // 切出，等待切回后继续
                            ret = 0;
                            continue;
                        }

                        alloc_task = task_manager::me()->get_task(this_task->get_id());
                        int64_t res = generate_global_unique_id_pool(type_id);
                        alloc_task.reset();
                        if (res <= 0) {
                            ret = res;
                            continue;
                        }
                        // unique_id_base.store(res - hello::config::EN_GCC_GLOBAL_ALLOC_ID_START, std::memory_order_release);
                        // unique_id_index.store(1, std::memory_order_release);
                        unique_id_base = res;
                        unique_id_index = 1;
                    }
                }

                return ret;
            }

        } // namespace uuid
    }     // namespace db
} // namespace rpc
