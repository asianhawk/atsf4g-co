include_directories(${PROJECT_SERVER_FRAME_INC_DIR})

# generated config loader protocol
add_custom_command(OUTPUT 
    "${PROJECT_SERVER_FRAME_SRC_DIR}/config/excel/pb_header_v3.pb.h"
    "${PROJECT_SERVER_FRAME_SRC_DIR}/config/excel/pb_header_v3.pb.cc"
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    --proto_path "./"
    --cpp_out "./"
    #-o "${PROJECT_INSTALL_RES_PBD_DIR}/config.pb"
    "./pb_header_v3.proto"
    WORKING_DIRECTORY "${PROJECT_SERVER_FRAME_SRC_DIR}/config/excel"
    DEPENDS "${PROJECT_SERVER_FRAME_SRC_DIR}/config/excel/pb_header_v3.proto"
    COMMENT "Generate [@${PROJECT_SERVER_FRAME_SRC_DIR}/config/excel] ./pb_header_v3.proto"
)

# build libserver_frame.so
file(GLOB_RECURSE SRC_LIST
    RELATIVE ${PROJECT_SERVER_FRAME_SRC_DIR}
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.h
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.hpp
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.c
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.cpp
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.cc
)

# add generated files
include("${PROJECT_SERVER_FRAME_BAS_DIR}/protocol/generate_proto_source.cmake")
list(REMOVE_DUPLICATES SRC_LIST)

source_group_by_dir(SRC_LIST)
if ( NOT MSVC AND NOT APPLE )
    add_definitions(-shared)
endif()

find_package(Libuuid)
if(NOT Libuuid_FOUND)
    EchoWithColor(COLOR YELLOW "-- Dependency: libuuid not found")
    # message(FATAL_ERROR "libuuid-devel or uuid-dev is required")
    set(Libuuid_LIBRARIES "")
endif()

# these service will use atgateway protocol
include_directories(${ATFRAMEWORK_SERVICE_GATEWAY_PROTOCOL_DIR})

add_library(${PROJECT_SERVER_FRAME_LIB_LINK} SHARED
    ${SRC_LIST}
    ${PROJECT_3RD_PARTY_SRC_LIST}
)

target_link_libraries(${PROJECT_SERVER_FRAME_LIB_LINK}
    ${PROJECT_LIB_LINK}
    ${3RD_PARTY_LIBCOPP_LINK_NAME}
    ${EXPORT_LIBATGW_INNER_V1_C_BIN_NAME}
    ${ATFRAMEWORK_SERVICE_COMPONENT_LINK_NAME}
    ${ATFRAMEWORK_ATAPP_LINK_NAME}
    ${ATFRAMEWORK_ATBUS_LINK_NAME}
    ${ATFRAMEWORK_ATFRAME_UTILS_LINK_NAME}
    ${3RD_PARTY_PROTOBUF_LINK_NAME}
    ${3RD_PARTY_REDIS_LINK_NAME}
    ${3RD_PARTY_LIBUV_LINK_NAME}
    ${Libuuid_LIBRARIES}
    ${COMPILER_OPTION_EXTERN_CXX_LIBS}
)
