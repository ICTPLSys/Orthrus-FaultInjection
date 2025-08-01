add_library(masstree_lib_raw closure.cpp)
target_compile_definitions(masstree_lib_raw PRIVATE NAMESPACE=raw)

add_library(masstree_lib_app closure.cpp)
target_compile_definitions(masstree_lib_app PRIVATE NAMESPACE=app)

add_library(masstree_lib_val closure.cpp)
target_compile_definitions(masstree_lib_val PRIVATE NAMESPACE=validator)
target_link_libraries(masstree_lib_val PRIVATE scee)

# add_executable(masstree_correctness multithread.cpp)
# target_link_libraries(masstree_correctness PRIVATE ${LIBS} masstree_lib_raw masstree_lib_app masstree_lib_val)

# add_executable(masstree_benchmark benchmark.cpp)
# target_link_libraries(masstree_benchmark PRIVATE ${LIBS} masstree_lib_raw masstree_lib_app masstree_lib_val)

add_executable(masstree_pure pure/multithread.cpp pure/masstree.cpp)
target_link_libraries(masstree_pure PRIVATE ${LIBS})

# add_executable(masstree_faultinject faultinject.cpp)
# target_link_libraries(masstree_faultinject PRIVATE ${LIBS} masstree_lib_raw masstree_lib_app masstree_lib_val)

add_executable(masstree_testbench testbench.cpp)
target_link_libraries(masstree_testbench PRIVATE ${LIBS} masstree_lib_raw masstree_lib_app masstree_lib_val)

add_executable(masstree_testbench_profile testbench.cpp)
target_link_libraries(masstree_testbench_profile PRIVATE ${LIBS} masstree_lib_raw masstree_lib_app masstree_lib_val)
target_compile_definitions(masstree_testbench_profile PRIVATE PROFILE_IT)

add_executable(masstree_testbench_pure pure/testbench.cpp pure/masstree.cpp)
target_link_libraries(masstree_testbench_pure PRIVATE ${LIBS})
