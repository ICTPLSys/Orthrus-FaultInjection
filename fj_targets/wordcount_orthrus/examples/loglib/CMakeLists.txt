add_executable(single_thread single_thread.cpp)
add_executable(multi_threads multi_threads.cpp)

target_link_libraries(single_thread PRIVATE ${LIBS})
target_link_libraries(multi_threads PRIVATE ${LIBS})

add_subdirectory(new_delete)
