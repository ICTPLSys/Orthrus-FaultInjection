add_executable(phoenix_loader loader.cpp)
target_link_libraries(phoenix_loader PRIVATE ${LIBS_disabled} boost_program_options)

add_executable(phoenix_loader_trivial loader_trivial.cpp)
target_link_libraries(phoenix_loader_trivial PRIVATE ${LIBS_disabled} boost_program_options)

add_library(phoenix_vanilla_raw vanilla/phoenix.cpp)
target_compile_definitions(phoenix_vanilla_raw PRIVATE NAMESPACE=raw DISABLE_SCEE)

add_library(phoenix_vanilla_app vanilla/phoenix.cpp)
target_compile_definitions(phoenix_vanilla_app PRIVATE NAMESPACE=app DISABLE_SCEE)

add_library(phoenix_vanilla_val vanilla/phoenix.cpp)
target_compile_definitions(phoenix_vanilla_val PRIVATE NAMESPACE=validator DISABLE_SCEE)

add_executable(phoenix_vanilla vanilla/main.cpp)
target_link_libraries(phoenix_vanilla PRIVATE ${LIBS_disabled}
    phoenix_vanilla_raw
    phoenix_vanilla_app
    phoenix_vanilla_val
    boost_program_options
)
target_compile_definitions(phoenix_vanilla PRIVATE DISABLE_SCEE)

add_executable(phoenix_vanilla_mem vanilla/main.cpp)
target_link_libraries(phoenix_vanilla_mem PRIVATE ${LIBS_disabled}
    phoenix_vanilla_raw
    phoenix_vanilla_app
    phoenix_vanilla_val
    boost_program_options
    profile_mem
)
target_compile_definitions(phoenix_vanilla_mem PRIVATE PROFILE_MEM DISABLE_SCEE)

add_library(phoenix_orthrus_raw orthrus/phoenix.cpp)
target_compile_definitions(phoenix_orthrus_raw PRIVATE NAMESPACE=raw)

add_library(phoenix_orthrus_app orthrus/phoenix.cpp)
target_compile_definitions(phoenix_orthrus_app PRIVATE NAMESPACE=app)

add_library(phoenix_orthrus_val orthrus/phoenix.cpp)
target_compile_definitions(phoenix_orthrus_val PRIVATE NAMESPACE=validator)

add_executable(phoenix_orthrus orthrus/main.cpp)
target_link_libraries(phoenix_orthrus PRIVATE ${LIBS_sampling}
    phoenix_orthrus_raw
    phoenix_orthrus_app
    phoenix_orthrus_val
    boost_program_options
)
target_compile_definitions(phoenix_orthrus PRIVATE SAMPLING)

add_executable(phoenix_orthrus_profile orthrus/main.cpp)
target_link_libraries(phoenix_orthrus_profile PRIVATE ${LIBS_sampling_profile}
    phoenix_orthrus_raw
    phoenix_orthrus_app
    phoenix_orthrus_val
    boost_program_options
)
target_compile_definitions(phoenix_orthrus_profile PRIVATE PROFILE SAMPLING)

add_executable(phoenix_orthrus_mem orthrus/main.cpp)
target_link_libraries(phoenix_orthrus_mem PRIVATE ${LIBS_sampling}
    phoenix_orthrus_raw
    phoenix_orthrus_app
    phoenix_orthrus_val
    boost_program_options
    profile_mem
)
target_compile_definitions(phoenix_orthrus_mem PRIVATE PROFILE_MEM SAMPLING)

add_library(phoenix_rbv_primary_raw rbv/phoenix.cpp)
target_compile_definitions(phoenix_rbv_primary_raw PRIVATE NAMESPACE=raw RBV_PRIMARY)

add_library(phoenix_rbv_primary_app rbv/phoenix.cpp)
target_compile_definitions(phoenix_rbv_primary_app PRIVATE NAMESPACE=app RBV_PRIMARY)

add_library(phoenix_rbv_primary_val rbv/phoenix.cpp)
target_compile_definitions(phoenix_rbv_primary_val PRIVATE NAMESPACE=validator RBV_PRIMARY)

add_library(phoenix_rbv_replica_raw rbv/phoenix.cpp)
target_compile_definitions(phoenix_rbv_replica_raw PRIVATE NAMESPACE=raw)

add_library(phoenix_rbv_replica_app rbv/phoenix.cpp)
target_compile_definitions(phoenix_rbv_replica_app PRIVATE NAMESPACE=app)

add_library(phoenix_rbv_replica_val rbv/phoenix.cpp)
target_compile_definitions(phoenix_rbv_replica_val PRIVATE NAMESPACE=validator)

add_executable(phoenix_rbv_primary rbv/primary.cpp)
target_link_libraries(phoenix_rbv_primary PRIVATE ${LIBS_disabled}
    phoenix_rbv_primary_raw
    phoenix_rbv_primary_app
    phoenix_rbv_primary_val
    boost_program_options
    rbv_primary_lib
)
target_compile_definitions(phoenix_rbv_primary PRIVATE RBV_PRIMARY)

add_executable(phoenix_rbv_primary_mem rbv/primary.cpp)
target_link_libraries(phoenix_rbv_primary_mem PRIVATE ${LIBS_disabled}
    phoenix_rbv_primary_raw
    phoenix_rbv_primary_app
    phoenix_rbv_primary_val
    boost_program_options
    rbv_primary_lib
    profile_mem
)
target_compile_definitions(phoenix_rbv_primary_mem PRIVATE RBV_PRIMARY PROFILE_MEM)

add_executable(phoenix_rbv_replica rbv/replica.cpp)
target_link_libraries(phoenix_rbv_replica PRIVATE ${LIBS_disabled}
    phoenix_rbv_replica_raw
    phoenix_rbv_replica_app
    phoenix_rbv_replica_val
    boost_program_options
    rbv_replica_lib
)

add_executable(phoenix_rbv_replica_mem rbv/replica.cpp)
target_link_libraries(phoenix_rbv_replica_mem PRIVATE ${LIBS_disabled}
    phoenix_rbv_replica_raw
    phoenix_rbv_replica_app
    phoenix_rbv_replica_val
    boost_program_options
    rbv_replica_lib
    profile_mem
)
target_compile_definitions(phoenix_rbv_replica_mem PRIVATE PROFILE_MEM)

add_executable(phoenix_rbv_replica_profile rbv/replica.cpp)
target_link_libraries(phoenix_rbv_replica_profile PRIVATE ${LIBS_disabled_profile}
    phoenix_rbv_replica_raw
    phoenix_rbv_replica_app
    phoenix_rbv_replica_val
    boost_program_options
    rbv_replica_lib
)
target_compile_definitions(phoenix_rbv_replica_profile PRIVATE PROFILE)
