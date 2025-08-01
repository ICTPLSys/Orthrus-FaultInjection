add_library(redis_demo_lib_raw closure.cpp)
target_compile_definitions(redis_demo_lib_raw PRIVATE NAMESPACE=raw)

add_library(redis_demo_lib_app closure.cpp)
target_compile_definitions(redis_demo_lib_app PRIVATE NAMESPACE=app)

add_library(redis_demo_lib_val closure.cpp)
target_compile_definitions(redis_demo_lib_val PRIVATE NAMESPACE=validator)
target_link_libraries(redis_demo_lib_val PRIVATE scee)

add_executable(redis_demo_correctness correctness.cpp)
target_link_libraries(redis_demo_correctness PRIVATE ${LIBS}
    redis_demo_lib_raw
    redis_demo_lib_app
    redis_demo_lib_val
)

add_executable(redis_demo_benchmark benchmark.cpp)
target_link_libraries(redis_demo_benchmark PRIVATE ${LIBS}
    redis_demo_lib_raw
    redis_demo_lib_app
    redis_demo_lib_val
)
