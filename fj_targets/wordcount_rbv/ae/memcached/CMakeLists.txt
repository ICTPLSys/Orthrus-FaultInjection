add_executable(memcached_client client.cpp)
target_link_libraries(memcached_client PRIVATE pthread)

add_library(memcached_vanilla_lib vanilla/hashmap.cpp)
add_executable(memcached_vanilla vanilla/server.cpp)
target_link_libraries(memcached_vanilla PRIVATE ${LIBS}
    memcached_vanilla_lib
)
# add_executable(memcached_vanilla_profile vanilla/server.cpp)
# target_link_libraries(memcached_vanilla_profile PRIVATE ${LIBS}
#     memcached_vanilla_lib
# )
# target_compile_definitions(memcached_vanilla_profile PRIVATE WITH_MONITOR)

# add_library(memcached_rbv_lib rbv/hashmap.cpp)
# add_executable(memcached_rbv_primary rbv/server_rbv.cpp)
# target_link_libraries(memcached_rbv_primary PRIVATE ${LIBS}
#     memcached_rbv_lib
# )
# target_compile_definitions(memcached_rbv_primary PRIVATE RBV_PRIMARY)
# add_executable(memcached_rbv_replica rbv/server_rbv.cpp)
# target_link_libraries(memcached_rbv_replica PRIVATE ${LIBS}
#     memcached_rbv_lib
# )

add_library(memcached_orthrus_raw orthrus/closure.cpp)
target_compile_definitions(memcached_orthrus_raw PRIVATE NAMESPACE=raw)

add_library(memcached_orthrus_app orthrus/closure.cpp)
target_compile_definitions(memcached_orthrus_app PRIVATE NAMESPACE=app)

add_library(memcached_orthrus_val orthrus/closure.cpp)
target_compile_definitions(memcached_orthrus_val PRIVATE NAMESPACE=validator)
target_link_libraries(memcached_orthrus_val PRIVATE scee)

add_executable(memcached_orthrus orthrus/server.cpp)
target_link_libraries(memcached_orthrus PRIVATE ${LIBS}
    memcached_orthrus_raw
    memcached_orthrus_app
    memcached_orthrus_val
)

add_executable(memcached_orthrus_profile orthrus/server.cpp)
target_link_libraries(memcached_orthrus_profile PRIVATE ${LIBS}
    memcached_orthrus_raw
    memcached_orthrus_app
    memcached_orthrus_val
)
target_compile_definitions(memcached_orthrus_profile PRIVATE PROFILE)