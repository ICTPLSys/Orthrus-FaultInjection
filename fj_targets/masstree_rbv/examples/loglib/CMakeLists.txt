add_executable(single_thread single_thread.cpp)
add_executable(multi_threads multi_threads.cpp)
add_executable(log_unroll log_unroll.cpp)

target_link_libraries(single_thread PRIVATE ${LIBS})
target_link_libraries(multi_threads PRIVATE ${LIBS})
target_link_libraries(log_unroll PRIVATE ${LIBS})

add_subdirectory(new_delete)
