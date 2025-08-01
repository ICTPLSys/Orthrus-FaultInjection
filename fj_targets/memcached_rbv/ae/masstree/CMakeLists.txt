add_library(masstree_vanilla_lib vanilla/masstree.cpp)

add_executable(masstree_vanilla vanilla/benchmark.cpp)
target_link_libraries(masstree_vanilla PRIVATE ${LIBS}
    masstree_vanilla_lib
)

add_library(masstree_orthrus_raw orthrus/closure.cpp)
target_compile_definitions(masstree_orthrus_raw PRIVATE NAMESPACE=raw)

add_library(masstree_orthrus_app orthrus/closure.cpp)
target_compile_definitions(masstree_orthrus_app PRIVATE NAMESPACE=app)

add_library(masstree_orthrus_val orthrus/closure.cpp)
target_compile_definitions(masstree_orthrus_val PRIVATE NAMESPACE=validator)
target_link_libraries(masstree_orthrus_val PRIVATE scee)

add_executable(masstree_orthrus orthrus/benchmark.cpp)
target_link_libraries(masstree_orthrus PRIVATE ${LIBS}
    masstree_orthrus_raw
    masstree_orthrus_app
    masstree_orthrus_val
)
