
include_directories(${S1AP_C_DIR})
include_directories(${PROJECT_SOURCE_DIR}/lib/gtpv2c)

add_library(TASK_S11
    s11.cpp
    )

target_link_libraries(TASK_S11
    COMMON
    LIB_BSTR LIB_HASHTABLE LIB_SGS_CLIENT LIB_GTPV2C TASK_MME_APP TASK_UDP
    ${PROTOBUF_LIBRARIES}
    )