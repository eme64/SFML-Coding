#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <cassert>
#include <string>


#ifndef EVP_GEOMETRY_HPP
#define EVP_GEOMETRY_HPP



namespace evp {
namespace geom {

template <typename FP>
struct Point {
   FP x,y;
   Point(FP x, FP y) : x(x),y(y) {}
   std::string to_string() {
      return "("+std::to_string(x)+","+std::to_string(y)+")";
   }
};

template <typename FP>
struct Segment {
   typedef Point<FP> P;
   P p0,p1;
   Segment(P p0, P p1) : p0(p0), p1(p1) {}
   std::string to_string() {
      return "["+p0.to_string()+","+p1.to_string()+"]";
   }
};

template <typename S>
void intersect()

} // namespace geom
} // namespace evp

#endif // EVP_GEOMETRY_HPP
