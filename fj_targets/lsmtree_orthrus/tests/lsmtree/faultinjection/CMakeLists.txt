set(TARGET lsmtree_fj_scee)
add_executable(${TARGET} lsmtree_fj_scee.cpp)
target_link_libraries(${TARGET} PRIVATE lsmtree ${LIBS})

set(TARGET lsmtree_fj_rbv)
add_executable(${TARGET} lsmtree_fj_rbv.cpp)
target_link_libraries(${TARGET} PRIVATE rawlsmtree ${LIBS})
