#ifndef EP_GUI_HPP
#define EP_GUI_HPP

namespace EP {
  class Color {
  public:
    float r,g,b,a;//0..1
    Color(const float r,const float g,const float b,const float a=1.0) {
      this->r = std::min(1.0f,std::max(0.0f,r));
      this->g = std::min(1.0f,std::max(0.0f,g));
      this->b = std::min(1.0f,std::max(0.0f,b));
      this->a = std::min(1.0f,std::max(0.0f,a));
    }

    sf::Color toSFML() const {
      return sf::Color(255.0*r,255.0*g,255.0*b,255.0*a);
    }
  private:
  };
  // sf::Color HueToRGB(float hue) {
  //   hue = fmod(hue,1.0);
  //   hue*=6.0;
  //   if (hue<1.0) {
  //     return sf::Color(255.0,255.0*hue,0);
  //   }
  //   hue-=1.0;
  //   if (hue<1.0) {
  //     return sf::Color(255.0-255.0*hue,255.0,0);
  //   }
  //   hue-=1.0;
  //   if (hue<1.0) {
  //     return sf::Color(0,255.0,255.0*hue);
  //   }
  //   hue-=1.0;
  //   if (hue<1.0) {
  //     return sf::Color(0,255.0-255.0*hue,255.0);
  //   }
  //   hue-=1.0;
  //   if (hue<1.0) {
  //     return sf::Color(255.0*hue,0,255.0);
  //   }
  //   hue-=1.0;
  //   if (hue<1.0) {
  //     return sf::Color(255.0,0,255.0-255.0*hue);
  //   }
  //   return sf::Color(255,255,255);
  // }
  //

  sf::Font font;
  bool isFont = false;
  sf::Font& getFont() {
    if (!isFont) {
      if (!font.loadFromFile("arial.ttf")) {
          std::cout << "font could not be loaded!" << std::endl;
      } else {
        isFont = true;
      }
    }
    return font;
  }

  void DrawText(float x, float y, std::string text, float size, sf::RenderTarget &target, const Color& color) {
    sf::Text shape(text, getFont());
    shape.setCharacterSize(size);
    shape.setColor(color.toSFML());
    shape.setPosition(x,y);
    //shape.setStyle(sf::Text::Bold | sf::Text::Underlined);
    target.draw(shape, sf::BlendAdd);
  }

  void DrawRect(float x, float y, float dx, float dy, sf::RenderTarget &target, const Color& color) {
    sf::RectangleShape rectangle;
    rectangle.setSize(sf::Vector2f(dx, dy));
    rectangle.setFillColor(color.toSFML());
    rectangle.setPosition(x, y);
    target.draw(rectangle, sf::BlendAdd);
  }

  // void DrawDot(float x, float y, sf::RenderWindow &window, sf::Color color = sf::Color(255,0,0))
  // {
  //   float r = 3;
  //   sf::CircleShape shape(r);
  //   shape.setFillColor(color);
  //   shape.setPosition(x-r, y-r);
  //   window.draw(shape, sf::BlendAdd);
  // }
  //
  // void DrawLine(float x1, float y1, float x2, float y2, sf::RenderWindow &window)
  // {
  //   sf::Vertex line[] =
  //   {
  //     sf::Vertex(sf::Vector2f(x1,y1), sf::Color(0,0,0)),
  //     sf::Vertex(sf::Vector2f(x2,y2), sf::Color(255,255,255))
  //   };
  //   window.draw(line, 2, sf::Lines, sf::BlendAdd);
  // }
  //




  namespace GUI {

    class Area {
    public:
      Area(Area* const parent, const float x,const float y,const float dx,const float dy)
      : parent_(parent),x_(x),y_(y),dx_(dx),dy_(dy){}

      float dx() const {return dx_;}
      float dy() const {return dy_;}

      void draw(const float px,const float py,const float scale, sf::RenderTarget &target) {
        DrawRect(x_*scale+py, y_*scale+py, dx_*scale, dy_*scale, target, Color(0.5,0.1,0.1));
      }

      void onResizeParent() {
        if (fillParent_&&parent_) { dx_ = parent_->dx(); dy_ = parent_->dy(); }
      }

      Area* sizeIs(const float dx,const float dy) {dx_=dx; dy_=dy; onResizeParent(); return this;}
      Area* fillParentIs(bool const value) { fillParent_ = value; return this;}

    private:
      Area* parent_ = NULL;
      float x_,y_,dx_,dy_; // x,y relative to parent
      bool fillParent_ = false;
    };

  }// namespace GUI
}// namespace EP
#endif //EP_GUI_HPP
