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
   
   static Color hue(float hue) {
      hue = fmod(hue,1.0);
      hue*=6.0;
      if (hue<1.0) {
        return Color(1,hue,0);
      }
      hue-=1.0;
      if (hue<1.0) {
        return Color(1,1-hue,0);
      }
      hue-=1.0;
      if (hue<1.0) {
        return Color(0,1,hue);
      }
      hue-=1.0;
      if (hue<1.0) {
        return Color(0,1-hue,1);
      }
      hue-=1.0;
      if (hue<1.0) {
        return Color(hue,0,1);
      }
      hue-=1.0;
      if (hue<1.0) {
        return Color(1,0,1-hue);
      }
      return Color(1,1,1);
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

void DrawLine(float x1, float y1, float x2, float y2, sf::RenderTarget &window, const Color& color)
{
  sf::Vertex line[] =
  {
    sf::Vertex(sf::Vector2f(x1,y1), color.toSFML()),
    sf::Vertex(sf::Vector2f(x2,y2), color.toSFML())
  };
  window.draw(line, 2, sf::Lines, sf::BlendAlpha);
}



#endif //EVP_DRAW_HPP
