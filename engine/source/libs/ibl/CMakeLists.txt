cmake_minimum_required(VERSION 3.19)
project(cmgen)

set(TARGET ibl)
set(PUBLIC_HDR_DIR include)

# ==================================================================================================
# Sources and headers
# ==================================================================================================
set(PUBLIC_HDRS
    ${PUBLIC_HDR_DIR}/ibl/Cubemap.h
    ${PUBLIC_HDR_DIR}/ibl/CubemapIBL.h
    ${PUBLIC_HDR_DIR}/ibl/CubemapSH.h
    ${PUBLIC_HDR_DIR}/ibl/CubemapUtils.h
    ${PUBLIC_HDR_DIR}/ibl/Image.h
    ${PUBLIC_HDR_DIR}/ibl/utilities.h
)

set(PRIVATE_HDRS
    src/CubemapUtilsImpl.h
)

set(SRCS
    src/Cubemap.cpp
    src/CubemapIBL.cpp
    src/CubemapSH.cpp
    src/CubemapUtils.cpp
    src/Image.cpp
)

# ==================================================================================================
# Target definitions
# ==================================================================================================
include_directories(${PUBLIC_HDR_DIR})
include_directories(${CMAKE_SOURCE_DIR}/libs/math)
include_directories(${CMAKE_SOURCE_DIR}/libs)

add_library(${TARGET} STATIC ${PUBLIC_HDRS} ${PRIVATE_HDRS} ${SRCS})
target_include_directories(${TARGET} PUBLIC ${PUBLIC_HDR_DIR})
set_target_properties(${TARGET} PROPERTIES FOLDER Libs)

## 正确链接math库和Async++库
#target_link_libraries(${TARGET} math Async++)
#
## 添加包含目录以支持async++
#target_include_directories(${TARGET} PRIVATE 
#    ${CMAKE_SOURCE_DIR}/libs/asyncplusplus/include
#)

# 链接需要的库
target_link_libraries(${TARGET} 
    basisu_encoder
    image
    imageio
    math
    utils
)

# ==================================================================================================
# Compile options and optimizations
# ==================================================================================================
if (MSVC)
    target_compile_options(${TARGET} PRIVATE /fp:fast)
else()
    target_compile_options(${TARGET} PRIVATE -ffast-math -fno-finite-math-only)
endif()

# ==================================================================================================
# Installation
# ==================================================================================================
install(TARGETS ${TARGET} ARCHIVE DESTINATION lib)
install(DIRECTORY ${PUBLIC_HDR_DIR}/ibl DESTINATION include)