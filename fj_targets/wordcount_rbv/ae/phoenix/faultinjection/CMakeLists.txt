# ================= FJ Target ===================
add_library(phoenix_rbv_fj_app rbv/phoenix.cpp)
target_include_directories(phoenix_rbv_fj_app PRIVATE ../)
target_compile_definitions(phoenix_rbv_fj_app PRIVATE NAMESPACE=app DISABLE_SCEE)

add_library(phoenix_rbv_fj_validator rbv/phoenix.cpp)
target_include_directories(phoenix_rbv_fj_validator PRIVATE ../)
target_compile_definitions(phoenix_rbv_fj_validator PRIVATE NAMESPACE=validator DISABLE_SCEE)

add_executable(phoenix_rbv_fj rbv/main.cpp)
target_include_directories(phoenix_rbv_fj PRIVATE ../)
target_compile_definitions(phoenix_rbv_fj PRIVATE DISABLE_SCEE)
target_link_libraries(phoenix_rbv_fj PRIVATE ${LIBS_disabled}
    phoenix_rbv_fj_app
    phoenix_rbv_fj_validator
    boost_program_options
)
