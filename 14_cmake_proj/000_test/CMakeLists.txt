project(000_test)

add_executable(000_test main.cpp)

target_link_libraries(000_test PUBLIC evp_lib)

set_target_properties(000_test PROPERTIES
   CXX_STANDARD 11
   CXX_STANDARD_REQUIRED YES
   CXX_EXTENSIONS NO
)

