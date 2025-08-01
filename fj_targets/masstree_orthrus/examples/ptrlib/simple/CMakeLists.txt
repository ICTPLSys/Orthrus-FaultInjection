add_library(ptrlib_simple_raw closure.cpp)
target_compile_definitions(ptrlib_simple_raw PRIVATE NAMESPACE=raw)

add_library(ptrlib_simple_app closure.cpp)
target_compile_definitions(ptrlib_simple_app PRIVATE NAMESPACE=app)

add_library(ptrlib_simple_val closure.cpp)
target_compile_definitions(ptrlib_simple_val PRIVATE NAMESPACE=validator)
target_link_libraries(ptrlib_simple_val PRIVATE scee)

add_executable(ptrlib_simple_demo main.cpp)

target_link_libraries(ptrlib_simple_demo PRIVATE ${LIBS}
    ptrlib_simple_raw
    ptrlib_simple_app
    ptrlib_simple_val
)
