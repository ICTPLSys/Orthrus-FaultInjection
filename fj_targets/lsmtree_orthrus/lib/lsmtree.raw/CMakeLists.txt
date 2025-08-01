set(MT19937_src ${CMAKE_SOURCE_DIR}/third/mt19937/mersenne-twister.cpp)
set(MT19937_include ${CMAKE_SOURCE_DIR}/third/mt19937/)

# create_lsmtree_library(raw)  # realraw
# set(TARGET rawlsmtree)
# add_library(${TARGET} lsmtree.cpp memtable.cpp runtime.cpp ${MT19937_src})
# target_include_directories(${TARGET} PUBLIC include  ${MT19937_include})
# target_compile_definitions(${TARGET} PRIVATE NAMESPACE=raw)
# target_compile_definitions(${TARGET} PRIVATE
#     LSMTREE_PROFILE_SKIPLIST_RDTSC=false
# )

set(TARGET rawlsmtree_app)
add_library(${TARGET} lsmtree.cpp memtable.cpp runtime.cpp ${MT19937_src})
target_include_directories(${TARGET} PUBLIC include  ${MT19937_include})
target_compile_definitions(${TARGET} PRIVATE NAMESPACE=app)
target_compile_definitions(${TARGET} PRIVATE
    LSMTREE_PROFILE_SKIPLIST_RDTSC=false
)

set(TARGET rawlsmtree_val)
add_library(${TARGET} lsmtree.cpp memtable.cpp runtime.cpp ${MT19937_src})
target_include_directories(${TARGET} PUBLIC include  ${MT19937_include})
target_compile_definitions(${TARGET} PRIVATE NAMESPACE=validator)
target_compile_definitions(${TARGET} PRIVATE
    LSMTREE_PROFILE_SKIPLIST_RDTSC=false
)

add_library(rawlsmtree INTERFACE)
target_link_libraries(rawlsmtree INTERFACE rawlsmtree_app rawlsmtree_val ${LIBS})
