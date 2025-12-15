cmake_minimum_required(VERSION 3.19)
project(imageio)

set(TARGET imageio)
set(PUBLIC_HDR_DIR include)

# ==================================================================================================
# Sources and headers
# ==================================================================================================
set(PUBLIC_HDRS
        include/imageio/BasisEncoder.h
        include/imageio/HDRDecoder.h
        include/imageio/ImageDecoder.h
        include/imageio/ImageDiffer.h
        include/imageio/ImageEncoder.h
)

set(SRCS
        src/BasisEncoder.cpp
        src/HDRDecoder.cpp
        src/ImageDecoder.cpp
        src/ImageDiffer.cpp
        src/ImageEncoder.cpp
)

# ==================================================================================================
# Include and target definitions
# ==================================================================================================
include_directories(${PUBLIC_HDR_DIR})

add_library(${TARGET} STATIC ${PUBLIC_HDRS} ${SRCS})

target_include_directories(${TARGET} PUBLIC ${PUBLIC_HDR_DIR})
set_target_properties(${TARGET} PROPERTIES FOLDER Libs)

target_link_libraries(${TARGET} PUBLIC image math png_static tinyexr basisu_encoder utils)

target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/libs/basis_universal-before_xuastc/encoder)

target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/libs/lpng1653)
target_include_directories(${TARGET} PRIVATE ${CMAKE_BINARY_DIR}/cmake/libs/lpng1653)
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/libs/tinyexr-1.0.12)
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/libs/utils/include)