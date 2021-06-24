add_compile_options(-std=c++14)

set(CMAKE_CXX_FLAGS "-Wno-write-strings -Wno-literal-suffix")
add_definitions(-DDEBUG_IS_ON=1)

list(APPEND PROTO_SRCS "")
list(APPEND PROTO_HDRS "")

add_library(LIB_GTPV2C
    3gpp_29.274.cpp
    gtpv2c.cpp
    )
    
target_link_libraries(LIB_GTPV2C
    COMMON MAGMA_CONFIG LIB_BSTR
    )

target_include_directories(LIB_GTPV2C PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )