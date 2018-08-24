# ============ simulator - [...] ============
get_filename_component(SIMULATOR_SRC_BIN_NAME ${CMAKE_CURRENT_LIST_DIR} NAME_WE)
set(SIMULATOR_SRC_BIN_NAME "${SIMULATOR_SRC_BIN_NAME}-cli")
EchoWithColor(COLOR GREEN "-- Configure ${SIMULATOR_SRC_BIN_NAME} on ${CMAKE_CURRENT_LIST_DIR}")

aux_source_directory(${CMAKE_CURRENT_LIST_DIR} SIMULATOR_SRC_LIST)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_INSTALL_BAS_DIR}/tools/simulator/bin")
file(MAKE_DIRECTORY "${PROJECT_INSTALL_BAS_DIR}/tools/simulator/bin")

# special setting for sample
add_compiler_define(ATBUS_MACRO_MSG_LIMIT=65536)
include_directories("${ATFRAMEWORK_BASE_DIR}/service/atgateway/protocols"
    ${ATFRAMEWORK_SERVICE_COMPONENT_DIR}
    "${ATFRAMEWORK_BASE_DIR}/export/atgw_inner_v1_c")

set(SRC_LIST "${CMAKE_CURRENT_LIST_DIR}/main.cpp")
# build libserver_frame.so
file(GLOB SRC_LIST_LIBSIMULATOR_UV
    ${CMAKE_CURRENT_LIST_DIR}/libsimulator_uv/*.h
    ${CMAKE_CURRENT_LIST_DIR}/libsimulator_uv/*.c
    ${CMAKE_CURRENT_LIST_DIR}/libsimulator_uv/*.cpp
)
list(APPEND SRC_LIST ${SRC_LIST_LIBSIMULATOR_UV})

file(GLOB SRC_LIST_UTILITY
    ${CMAKE_CURRENT_LIST_DIR}/utility/*.h
    ${CMAKE_CURRENT_LIST_DIR}/utility/*.cpp
)
list(APPEND SRC_LIST ${SRC_LIST_UTILITY})

file(GLOB_RECURSE SRC_LIST_PROTO
    ${CMAKE_CURRENT_LIST_DIR}/proto/*.h
    ${CMAKE_CURRENT_LIST_DIR}/proto/*.cpp
)
list(APPEND SRC_LIST ${SRC_LIST_PROTO})

# these service will use atgateway protocol
include_directories(
    ${CMAKE_CURRENT_LIST_DIR} ${PROJECT_SERVER_FRAME_INC_DIR} ${ATFRAMEWORK_SERVICE_GATEWAY_PROTOCOL_DIR}
    "${CMAKE_CURRENT_LIST_DIR}/libsimulator_uv"
)

find_package(Lua)
if (LUA_FOUND)
    include_directories(${LUA_INCLUDE_DIR})
    add_compiler_define(SIMULATOR_ENABLE_LUA=1)
    file(GLOB_RECURSE SRC_LIST_LUA
        ${CMAKE_CURRENT_LIST_DIR}/lua/*.h
        ${CMAKE_CURRENT_LIST_DIR}/lua/*.cpp
    )
    list(APPEND SRC_LIST ${SRC_LIST_LUA})

    # copy lua脚本
    EchoWithColor(COLOR GREEN "-- Install: Copy ${CMAKE_CURRENT_LIST_DIR}/lua/lua ...")
    file(
        COPY "${CMAKE_CURRENT_LIST_DIR}/lua/lua"
        DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lua"
        USE_SOURCE_PERMISSIONS
    )
endif()

source_group_by_dir(SRC_LIST)

# protocol hack
if (OPENSSL_FOUND)
    add_compiler_define(LIBATFRAME_ATGATEWAY_ENABLE_OPENSSL=1)
elseif(LIBRESSL_FOUND)
    add_compiler_define(LIBATFRAME_ATGATEWAY_ENABLE_LIBRESSL=1)
elseif(MBEDTLS_FOUND)
    add_compiler_define(LIBATFRAME_ATGATEWAY_ENABLE_MBEDTLS=1)
endif()

add_executable(${SIMULATOR_SRC_BIN_NAME}
    ${SRC_LIST} ${PROJECT_3RD_PARTY_SRC_LIST}
)

target_link_libraries(${SIMULATOR_SRC_BIN_NAME}
    ${LUA_LIBRARIES}
    ${PROJECT_SERVER_FRAME_LIB_LINK}
    ${EXPORT_LIBATGW_INNER_V1_C_BIN_NAME}
    ${ATFRAMEWORK_ATBUS_LINK_NAME}
    ${ATFRAMEWORK_ATFRAME_UTILS_LINK_NAME}
    ${3RD_PARTY_PROTOBUF_LINK_NAME}
    ${3RD_PARTY_LIBUV_LINK_NAME}
    ${3RD_PARTY_CRYPT_LINK_NAME}
    ${COMPILER_OPTION_EXTERN_CXX_LIBS}
)
