# add_library(mt19937 ${CMAKE_SOURCE_DIR}/third/mt19937/mersenne-twister.cpp)
# target_include_directories(mt19937 PUBLIC ${CMAKE_SOURCE_DIR}/third/mt19937)
set(MT19937_src ${CMAKE_SOURCE_DIR}/third/mt19937/mersenne-twister.cpp)
set(MT19937_include ${CMAKE_SOURCE_DIR}/third/mt19937/)

function(create_lsmtree_library NAMESPACE)
    set(TARGET lsmtree_${NAMESPACE})
    add_library(${TARGET} lsmtree.cpp memtable.cpp runtime.cpp ${MT19937_src})
    target_include_directories(${TARGET} PUBLIC ${MT19937_include})
    target_include_directories(${TARGET} PUBLIC include)
    target_compile_definitions(${TARGET} PRIVATE NAMESPACE=${NAMESPACE})
    target_compile_definitions(${TARGET} PRIVATE
        LSMTREE_PROFILE_SKIPLIST_RDTSC=false
    )
endfunction()

create_lsmtree_library(raw)
create_lsmtree_library(app)
create_lsmtree_library(validator)

add_library(lsmtree INTERFACE)
target_link_libraries(lsmtree INTERFACE lsmtree_raw lsmtree_app lsmtree_validator ${LIBS})

# ===============================================

function(create_lsmtree_library_stat NAMESPACE)
    set(TARGET lsmtree_${NAMESPACE}_stat)
    add_library(${TARGET} lsmtree.cpp memtable.cpp runtime.cpp ${MT19937_src})
    target_include_directories(${TARGET} PUBLIC ${MT19937_include})
    target_include_directories(${TARGET} PUBLIC include)
    target_compile_definitions(${TARGET} PRIVATE NAMESPACE=${NAMESPACE})
    target_compile_definitions(${TARGET} PRIVATE
        LSMTREE_PROFILE_SKIPLIST_RDTSC=true
    )
endfunction()

create_lsmtree_library_stat(raw)
create_lsmtree_library_stat(app)
create_lsmtree_library_stat(validator)

add_library(lsmtree-stat INTERFACE)
target_link_libraries(lsmtree-stat INTERFACE lsmtree_raw_stat lsmtree_app_stat lsmtree_validator_stat ${LIBS})
