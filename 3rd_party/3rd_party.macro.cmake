# =========== 3rd_party ===========
unset (PROJECT_3RD_PARTY_SRC_LIST)

# =========== 3rd_party - msgpack ===========
include("${PROJECT_3RD_PARTY_ROOT_DIR}/msgpack/msgpack.cmake")

# =========== 3rd_party - libuv ===========
include("${PROJECT_3RD_PARTY_ROOT_DIR}/libuv/libuv.cmake")

# =========== 3rd_party - libiniloader ===========
include("${PROJECT_3RD_PARTY_ROOT_DIR}/libiniloader/libiniloader.cmake")

# =========== 3rd_party - libcurl ===========
include("${PROJECT_3RD_PARTY_ROOT_DIR}/libcurl/libcurl.cmake")

# =========== 3rd_party - rapidjson ===========
include("${PROJECT_3RD_PARTY_ROOT_DIR}/rapidjson/rapidjson.cmake")

# =========== 3rd_party - flatbuffers ===========
include("${PROJECT_3RD_PARTY_ROOT_DIR}/flatbuffers/flatbuffers.cmake")

# =========== 3rd_party - jemalloc ===========
if(NOT MSVC OR PROJECT_ENABLE_JEMALLOC)
    include("${PROJECT_3RD_PARTY_ROOT_DIR}/jemalloc/jemalloc.cmake")
endif()

# =========== 3rd_party - crypto ===========
# copy from atframework/libatframe_utils/repo/project/cmake/ProjectBuildOption.cmake
if (CRYPTO_USE_OPENSSL OR CRYPTO_USE_LIBRESSL OR CRYPTO_USE_BORINGSSL)
    find_package(OpenSSL)
    if (OPENSSL_FOUND)
        include_directories(${OPENSSL_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "CRYPTO_USE_OPENSSL,CRYPTO_USE_LIBRESSL,CRYPTO_USE_BORINGSSL is set but openssl not found")
    endif()
elseif (CRYPTO_USE_MBEDTLS)
    find_package(MbedTLS)
    if (MBEDTLS_FOUND) 
        include_directories(${MbedTLS_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "CRYPTO_USE_MBEDTLS is set but mbedtls not found")
    endif()
elseif (NOT CRYPTO_DISABLED)
    # try to find openssl or mbedtls
    find_package(OpenSSL)
    if (OPENSSL_FOUND)
        message(STATUS "Crypto enabled.(openssl found)")
        set(CRYPTO_USE_OPENSSL 1)
        include_directories(${OPENSSL_INCLUDE_DIR})
    else ()
        find_package(MbedTLS)
        if (MBEDTLS_FOUND) 
            message(STATUS "Crypto enabled.(mbedtls found)")
            set(CRYPTO_USE_MBEDTLS 1)
            include_directories(${MbedTLS_INCLUDE_DIRS})
        endif()
    endif()
endif()
if (OPENSSL_FOUND)
    list(APPEND 3RD_PARTY_CRYPT_LINK_NAME ${OPENSSL_CRYPTO_LIBRARY})
elseif (MBEDTLS_FOUND)
    list(APPEND 3RD_PARTY_CRYPT_LINK_NAME ${MbedTLS_CRYPTO_LIBRARIES})
else()
    message(FATAL_ERROR "must at least have one of openssl,libressl or mbedtls.")
endif()

if (NOT CRYPTO_DISABLED)
    find_package(Libsodium)
    if (Libsodium_FOUND)
        list(APPEND 3RD_PARTY_CRYPT_LINK_NAME ${Libsodium_LIBRARIES})
    endif()
endif()

if (MINGW)
    EchoWithColor(COLOR GREEN "-- MinGW: custom add lib gdi32")
    list(APPEND 3RD_PARTY_CRYPT_LINK_NAME gdi32)
endif()

# =========== 3rd_party - libcopp ===========
include("${PROJECT_3RD_PARTY_ROOT_DIR}/libcopp/libcopp.cmake")

# =========== 3rd_party - redis ===========
include("${PROJECT_3RD_PARTY_ROOT_DIR}/redis/redis.cmake")

# =========== 3rd_party - protobuf ===========
include("${PROJECT_3RD_PARTY_ROOT_DIR}/protobuf/protobuf.cmake")

## 导入所有工程项目
add_project_recurse(${CMAKE_CURRENT_LIST_DIR})