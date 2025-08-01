add_executable(memcached_client client.cpp)
target_link_libraries(memcached_client PRIVATE pthread)

add_executable(memcached_vanilla vanilla/server.cpp vanilla/hashmap.cpp)
target_link_libraries(memcached_vanilla PRIVATE ${LIBS_disabled})

add_executable(memcached_vanilla_mem vanilla/server.cpp vanilla/hashmap.cpp)
target_link_libraries(memcached_vanilla_mem PRIVATE ${LIBS_disabled} profile_mem)
target_compile_definitions(memcached_vanilla_mem PRIVATE PROFILE_MEM)

add_library(memcached_orthrus_raw orthrus/closure.cpp)
target_compile_definitions(memcached_orthrus_raw PRIVATE NAMESPACE=raw)

add_library(memcached_orthrus_app orthrus/closure.cpp)
target_compile_definitions(memcached_orthrus_app PRIVATE NAMESPACE=app)

add_library(memcached_orthrus_val orthrus/closure.cpp)
target_compile_definitions(memcached_orthrus_val PRIVATE NAMESPACE=validator)

add_executable(memcached_orthrus orthrus/server.cpp)
target_link_libraries(memcached_orthrus PRIVATE ${LIBS}
    memcached_orthrus_raw
    memcached_orthrus_app
    memcached_orthrus_val
)

add_executable(memcached_orthrus_profile orthrus/server.cpp)
target_link_libraries(memcached_orthrus_profile PRIVATE ${LIBS_profile}
    memcached_orthrus_raw
    memcached_orthrus_app
    memcached_orthrus_val
)
target_compile_definitions(memcached_orthrus_profile PRIVATE PROFILE)

add_executable(memcached_orthrus_mem orthrus/server.cpp)
target_link_libraries(memcached_orthrus_mem PRIVATE ${LIBS}
    memcached_orthrus_raw
    memcached_orthrus_app
    memcached_orthrus_val
    profile_mem
)
target_compile_definitions(memcached_orthrus_mem PRIVATE PROFILE_MEM)

add_executable(memcached_rbv_primary rbv/primary.cpp rbv/hashmap.cpp)
target_link_libraries(memcached_rbv_primary PRIVATE rbv_primary_lib)

add_executable(memcached_rbv_replica rbv/replica.cpp rbv/hashmap.cpp)
target_link_libraries(memcached_rbv_replica PRIVATE rbv_replica_lib)

add_executable(memcached_rbv_primary_mem rbv/primary.cpp rbv/hashmap.cpp)
target_link_libraries(memcached_rbv_primary_mem PRIVATE rbv_primary_lib profile_mem)
target_compile_definitions(memcached_rbv_primary_mem PRIVATE PROFILE_MEM)

add_executable(memcached_rbv_replica_mem rbv/replica.cpp rbv/hashmap.cpp)
target_link_libraries(memcached_rbv_replica_mem PRIVATE rbv_replica_lib profile_mem)
target_compile_definitions(memcached_rbv_replica_mem PRIVATE PROFILE_MEM)

add_executable(memcached_rbv_primary_profile rbv/primary.cpp rbv/hashmap.cpp)
target_link_libraries(memcached_rbv_primary_profile PRIVATE rbv_primary_lib profile)
target_compile_definitions(memcached_rbv_primary_profile PRIVATE PROFILE)

add_executable(memcached_rbv_fj rbv/faultinjection.cpp rbv/hashmap.cpp)
target_link_libraries(memcached_rbv_fj PRIVATE rbv_primary_lib rbv_replica_lib pthread)
target_compile_definitions(memcached_rbv_fj PRIVATE DISABLE_SCEE)