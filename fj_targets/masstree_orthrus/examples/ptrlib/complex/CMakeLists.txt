add_library(ptrlib_complex_raw closure.cpp)
target_compile_definitions(ptrlib_complex_raw PRIVATE NAMESPACE=raw)

add_library(ptrlib_complex_app closure.cpp)
target_compile_definitions(ptrlib_complex_app PRIVATE NAMESPACE=app)

add_library(ptrlib_complex_val closure.cpp)
target_compile_definitions(ptrlib_complex_val PRIVATE NAMESPACE=validator)
target_link_libraries(ptrlib_complex_val PRIVATE scee)

add_executable(ptrlib_complex_demo main.cpp)

target_link_libraries(ptrlib_complex_demo PRIVATE ${LIBS}
    ptrlib_complex_raw
    ptrlib_complex_app
    ptrlib_complex_val
)
