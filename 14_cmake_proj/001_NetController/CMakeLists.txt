project(001_NetController)

set(NC_HEADER_FILES
   rocket_room.hpp
   space_gen.hpp
   voronoi_room.hpp
)

add_executable(001_NetController main.cpp ${NC_HEADER_FILES})

target_link_libraries(001_NetController PUBLIC evp_lib fastnoise_lib)
target_link_libraries(001_NetController PUBLIC sfml-graphics sfml-window sfml-system sfml-network pthread)
target_link_libraries(001_NetController PUBLIC box2d)

file(COPY RoomServer DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY arial.ttf DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(001_NetController PROPERTIES
   CXX_STANDARD 11
   CXX_STANDARD_REQUIRED YES
   CXX_EXTENSIONS NO
)

