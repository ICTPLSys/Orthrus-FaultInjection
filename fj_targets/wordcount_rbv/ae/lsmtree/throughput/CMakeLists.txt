# add_executable(lsmtree_throughput_raw lsmtree_throughput_raw.cpp)
# target_link_libraries(lsmtree_throughput_raw PRIVATE lsmtree gflags::gflags ${LIBS_nocheck})

# add_executable(lsmtree_throughput_scee lsmtree_throughput_scee.cpp)
# target_link_libraries(lsmtree_throughput_scee PRIVATE lsmtree gflags::gflags ${LIBS})

# add_executable(lsmtree_throughput_rbv lsmtree_throughput_rbv.cpp)
# target_link_libraries(lsmtree_throughput_rbv PRIVATE lsmtree gflags::gflags ${LIBS_nocheck})

# add_executable(lsmtree_throughput_stats lsmtree_throughput_stats.cpp)
# target_link_libraries(lsmtree_throughput_stats PRIVATE lsmtree-stat gflags::gflags ${LIBS_profile})

# ================== throughput ============================================
function(add_throughput_target NAME LIBLSMTREE)
    set(TARGET lsmtree_socket_throughput_${NAME})
    add_executable(${TARGET} socket_throughput_${NAME}.cpp)
    target_link_libraries(${TARGET} PRIVATE
        gflags::gflags
        ${LIBLSMTREE}
        hdr_histogram::hdr_histogram_static
        profile_mem
        ${LIBS_profile}
    )
endfunction()

add_throughput_target(raw      rawlsmtree)
add_throughput_target(scee     lsmtree)
add_throughput_target(rbv      rawlsmtree)

# ================== validation latency =====================================
function(add_validation_latency_cdf_target NAME LIBLSMTREE)
    set(TARGET lsmtree_socket_val_latency_cdf_${NAME})
    add_executable(${TARGET} socket_throughput_${NAME}.cpp)
    target_compile_definitions(${TARGET} PRIVATE ENABLE_PROFILE_VAL_CDF=true)
    target_link_libraries(${TARGET} PRIVATE
        gflags::gflags
        ${LIBLSMTREE}
        hdr_histogram::hdr_histogram_static
        profile_mem
        ${LIBS_profile}
    )
endfunction()

add_validation_latency_cdf_target(raw      rawlsmtree)
add_validation_latency_cdf_target(scee     lsmtree)
add_validation_latency_cdf_target(rbv      rawlsmtree)

# ================= profile memory usage =====================================
function(add_profile_mem_target NAME LIBLSMTREE)
    set(TARGET lsmtree_socket_profile_mem_${NAME})
    add_executable(${TARGET} socket_throughput_${NAME}.cpp)
    target_compile_definitions(${TARGET} PRIVATE ENABLE_PROFILE_MEM=true)
    target_link_libraries(${TARGET} PRIVATE
        gflags::gflags
        hdr_histogram::hdr_histogram_static
        profile_mem
        ${LIBS_profile}
        ${LIBLSMTREE}
    )
endfunction()

add_profile_mem_target(raw      rawlsmtree)
add_profile_mem_target(scee     lsmtree)
add_profile_mem_target(rbv      rawlsmtree)


# ========================== simple client =====================================
set(TARGET lsmtree_socket_throughput_client)
add_executable(${TARGET} client.cpp)
target_link_libraries(${TARGET} PRIVATE
    gflags::gflags
    hdr_histogram
    ${LIBS_deps}
)
