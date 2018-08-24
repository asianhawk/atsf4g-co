//
// Created by owt50 on 2017/2/6.
//

#include <common/string_oprs.h>

#include "protobuf_mini_dumper.h"

#define MSG_DISPATCHER_DEBUG_PRINT_BOUND 4096

const char *protobuf_mini_dumper_get_readable(const ::google::protobuf::Message &msg, uint8_t idx) {
    //    static char msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND] = {0};
    static std::string debug_string[256];

    ::google::protobuf::TextFormat::Printer printer;
    printer.SetUseUtf8StringEscaping(true);
    // printer.SetExpandAny(true);
    printer.SetUseShortRepeatedPrimitives(true);
    printer.SetSingleLineMode(false);
    printer.SetTruncateStringFieldLongerThan(MSG_DISPATCHER_DEBUG_PRINT_BOUND);
    printer.SetPrintMessageFieldsInIndexOrder(false);

    debug_string[idx].clear();
    printer.PrintToString(msg, &debug_string[idx]);

    //    msg_buffer[0] = 0;
    //    size_t sz = protobuf_mini_dumper_dump_readable(msg, msg_buffer, MSG_DISPATCHER_DEBUG_PRINT_BOUND - 1, 0);
    //
    //    if (sz > MSG_DISPATCHER_DEBUG_PRINT_BOUND - 5) {
    //        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 5] = '.';
    //        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 4] = '.';
    //        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 3] = '.';
    //        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 2] = '}';
    //        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 1] = 0;
    //    }
    return debug_string[idx].c_str();
}
