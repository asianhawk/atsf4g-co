# Windows下只用来看代码
#if (NOT WIN32 OR CYGWIN)
    add_subdirectory(${3RD_PARTY_REDIS_HAPP_DIR})
#endif()
