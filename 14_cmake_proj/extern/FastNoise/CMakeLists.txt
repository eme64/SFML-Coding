set(FASTNOISE_SOURCE_FILES
   FastNoise.cpp
)

set(FASTNOISE_HEADER_FILES
   FastNoise.h
)

add_library(fastnoise_lib ${FASTNOISE_SOURCE_FILES} ${FASTNOISE_HEADER_FILES})
target_include_directories(fastnoise_lib PUBLIC .)

set_target_properties(fastnoise_lib PROPERTIES
   CXX_STANDARD 11
   CXX_STANDARD_REQUIRED YES
   CXX_EXTENSIONS NO
)

