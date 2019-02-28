#ifndef EP_GUI_HPP
#define EP_GUI_HPP

#include <list>

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
    target.draw(shape, sf::BlendAlpha);//BlendAdd
  }

  void DrawRect(float x, float y, float dx, float dy, sf::RenderTarget &target, const Color& color) {
    sf::RectangleShape rectangle;
    rectangle.setSize(sf::Vector2f(dx, dy));
    rectangle.setFillColor(color.toSFML());
    rectangle.setPosition(x, y);
    target.draw(rectangle, sf::BlendAlpha);//BlendAdd
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
    // base: Area
    // Windows,Buttons,expanders(x,y-sliders)

    class Area {
    public:
      Area(const std::string& name,Area* const parent, const float x,const float y,const float dx,const float dy)
      : name_(name),parent_(parent),x_(x),y_(y),dx_(dx),dy_(dy){
        if (parent_) {parent_->childIs(this);}
      }

      float dx() const {return dx_;}
      float dy() const {return dy_;}
      std::string name() const {return name_;}
      std::string fullName() const {if (parent_) {return parent_->fullName()+"/"+name_;}else{return name_;}}

      virtual void draw(const float px,const float py,const float scale, sf::RenderTarget &target) {
        DrawRect(x_*scale+py, y_*scale+py, dx_*scale, dy_*scale, target, Color(0.5,0.1,0.1));
        for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
          (*rit)->draw(x_*scale+py, y_*scale+py,scale,target);
        }
      }

      void onResizeParent(const float dx,const float dy) {
        if (fillParent_&&parent_) { sizeIs(dx,dy); }
      }

      virtual void onMouseOver(const float px,const float py,const float scale) {}// relative to parent
      virtual void onMouseOverStart() {}
      virtual void onMouseOverEnd() {}

      Area* checkMouseOver(const float px,const float py,const float scale,const bool doNotify=true) {// relative to parent
        if (px>=x_*scale and py>=y_*scale and px<=(x_+dx_)*scale and py<=(y_+dy_)*scale) {
          for (auto &c : children_) {
            Area* over = c->checkMouseOver(px-x_*scale,py-y_*scale,scale,doNotify);
            if (over!=NULL) {
              return over;
            }
          }
          if (doNotify) {onMouseOver(px,py,scale);}
          return this;
        }
        return NULL;
      }

      Area* sizeIs(const float dx,const float dy) {
        dx_=dx; dy_=dy;
        for (auto &c : children_) {
          c->onResizeParent(dx,dy);
        }
        return this;
      }

      Area* fillParentIs(bool const value) { fillParent_ = value; return this;}
      Area* childIs(Area* c) {children_.push_back(c); return this;}
    protected:
      std::string name_;
      Area* parent_ = NULL;
      std::list<Area*> children_;
      float x_,y_,dx_,dy_; // x,y relative to parent
      bool fillParent_ = false;
    };// class Area
    class Button : public Area {
    public:
      Button(const std::string& name,Area* const parent, const float x,const float y,const float dx,const float dy, const std::string text,const std::vector<Color> bgColors,const std::vector<Color> textColors)
      : Area(name,parent,x,y,dx,dy),text_(text),bgColors_(bgColors),textColors_(textColors){}

      virtual void draw(const float px,const float py,const float scale, sf::RenderTarget &target) {
        float gx = x_*scale+px;
        float gy = y_*scale+py;
        DrawRect(gx, gy, dx_*scale, dy_*scale, target, bgColors_[state_]);
        DrawText(gx+1*scale, gy+1*scale, text_, (dy_-2)*scale, target, textColors_[state_]);
        for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
          (*rit)->draw(x_*scale+py, y_*scale+py,scale,target);
        }
      }
      virtual void onMouseOver(const float px,const float py,const float scale) {// relative to parent
        if (state_==0) {state_=1;}
      }
      virtual void onMouseOverStart() {if (state_==0) {state_=1;}}
      virtual void onMouseOverEnd() {if (state_==1) {state_=0;}}

    private:
      std::string text_;
      std::vector<Color> bgColors_,textColors_;
      size_t state_=0;//0:normal, 1:mouse over, 2:pressing, 3:clicked
    };
    class Window : public Area {
    public:
      Window(const std::string& name,Area* const parent, const float x,const float y,const float dx,const float dy, const std::string title)
      : Area(name,parent,x,y,dx,dy),title_(title){
        Area* closeButton = new Button("close",this,dx-headerSize,borderSize,headerSize-2*borderSize,headerSize-2*borderSize,"X",
                                        std::vector<Color>{Color(0.5,0,0),Color(0.4,0,0),Color(0.2,0,0),Color(0.2,0,0)},
                                        std::vector<Color>{Color(1,0.5,0.5),Color(1,0.8,0.8),Color(0.6,0.1,0.1),Color(0.5,0,0)}
                                      );
      }

      virtual void draw(const float px,const float py,const float scale, sf::RenderTarget &target) {
        float gx = x_*scale+px;
        float gy = y_*scale+py;
        DrawRect(gx, gy, dx_*scale, dy_*scale, target, Color(0.1,0.5,0.1));
        DrawText(gx+borderSize*scale, gy+borderSize*scale, title_, (headerSize-2*borderSize)*scale, target, Color(1,1,1));
        for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
          (*rit)->draw(x_*scale+py, y_*scale+py,scale,target);
        }
      }

    private:
      std::string title_;
      const float borderSize = 2.0; // sides and bottom
      const float headerSize = 20.0;// top
    };

  }// namespace GUI
}// namespace EP
#endif //EP_GUI_HPP
