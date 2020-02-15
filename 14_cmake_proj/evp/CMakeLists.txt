set(EVPLIB_SOURCE_FILES
   draw.cpp
   server.cpp
)

set(EVP_HEADER_FILES
   ../include/evp/draw.hpp
   ../include/evp/noise.hpp
   ../include/evp/voronoi.hpp
   ../include/evp/server.hpp
)

add_library(evp_lib ${EVPLIB_SOURCE_FILES} ${EVP_HEADER_FILES})
target_include_directories(evp_lib PUBLIC ../include)

set_target_properties(evp_lib PROPERTIES
   CXX_STANDARD 11
   CXX_STANDARD_REQUIRED YES
   CXX_EXTENSIONS NO
)

