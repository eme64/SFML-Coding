#include <SFML/Graphics.hpp>

#ifndef EVP_DRAW_HPP
#define EVP_DRAW_HPP

class Color {
public:
   float r,g,b,a;//0..1
   Color(const float r,const float g,const float b,const float a=1.0) {
     this->r = std::min(1.0f,std::max(0.0f,r));
     this->g = std::min(1.0f,std::max(0.0f,g));
     this->b = std::min(1.0f,std::max(0.0f,b));
     this->a = std::min(1.0f,std::max(0.0f,a));
   }
   Color operator*(const float scale) {return Color(scale*r,scale*g,scale*b,a);}
 
   sf::Color toSFML() const {
     return sf::Color(255.0*r,255.0*g,255.0*b,255.0*a);
   }
protected:
};


void DrawRect(float x, float y, float dx, float dy, sf::RenderTarget &target, const Color& color, float w=0.0) {
   sf::RectangleShape rectangle;
   rectangle.setSize(sf::Vector2f(dx, dy));
   rectangle.setFillColor(color.toSFML());
   rectangle.setPosition(x, y);
   rectangle.setRotation(w);
   target.draw(rectangle, sf::BlendAlpha);//BlendAdd
}



#endif //EVP_DRAW_HPP
