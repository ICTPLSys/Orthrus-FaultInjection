# create_lsmtree_library(raw)  # realraw
set(TARGET rawlsmtree)
add_library(${TARGET} lsmtree.cpp memtable.cpp runtime.cpp)
target_include_directories(${TARGET} PUBLIC include)
target_compile_definitions(${TARGET} PRIVATE NAMESPACE=raw)
target_compile_definitions(${TARGET} PRIVATE
    LSMTREE_PROFILE_SKIPLIST_RDTSC=false
)
