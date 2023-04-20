
#ifndef EVP_DRAW_HPP
#define EVP_DRAW_HPP

sf::Font FONT;

sf::Color HueToRGB(float hue) {
  hue = fmod(fmod(hue,1.0)+1.0,1.0);
  hue*=6.0;
  if (hue<1.0) {
    return sf::Color(255.0,255.0*hue,0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(255.0-255.0*hue,255.0,0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(0,255.0,255.0*hue);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(0,255.0-255.0*hue,255.0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(255.0*hue,0,255.0);
  }
  hue-=1.0;
  return sf::Color(255.0,0,255.0-255.0*hue);
}

sf::Color heat(float h) {
  if (h >= 0) {
    return sf::Color(255.0*h,0,0);
  } else {
    return sf::Color(0,-255.0*h,-255.0*h);
  }
}

sf::Color heat2(float h) {
  if (h >= 0) {
    if (h <= 0.1) {
      h *= 10.0;
      return sf::Color(255.0*h,0,255.0*h);
    } else if (h <= 0.3) {
      h = (h-0.1) * 5.0;
      return sf::Color(255.0,0,255.0 - 255.0*h);
    } else {
      h = (h-0.3) * (1.0 / 0.7);
      return sf::Color(255.0,255.0*h,0);
    }
  } else {
    h = -h;
    if (h <= 0.1) {
      h *= 10.0;
      return sf::Color(0,0,255.0*h);
    } else if (h <= 0.3) {
      h = (h-0.1) * 5.0;
      return sf::Color(0,255.0*h,255.0);
    } else {
      h = (h-0.3) * (1.0 / 0.7);
      return sf::Color(0,255.0,255.0-255.0*h);
    }
  }
}

void DrawDot(float x, float y, float r, sf::RenderWindow &window, sf::Color color = sf::Color(255,0,0))
{
  sf::CircleShape shape(r);
  shape.setFillColor(color);
  shape.setPosition(x-r, y-r);
  window.draw(shape, sf::BlendAdd);
}

void DrawLine(float x1, float y1, float x2, float y2, sf::RenderWindow &window)
{
  sf::Vertex line[] =
  {
    sf::Vertex(sf::Vector2f(x1,y1), sf::Color(0,0,0)),
    sf::Vertex(sf::Vector2f(x2,y2), sf::Color(255,255,255))
  };
  window.draw(line, 2, sf::Lines, sf::BlendAdd);
}

void DrawRect(float x, float y, float dx, float dy, sf::RenderWindow &window, sf::Color color)
{
  sf::RectangleShape rectangle;
  rectangle.setSize(sf::Vector2f(dx, dy));
  rectangle.setFillColor(color);
  rectangle.setPosition(x, y);
  window.draw(rectangle, sf::BlendAdd);
}

void DrawText(float x, float y, std::string text, float size, sf::RenderWindow &window, sf::Color color = sf::Color(255,255,255))
{
  sf::Text shape(text, FONT);//(text, FONT);
  //shape.setFont(FONT);
  //shape.setString(text); //std::to_string(input[i])
  shape.setCharacterSize(size);
  shape.setColor(color);
  shape.setPosition(x,y);
  //shape.setStyle(sf::Text::Bold | sf::Text::Underlined);
  window.draw(shape, sf::BlendAdd);
}



#endif //EVP_DRAW_HPP

