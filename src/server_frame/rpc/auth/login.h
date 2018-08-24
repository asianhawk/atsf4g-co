//
// Created by owt50 on 2016/9/28.
//

#ifndef RPC_AUTH_LOGIN_H
#define RPC_AUTH_LOGIN_H

#pragma once

#include <stdint.h>
#include <cstddef>
#include <string>

namespace rpc {
    namespace auth {
        namespace login {
            /**
             * @brief generate logincode
             * @param code where to store code
             * @param sz code length
             */
            void generate_login_code(char *code, size_t sz);

            /**
             * @brief add platform and channel prefix into openid
             * @param plat_id platform id (determine how to login)
             * @param channel_id channel id (what's the publish channel)
             * @param openid raw openid
             * @return final openid
             */
            std::string make_open_id(uint32_t zone_id, uint32_t plat_id, uint32_t channel_id, const std::string &openid);
        }
    }
}

#endif //_RPC_AUTH_LOGIN_H
