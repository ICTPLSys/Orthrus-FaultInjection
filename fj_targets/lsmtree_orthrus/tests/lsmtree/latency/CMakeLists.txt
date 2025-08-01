# ================ latency vs pXX ==================
function(add_latency_target NAME LIBLSMTREE)
    set(TARGET lsmtree_socket_latency_pXX_${NAME})
    add_executable(${TARGET} socket_latency_pXX_${NAME}.cpp)
    target_link_libraries(${TARGET} PRIVATE
        gflags::gflags
        ${LIBLSMTREE}
        ${LIBS}
    )
endfunction()

add_latency_target(raw      rawlsmtree)
add_latency_target(scee     lsmtree)
add_latency_target(rbv      rawlsmtree)

set(TARGET lsmtree_socket_latency_pXX_client)
add_executable(${TARGET} client.cpp)
target_link_libraries(${TARGET} PRIVATE
    gflags::gflags
    hdr_histogram
    ${LIBS_deps}
)

# ========================================
# function(add_latency_cdf_target NAME)
#     set(TARGET lsmtree_socket_latency_cdf_${NAME})
#     add_executable(${TARGET} socket_latency_pXX_${NAME}.cpp)
#     target_link_libraries(${TARGET} PRIVATE
#         gflags::gflags
#         lsmtree
#         hdr_histogram
#         ${LIBS_profile}
#     )
# endfunction()

# function(add_latency_cdf_target_raw NAME)
#     set(TARGET lsmtree_socket_latency_cdf_${NAME})
#     add_executable(${TARGET} socket_latency_pXX_${NAME}.cpp)
#     target_link_libraries(${TARGET} PRIVATE
#         gflags::gflags
#         rawlsmtree
#         hdr_histogram
#     )
# endfunction()

# add_latency_cdf_target_raw(raw)
# add_latency_cdf_target(scee)
# add_latency_cdf_target_raw(rbv)
