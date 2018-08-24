//
// Created by owt50 on 2016/10/9.
//

#ifndef RPC_DB_UUID_H
#define RPC_DB_UUID_H

#pragma once

#include <string>

namespace rpc {
    namespace db {
        namespace uuid {
            /**
             * 生成指定类型的UUID
             * @param uuid 输出生成的UUID
             * @return 0或错误码
             */
            int generate_standard_uuid(std::string &uuid);

            int64_t generate_global_unique_id(uint8_t type_id);

        } // namespace uuid
    }     // namespace db
} // namespace rpc

#endif //_RPC_DB_UUID_H
