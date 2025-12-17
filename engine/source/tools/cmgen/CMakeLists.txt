cmake_minimum_required(VERSION 3.19)
project(cmgen)

set(TARGET cmgen)

# ==================================================================================================
# Sources and headers
# ==================================================================================================
set(HDRS
        src/ProgressUpdater.h
)

set(SRCS
    src/cmgen.cpp
        src/ProgressUpdater.cpp
)

# ==================================================================================================
# Target definitions
# ==================================================================================================
add_executable(${TARGET} ${HDRS} ${SRCS})

target_link_libraries(${TARGET} PRIVATE ibl imageio getopt math utils image basisu_encoder)
set_target_properties(${TARGET} PROPERTIES FOLDER Tools)

# ==================================================================================================
# Compile options and optimizations
# ==================================================================================================
if (MSVC)
    target_compile_options(${TARGET} PRIVATE /fp:fast)
else()
    target_compile_options(${TARGET} PRIVATE -ffast-math -fno-finite-math-only)
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0 /Zc:__cplusplus")
endif()

# ==================================================================================================
# Installation
# ==================================================================================================
install(TARGETS ${TARGET} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)

# 创建bin/tools目录（如果不存在）
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
