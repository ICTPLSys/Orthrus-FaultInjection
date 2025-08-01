add_library(wc_lib_raw wc.cpp)
target_compile_definitions(wc_lib_raw PRIVATE NAMESPACE=raw)

add_library(wc_lib_app wc.cpp)
target_compile_definitions(wc_lib_app PRIVATE NAMESPACE=app)

add_library(wc_lib_val wc.cpp)
target_compile_definitions(wc_lib_val PRIVATE NAMESPACE=validator)

# add_executable(test_wc test_wc.cpp)
# target_link_libraries(test_wc PRIVATE ${LIBS}
#     wc_lib_raw
#     wc_lib_app
#     wc_lib_val
# )

add_executable(wc main.cpp)
target_link_libraries(wc PRIVATE ${LIBS}
    wc_lib_raw
    wc_lib_app
    wc_lib_val
    boost_program_options
)
