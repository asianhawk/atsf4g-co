//
// Created by owt50 on 2017/2/6.
//

#ifndef UTILITY_PROTOBUF_MINI_DUMPER_H
#define UTILITY_PROTOBUF_MINI_DUMPER_H

#pragma once

#include <cstddef>
#include <stdint.h>


#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>

/**
 * @brief 返回易读数据
 * @note 因为protobuf默认的DebugString某些情况下会打印出巨量不易读的内容，故而自己实现。优化一下。
 * @param msg 要打印的message
 * @param ident 缩进层级
 */
const char *protobuf_mini_dumper_get_readable(const ::google::protobuf::Message &msg, uint8_t idx = 0);

#endif //_UTILITY_PROTOBUF_MINI_DUMPER_H
