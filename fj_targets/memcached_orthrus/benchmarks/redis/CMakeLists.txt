add_library(redis_lib_raw closure.cpp)
target_compile_definitions(redis_lib_raw PRIVATE NAMESPACE=raw)

add_library(redis_lib_app closure.cpp)
target_compile_definitions(redis_lib_app PRIVATE NAMESPACE=app)

add_library(redis_lib_val closure.cpp)
target_compile_definitions(redis_lib_val PRIVATE NAMESPACE=validator)
target_link_libraries(redis_lib_val PRIVATE scee)

add_executable(redis_benchmark benchmark.cpp)
target_link_libraries(redis_benchmark PRIVATE ${LIBS}
    redis_lib_raw
    redis_lib_app
    redis_lib_val
)

add_executable(redis_client client.cpp)
target_link_libraries(redis_client PRIVATE pthread)

add_executable(redis_server server.cpp)
target_link_libraries(redis_server PRIVATE ${LIBS}
    redis_lib_raw
    redis_lib_app
    redis_lib_val
)

add_library(redis_lib_raw_nocrc closure.cpp)
target_compile_definitions(redis_lib_raw_nocrc PRIVATE NAMESPACE=raw NO_CRC)

add_library(redis_lib_app_nocrc closure.cpp)
target_compile_definitions(redis_lib_app_nocrc PRIVATE NAMESPACE=app NO_CRC)

add_library(redis_lib_val_nocrc closure.cpp)
target_compile_definitions(redis_lib_val_nocrc PRIVATE NAMESPACE=validator NO_CRC)
target_link_libraries(redis_lib_val_nocrc PRIVATE scee)

add_executable(redis_server_nocrc server.cpp)
target_link_libraries(redis_server_nocrc PRIVATE ${LIBS}
    redis_lib_raw_nocrc
    redis_lib_app_nocrc
    redis_lib_val_nocrc
)
target_compile_definitions(redis_server_nocrc PRIVATE NO_CRC)

add_library(redis_pure_lib pure/hashmap.cpp)
add_executable(redis_pure_server pure/server.cpp)
target_link_libraries(redis_pure_server PRIVATE ${LIBS}
    redis_pure_lib
)

add_executable(redis_faultinject faultinject.cpp)
target_link_libraries(redis_faultinject PRIVATE ${LIBS}
    redis_lib_raw
    redis_lib_app
    redis_lib_val
)