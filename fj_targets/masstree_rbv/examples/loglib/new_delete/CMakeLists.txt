add_library(new_delete_test_fn_app test_fn.cpp)
target_compile_definitions(new_delete_test_fn_app PRIVATE app_namespace=app)
target_link_libraries(new_delete_test_fn_app PRIVATE scee)

add_library(new_delete_test_fn_val test_fn.cpp)
target_compile_definitions(new_delete_test_fn_val PRIVATE app_namespace=validator)
target_link_libraries(new_delete_test_fn_val PRIVATE scee)

add_executable(new_delete demo.cpp)

target_link_libraries(new_delete PRIVATE ${LIBS}
    new_delete_test_fn_app
    new_delete_test_fn_val
)
