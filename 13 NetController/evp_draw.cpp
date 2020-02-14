#include "evp_draw.hpp"


sf::Font&
evp::getFont() {
   if (!isFont) {
     if (!font.loadFromFile("arial.ttf")) {
         std::cout << "font could not be loaded!" << std::endl;
     } else {
       isFont = true;
     }
   }
   return font;
}


void
evp::DrawText(float x, float y, std::string text, float size, sf::RenderTarget &target, const Color& color,float alignX,float alignY) {
    sf::Text shape(text, getFont());
    shape.setCharacterSize(std::floor(size));
    //shape.setColor(color.toSFML());
    shape.setFillColor(color.toSFML());
    sf::FloatRect bounds = shape.getLocalBounds();
    shape.setPosition(std::floor(x-alignX*bounds.width),std::floor(y-alignY*bounds.height));
    //shape.setStyle(sf::Text::Bold | sf::Text::Underlined);
    target.draw(shape, sf::BlendAlpha);//BlendAdd
  }


void
evp::DrawRect(float x, float y, float dx, float dy, sf::RenderTarget &target, const Color& color, float w) {
   sf::RectangleShape rectangle;
   rectangle.setSize(sf::Vector2f(dx, dy));
   rectangle.setFillColor(color.toSFML());
   rectangle.setPosition(x, y);
   rectangle.setRotation(w);
   target.draw(rectangle, sf::BlendAlpha);//BlendAdd
}

void
evp::DrawLine(float x1, float y1, float x2, float y2, sf::RenderTarget &window, const Color& color)
{
  sf::Vertex line[] =
  {
    sf::Vertex(sf::Vector2f(x1,y1), color.toSFML()),
    sf::Vertex(sf::Vector2f(x2,y2), color.toSFML())
  };
  window.draw(line, 2, sf::Lines, sf::BlendAlpha);
}



