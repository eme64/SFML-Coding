#ifndef EP_GUI_HPP
#define EP_GUI_HPP
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <list>
#include <functional>


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

    Color operator*(const float scale) {return Color(scale*r,scale*g,scale*b,scale*a);}

    sf::Color toSFML() const {
      return sf::Color(255.0*r,255.0*g,255.0*b,255.0*a);
    }
  protected:
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
  void DrawOval(float x, float y, float dx, float dy, sf::RenderTarget &target, const Color& color) {
    sf::CircleShape shape(1);
    shape.setScale(dx*0.5,dy*0.5);
    shape.setFillColor(color.toSFML());
    shape.setPosition(x, y);
    target.draw(shape, sf::BlendAlpha);
  }
  void DrawLine(float x1, float y1, float x2, float y2, sf::RenderTarget &target, const Color& color,float width=1) {
    const float dist = std::sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
    const float rad = std::atan2(y1-y2,x1-x2)+0.5*M_PI;
    const float angle = rad*180.0/M_PI;
    sf::RectangleShape rectangle;
    rectangle.setSize(sf::Vector2f(width, dist));
    rectangle.setFillColor(color.toSFML());
    rectangle.setPosition(x1-std::cos(rad)*width*0.5, y1-std::sin(rad)*width*0.5);
    rectangle.setRotation(angle);
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
      float globalX() const {return x_+(parent_?parent_->globalX():0);}
      float globalY() const {return y_+(parent_?parent_->globalY():0);}
      std::string name() const {return name_;}
      std::string fullName() const {if (parent_) {return parent_->fullName()+"/"+name_;}else{return name_;}}

      virtual void draw(const float px,const float py, sf::RenderTarget &target) {
        DrawRect(x_+px, y_+py, dx_, dy_, target, bgColor_);
        for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
          (*rit)->draw(x_+px, y_+py,target);
        }
      }

      virtual void childSize(float &cdx, float &cdy) {
        cdx = dx_;
        cdy = dy_;
      }
      virtual void childOffset(float &cx, float &cy) {
        cx = 0;
        cy = 0;
      }

      void onResizeParent() {
        if (fillParent_&&parent_) {
          float cdx,cdy;
          parent_->childSize(cdx,cdy);
          sizeIs(cdx,cdy);
        }
      }

      virtual void onMouseOverStart() {}
      virtual void onMouseOver(const float px,const float py) {}// relative to parent
      virtual void onMouseOverEnd() {}

      virtual bool onMouseDownStart(const bool isFirstDown,const float x,const float y) {
        std::cout << "onMouseDownStart " << fullName() << " fist:" << std::to_string(isFirstDown) << std::endl;
        if (isFirstDown) {setFocus();}
        return false;
      }
      //isFirstDown: true if just mouseDown, false if slided from other area that did not lock drag.
      // return: capture drag. can only capture if isFreshDown==true

      virtual bool onMouseDown(const bool isCaptured,const float x,const float y,float &dx, float &dy,Area* const over) {return true;}
      // px,py: relative to parent, position of mouse in last frame.
      //dx/dy the moving the mouse is trying to do. If change (eg =0), mouse position will be changed accordingly.
      //return true: keep drag, false: chain mouse

      virtual void onMouseDownEnd(const bool isCaptured, const bool isLastDown,const float x,const float y,Area* const over) {
        std::cout << "onMouseDownEnd " << fullName() << " captured:" << std::to_string(isCaptured) << " last:" << std::to_string(isLastDown) << std::endl;
      }
      // isLastDown: if true indicates that mouse physically released, else only left scope bc not captured

      virtual Area* checkMouseOver(const float px,const float py,const bool doNotify=true) {// relative to parent
        if (px>=x_ and py>=y_ and px<=(x_+dx_) and py<=(y_+dy_)) {
          for (auto &c : children_) {
            Area* over = c->checkMouseOver(px-x_,py-y_,doNotify);
            if (over!=NULL) {
              return over;
            }
          }
          if (doNotify) {onMouseOver(px,py);}
          return this;
        }
        return NULL;
      }

      virtual void onKeyPressed(const sf::Keyboard::Key keyCode) {
        std::cout << "onKeyPressed " << fullName() << " focus:" << std::to_string(keyCode) << std::endl;
      }

      Area* sizeIs(const float dx,const float dy) {
        const float dxOld = dx_;
        const float dyOld = dy_;
        dx_=dx; dy_=dy;
        onResize(dxOld,dyOld);
        for (auto &c : children_) {
          c->onResizeParent();
        }
        return this;
      }
      virtual void onResize(const float dxOld, const float dyOld) {
        std::cout << "resizig: " << fullName() << std::endl;
      }

      Area* positionIs(const float x,const float y) {
        const float xOld = x_;
        const float yOld = y_;
        x_=x; y_=y;
        onPosition(xOld,yOld);
        return this;
      }
      virtual void onPosition(const float xOld, const float yOld) {
        std::cout << "position: " << fullName() << std::endl;
      }

      Area* fillParentIs(bool const value) { fillParent_ = value; return this;}
      Area* childIs(Area* c) {children_.push_back(c); return this;}
      Area* parentIs(Area* p) {parent_=p; return this;}
      Area* firstChild() {return children_.front();}
      bool isFirstChild() {return parent_?this==parent_->firstChild():true;}

      void setFocus(bool isRecursive=false) {
        std::cout << "setFocus " << fullName() << std::endl;
        if (!isRecursive) {
          Area* const oldFocus = getFocus();
          if (oldFocus==this) {return;}
          if (oldFocus) {oldFocus->unFocus();}
          isFocus_=true;
        }
        isFocusPath_=true;
        if (parent_) {
          parent_->firstChildIs(this);
          parent_->setFocus(true);
        }
      }
      void unFocus() {
        std::cout << "unFocus " << fullName() << std::endl;
        isFocus_=false;
        isFocusPath_=false;
        if (parent_) {parent_->unFocus();}
      }
      Area* getFocus(bool traverseDown=false) {
        // travels up to top parent, then travels down until hits the focus child
        if (isFocus_) {return this;}

        if (traverseDown) {
          Area* const f = children_.front();
          if (f) {
            return f->getFocus(true);
          } else {
            return NULL;
          }
        } else {
          if (parent_) {
            parent_->getFocus(false);
          } else {
            return getFocus(true);
          }
        }
      }
      bool isFocus() {return isFocus_;}
      bool isFocusPath() {return isFocusPath_;}

      void colorIs(Color c) {bgColor_=c;}
    protected:
      std::string name_;
      Area* parent_ = NULL;
      std::list<Area*> children_;
      bool firstChildIs(Area* c) {
        const bool found = (std::find(children_.begin(), children_.end(), c) != children_.end());
        if (found) {children_.remove(c);}
        children_.push_front(c);
        return found;
      }
      float x_,y_,dx_,dy_; // x,y relative to parent
      bool fillParent_ = false;
      Color bgColor_ = Color(0.5,0.1,0.1);
      bool isFocus_=false;
      bool isFocusPath_=false;
    };// class Area
    class Button : public Area {
    public:
      Button(const std::string& name,Area* const parent,
             const float x,const float y,const float dx,const float dy,
             const std::string text,
             const std::vector<Color> bgColors = std::vector<Color>{Color(0.5,0.5,0.5),Color(0.4,0.4,0.4),Color(0.3,0.3,0.3),Color(0.2,0.2,0.2)},
             const std::vector<Color> textColors = std::vector<Color>{Color(0,0,0),Color(0,0,0),Color(1,1,1),Color(1,1,1)}
            )
      : Area(name,parent,x,y,dx,dy),text_(text),bgColors_(bgColors),textColors_(textColors){}

      virtual void draw(const float px,const float py, sf::RenderTarget &target) {
        float gx = x_+px;
        float gy = y_+py;
        DrawRect(gx, gy, dx_, dy_, target, bgColors_[state_]);
        DrawText(gx+1, gy+1, text_, (dy_-2), target, textColors_[state_]);
        for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
          (*rit)->draw(x_+px, y_+py,target);
        }
      }

      virtual void onMouseOverStart() {if (state_==0) {state_=1;}}
      virtual void onMouseOver(const float px,const float py) {// relative to parent
        if (state_==0) {state_=1;}
      }
      virtual void onMouseOverEnd() {if (state_==1) {state_=0;}}

      virtual bool onMouseDownStart(const bool isFirstDown,const float x,const float y) {
        std::cout << "onMouseDownStart " << fullName() << " fist:" << std::to_string(isFirstDown) << std::endl;
        if (isFirstDown) {setFocus();}
        return true;
      }
      virtual bool onMouseDown(const bool isCaptured,const float x,const float y,float &dx, float &dy,Area* const over) {dx=0,dy=0;return false;}

    protected:
      std::string text_;
      std::vector<Color> bgColors_,textColors_;
      size_t state_=0;//0:normal, 1:mouse over, 2:pressing, 3:clicked
    };
    class Window : public Area {
    public:
      Window(const std::string& name,Area* const parent, const float x,const float y,const float dx,const float dy, const std::string title)
      : Area(name,parent,x,y,dx,dy),title_(title){
        bgColor_ = Color(0.1,0.5,0.1);
        closeButton_ = new Button("close",this,borderSize,borderSize,headerSize-2*borderSize,headerSize-2*borderSize,"X",
                                        std::vector<Color>{Color(0.5,0,0),Color(0.4,0,0),Color(0.2,0,0),Color(0.2,0,0)},
                                        std::vector<Color>{Color(1,0.5,0.5),Color(1,0.8,0.8),Color(0.6,0.1,0.1),Color(0.5,0,0)}
                                      );
      }

      virtual void draw(const float px,const float py, sf::RenderTarget &target) {// px global pos of parent
        float gx = x_+px;
        float gy = y_+py;

        DrawRect(gx, gy, dx_, dy_, target, bgColor_*(isFocusPath()?1.0:0.8));
        DrawText(gx+borderSize+headerSize, gy+borderSize, title_, (headerSize-2*borderSize), target, Color(1,1,1));

        sf::View viewOld = target.getView(); // push new view
        sf::View view;
        float cx,cy,cdx,cdy;childSize(cdx,cdy);childOffset(cx,cy);
        sf::Vector2u size = target.getSize();
        sf::FloatRect rect = sf::FloatRect((gx+cx)/size.x,(gy+cy)/size.y,(cdx)/size.x,(cdy)/size.y);
        sf::FloatRect inter;
        rect.intersects(viewOld.getViewport(),inter);
        view.setViewport(inter);
        view.reset(sf::FloatRect(cx,cy,inter.width*size.x,inter.height*size.y));
        view.move(sf::Vector2f(gx,gy));
        target.setView(view);


        for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
          if (closeButton_!=(*rit)) {
            (*rit)->draw(gx,gy,target);
          }
        }
        target.setView(viewOld);// pop new view

        closeButton_->draw(gx,gy,target);
      }

      virtual bool onMouseDownStart(const bool isFirstDown,const float x,const float y) {
        if (isFirstDown) {
          setFocus();
          float lx = x-globalX();
          float ly = y-globalY();

          // check what to do with drag:
          resetResizing();
          if (lx<borderSize) {isResizingLeft = true;}
          if (ly<borderSize) {isResizingTop = true;}
          if (lx>dx_-borderSize) {isResizingRight = true;}
          if (ly>dy_-borderSize) {isResizingBottom = true;}
          isDragging = !(isResizingLeft || isResizingTop || isResizingRight || isResizingBottom);
        }
        return true;
      }

      virtual bool onMouseDown(const bool isCaptured,const float x,const float y,float &dx, float &dy,Area* const over) {
        float moveX=0, moveY=0,moveDX=0, moveDY=0;
        float cx,cy,cdx,cdy;
        parent_->childSize(cdx,cdy);parent_->childOffset(cx,cy);

        if (isCaptured) {
          if (isDragging || isResizingLeft) {
            moveX=std::max(cx-x_,std::min(cx+cdx-x_-dx_,dx));
            dx=moveX;
            if (isResizingLeft) {moveDX=-dx;}
          }
          if (isDragging || isResizingTop) {
            moveY=std::max(cy-y_,std::min(cy+cdy-y_-dy_,dy));
            dy=moveY;
            if (isResizingTop) {moveDY=-dy;}
          }
          if (isResizingRight) {
            moveDX=std::max(cx-x_,std::min(cx+cdx-x_-dx_,dx));
            dx=moveDX;
          }
          if (isResizingBottom) {
            moveDY=std::max(cy-y_,std::min(cy+cdy-y_-dy_,dy));
            dy=moveDY;
          }
        }
        if (moveX!=0 || moveY!=0) {positionIs(x_+moveX,y_+moveY);}
        if (moveDX!=0 || moveDY!=0) {sizeIs(dx_+moveDX,dy_+moveDY);}
        return true;
      }
      virtual void onMouseDownEnd(const bool isCaptured, const bool isLastDown,const float x,const float y,Area* const over) {
        resetResizing();
        isDragging = false;
      }

      virtual void childSize(float &cdx, float &cdy) {
        cdx = dx_-2*borderSize;
        cdy = dy_-headerSize-borderSize;
      }
      virtual void childOffset(float &cx, float &cy) {
        cx = borderSize;
        cy = headerSize;
      }


    protected:
      std::string title_;
      Button* closeButton_=NULL;//shortcut, still in children
      const float borderSize = 5.0; // sides and bottom
      const float headerSize = 25.0;// top
      bool isResizable = false;
      bool isResizingLeft = false;
      bool isResizingRight = false;
      bool isResizingTop = false;
      bool isResizingBottom = false;
      bool isDragging = false;
      void resetResizing() {
        isResizingLeft = false;
        isResizingRight = false;
        isResizingTop = false;
        isResizingBottom = false;
      }
    };

    class Slider : public Area {
    public:
      class SliderButtton : public Area {
      public:
        SliderButtton(const std::string& name,Area* const parent,
               const float x,const float y,const float dx,const float dy,
               const std::vector<Color> buttonColors = std::vector<Color>{Color(0.6,0.6,0.6),Color(0.7,0.7,0.7),Color(0.8,0.8,0.8)}
              )
        : Area(name,parent,x,y,dx,dy),buttonColors_(buttonColors){}
        virtual void onMouseOverStart() {if (state_==0) {state_=1;}}
        virtual void onMouseOver() {if (state_==0) {state_=1;}}
        virtual void onMouseOverEnd() {if (state_==1) {state_=0;}}

        virtual bool onMouseDownStart(const bool isFirstDown,const float x,const float y) {
          if (isFirstDown) {
            setFocus();
            state_=2;
          }
          return isFirstDown;
        }
        virtual bool onMouseDown(const bool isCaptured,const float x,const float y,float &dx, float &dy,Area* const over) {
          if (isCaptured) {
            if(Slider* p = dynamic_cast<Slider*>(parent_)) {
              return p->onSliderButtonDrag(dx,dy);// let parent handle the case
            }
          }
          return true;
        }
        virtual void onMouseDownEnd(const bool isCaptured, const bool isLastDown,const float x,const float y,Area* const over) {
          if (isCaptured) {
            state_=0;
          }
        }
        virtual void draw(const float px,const float py, sf::RenderTarget &target) {
          float gx = x_+px;
          float gy = y_+py;
          DrawRect(gx, gy, dx_, dy_, target, buttonColors_[state_]);
          for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
            (*rit)->draw(x_+px, y_+py,target);
          }
        }
      protected:
        std::vector<Color> buttonColors_;
        size_t state_=0;//0:normal, 1:mouse over, 2:pressing
      };
      Slider(const std::string& name,Area* const parent,
             const float x,const float y,const float dx,const float dy,
             const bool isHorizontal,const float minVal,const float maxVal,const float initVal,const float buttonLength,
             const std::vector<Color> bgColors = std::vector<Color>{Color(0.5,0.5,0.5),Color(0.4,0.4,0.4),Color(0.3,0.3,0.3)},
             const std::vector<Color> buttonColors = std::vector<Color>{Color(0.6,0.6,0.6),Color(0.7,0.7,0.7),Color(0.8,0.8,0.8)}
            )
      : Area(name,parent,x,y,dx,dy),
        isHorizontal_(isHorizontal),minVal_(minVal),maxVal_(maxVal),val_(initVal),buttonLength_(buttonLength),
        bgColors_(bgColors){
            sliderButton_ = new SliderButtton("button",this,0,0,dx/2,dy/2,buttonColors);
            adjustChildren();
      }
      void adjustChildren() {
        const float bLenRel = buttonLength_/(maxVal_-minVal_+buttonLength_);
        const float bPosRel = (val_-minVal_)/(maxVal_-minVal_+buttonLength_);
        if (isHorizontal_) {
          sliderButton_->sizeIs(dx_*bLenRel,dy_);
          sliderButton_->positionIs(dx_*bPosRel,0);
        } else {
          sliderButton_->sizeIs(dx_,dy_*bLenRel);
          sliderButton_->positionIs(0,dy_*bPosRel);
        }
      }
      void valIs(float val) {
        if (val_!=val) {
          val_=val;
          adjustChildren();
          if (onVal_) {onVal_(val_);}
        }
      }
      void onValIs(std::function<void(float)> onVal) {onVal_=onVal;}// use to listen to value change
      void valBoundsIs(const float minVal,const float maxVal,const float buttonLength) {
        minVal_=minVal; maxVal_=maxVal;buttonLength_=buttonLength;
        if (val_<minVal_) {valIs(minVal_);} else if (val_>maxVal_) {valIs(maxVal_);}
        adjustChildren();
      }
      bool onSliderButtonDrag(float &dx, float &dy) {
        if (minVal_==maxVal_) {return true;}// abort, want no bad action
        float *absDrag;
        float absLen;
        if (isHorizontal_) {
          absDrag = &dx;
          absLen = dx_-sliderButton_->dx();
        } else {
          absDrag = &dy;
          absLen = dy_-sliderButton_->dy();
        }
        float relDrag = (*absDrag)/(absLen);
        float wantDrag = relDrag*(maxVal_-minVal_);
        float doDrag = std::max(minVal_-val_,std::min(maxVal_-val_,wantDrag));
        valIs(val_+doDrag);// apply drag
        float doRelDrag = doDrag/(maxVal_-minVal_);
        *absDrag = doRelDrag*absLen;// feedback to caller
        return true;
      }
      virtual void onResize(const float dxOld, const float dyOld) {
        adjustChildren();
      }

      virtual void onMouseOverStart() {if (state_==0) {state_=1;}}
      virtual void onMouseOver() {if (state_==0) {state_=1;}}
      virtual void onMouseOverEnd() {if (state_==1) {state_=0;}}

      virtual bool onMouseDownStart(const bool isFirstDown,const float x,const float y) {
        if (isFirstDown) {
          setFocus();
          state_=2;
        }
        return isFirstDown;
      }
      virtual bool onMouseDown(const bool isCaptured,const float x,const float y,float &dx, float &dy,Area* const over) {return true;}
      virtual void onMouseDownEnd(const bool isCaptured, const bool isLastDown,const float x,const float y,Area* const over) {
        if (isCaptured) {
          state_=0;
        }
      }

      virtual void draw(const float px,const float py, sf::RenderTarget &target) {
        float gx = x_+px;
        float gy = y_+py;
        DrawRect(gx, gy, dx_, dy_, target, bgColors_[state_]);
        for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
          (*rit)->draw(x_+px, y_+py,target);
        }
      }

    protected:
      SliderButtton* sliderButton_;
      bool isHorizontal_;
      float minVal_,maxVal_,val_,buttonLength_;
      std::function<void(float)> onVal_;
      std::vector<Color> bgColors_;
      size_t state_=0;//0:normal, 1:mouse over, 2:pressing
    };

    class ScrollArea : public Area {
    public:
      class ScrollAreaViewer : public Area {
      public:
        ScrollAreaViewer(const std::string& name,Area* const parent, Area* const child,const float x,const float y,const float dx,const float dy)
        : Area(name,parent,x,y,dx,dy),child_(child){
          child_->parentIs(this);
          childIs(child_);
        }
        virtual void draw(const float px,const float py, sf::RenderTarget &target) {
          float gx = x_+px;
          float gy = y_+py;
          DrawRect(gx, gy, dx_, dy_, target, bgColor_);

          sf::View viewOld = target.getView(); // push new view
          sf::View view;
          float cx,cy,cdx,cdy;childSize(cdx,cdy);childOffset(cx,cy);
          sf::Vector2u size = target.getSize();
          sf::FloatRect rect = sf::FloatRect((gx+cx)/size.x,(gy+cy)/size.y,(cdx)/size.x,(cdy)/size.y);
          sf::FloatRect inter;
          rect.intersects(viewOld.getViewport(),inter);
          view.setViewport(inter);
          view.reset(sf::FloatRect(cx,cy,inter.width*size.x,inter.height*size.y));
          view.move(sf::Vector2f(gx,gy));
          target.setView(view);

          for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
              (*rit)->draw(gx+childOffsetX_,gy+childOffsetY_,target);
          }
          target.setView(viewOld);// pop new view
        }
        virtual Area* checkMouseOver(const float px,const float py,const bool doNotify=true) {// relative to parent
          if (px>=x_ and py>=y_ and px<=(x_+dx_) and py<=(y_+dy_)) {
            for (auto &c : children_) {
              Area* over = c->checkMouseOver(px-x_-childOffsetX_,py-y_-childOffsetY_,doNotify);
              if (over!=NULL) {
                return over;
              }
            }
            if (doNotify) {onMouseOver(px,py);}
            return this;
          }
          return NULL;
        }
        void childOffsetXIs(const float v) {childOffsetX_=v;}
        void childOffsetYIs(const float v) {childOffsetY_=v;}
        Area* child() const {return child_;}
      protected:
        Area* child_;
        float childOffsetX_=0;
        float childOffsetY_=0;
      };

      ScrollArea(const std::string& name,Area* const parent, Area* const child,
                 const float x,const float y,const float dx,const float dy,
                 const bool scrollX=true,const bool scrollY=true)
      : Area(name,parent,x,y,dx,dy),scrollX_(scrollX),scrollY_(scrollY){
        scrollView_ = new ScrollAreaViewer("viewer",this,child,0,0,dx,dy);
        if (scrollX_) {
          sliderX_ = new Slider("sliderX",this,0,0,dx,dy,true,0,100,0,20);
          sliderX_->onValIs([this](float val) {scrollView_->childOffsetXIs(-val);});
        }
        if (scrollY_) {
          sliderY_ = new Slider("sliderY",this,0,0,dx,dy,false,0,100,0,20);
          sliderY_->onValIs([this](float val) {scrollView_->childOffsetYIs(-val);});
        }
        adjustChildren();// above just set bogus values, this will adjust them all anyway
      }
      void adjustChildren() {
        float middleX = scrollX_?dx_-scrollerWidth_:dx_;
        float middleY = scrollX_?dy_-scrollerWidth_:dy_;
        scrollView_->sizeIs(middleX,middleY);
        if (sliderX_) {
          sliderX_->positionIs(0,middleY);
          sliderX_->sizeIs(middleX,scrollerWidth_);
          sliderX_->valBoundsIs(0.0f,std::max(0.0f,scrollView_->child()->dx()-middleX),middleX);
        }
        if (sliderY_) {
          sliderY_->positionIs(middleX,0);
          sliderY_->sizeIs(scrollerWidth_,middleY);
          sliderY_->valBoundsIs(0.0f,std::max(0.0f,scrollView_->child()->dy()-middleY),middleY);
        }
      }
      virtual void onResize(const float dxOld, const float dyOld) {
        adjustChildren();
      }
    protected:
      ScrollAreaViewer* scrollView_;
      Slider* sliderX_;
      Slider* sliderY_;
      const bool scrollX_;
      const bool scrollY_;
      float scrollerWidth_=20;
    };

    class Socket : public Area {
    public:
      enum Direction {Up,Down};
      Socket(const std::string& name,Area* const parent,
             const float x,const float y,Direction direction,
             const std::string text,
             const Color bgColor = Color(0.5,0.5,0.5),
             const Color textColor = Color(0,0,0),
             const Color socketColor = Color(0,0,0),
             const Color connectorColor = Color(0,0,0)
            )
      : Area(name,parent,x,y,20,20),text_(text),textColor_(textColor),socketColor_(socketColor),connectorColor_(connectorColor),direction_(direction){
        colorIs(bgColor);
        switch (direction_) {
          case Direction::Up:
          case Direction::Down:{
            sizeIs(16,24);
            break;
          }
        }
        canTakeSink = []() {return true;};
        canMakeSource = [](Socket* s) {return true;};
        onSinkIs = [](Socket* s) {};
        onSinkDel = [](Socket* s) {};
        onSourceIs = [](Socket* s) {};
        onSourceDel = [](Socket* s) {};
      }

      float socketX() {
        switch (direction_) {
          case Direction::Up:return dx_*0.5;
          case Direction::Down:return dx_*0.5;
        }
      };
      float socketY() {
        switch (direction_) {
          case Direction::Up:return dx_*0.5;
          case Direction::Down:return dy_-dx_*0.5;
        }
      };
      Color connectorColor() {return connectorColor_;}

      // for client use
      void sourceIs(Socket* source) {
        if (source==source_) {return;}
        if (source_) {
          onSourceDel(source_);
          source_->sinkDel(this);
        }
        source_=source;
        if (source_) {
          source_->sinkIs(this);
          onSourceIs(source_);
        }
      }

      // internal: only react!
      void sinkIs(Socket* sink) {
        const bool found = (std::find(sink_.begin(), sink_.end(), sink) != sink_.end());
        if (!found) {
          sink_.push_front(sink);
          onSinkIs(sink);
        }
      }
      void sinkDel(Socket* sink) {
        const bool found = (std::find(sink_.begin(), sink_.end(), sink) != sink_.end());
        if (found) {
          onSinkDel(sink);
          sink_.remove(sink);
        }
      }

      virtual void draw(const float px,const float py, sf::RenderTarget &target) {
        float gx = x_+px;
        float gy = y_+py;

        switch (direction_) {
          case Direction::Up:{
            DrawOval(gx, gy, dx_,dx_,target,bgColor_);
            DrawRect(gx, gy+dx_*0.5, dx_, dy_-dx_*0.5, target, bgColor_);
            DrawOval(gx+dx_*0.25, gy+dx_*0.25, dx_*0.5,dx_*0.5,target,socketColor_);
            DrawText(gx+1, gy+dy_*0.5, text_, (dy_-dx_), target, textColor_);
            break;
          }
          case Direction::Down:{
            DrawOval(gx, gy+dy_-dx_, dx_,dx_,target,bgColor_);
            DrawRect(gx, gy, dx_, dy_-dx_*0.5, target, bgColor_);
            DrawOval(gx+dx_*0.25, gy+dy_-dx_*0.75, dx_*0.5,dx_*0.5,target,socketColor_);
            DrawText(gx+1, gy+1, text_, (dy_-dx_), target, textColor_);
            break;
          }
        }
        if (source_) {
          DrawLine(x_+px+socketX(),y_+py+socketY(),source_->globalX()+source_->socketX(),source_->globalY()+source_->socketY(),target,connectorColor_,dx_*0.5);
        }
        for (std::list<Socket*>::reverse_iterator rit=sink_.rbegin(); rit!=sink_.rend(); ++rit) {
          DrawLine(x_+px+socketX(),y_+py+socketY(),(*rit)->globalX()+(*rit)->socketX(),(*rit)->globalY()+(*rit)->socketY(),target,(*rit)->connectorColor(),dx_*0.5);
        }
        if (isSettingSource) {
          DrawLine(x_+px+socketX(),y_+py+socketY(),isSettingSourceX,isSettingSourceY,target,connectorColor_,dx_*0.5);
        }

        for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
          (*rit)->draw(x_+px, y_+py,target);
        }
      }
      virtual bool onMouseDownStart(const bool isFirstDown,const float x,const float y) {
        if (isFirstDown) {setFocus();}
        if (canTakeSink()) {
          isSettingSource = true;
          isSettingSourceX = x;
          isSettingSourceY = y;
          sourceIs(NULL);
          return true;
        } else {
          return false;
        }
      }
      virtual bool onMouseDown(const bool isCaptured,const float x,const float y,float &dx, float &dy,Area* const over) {
        if (isCaptured and isSettingSource) {
          isSettingSourceX = x;
          isSettingSourceY = y;
        }
        return true;
      }
      virtual void onMouseDownEnd(const bool isCaptured, const bool isLastDown,const float x,const float y,Area* const over) {
        isSettingSource = false;
        if (isCaptured) {
          Socket* source = dynamic_cast<Socket*>(over);
          if (source and source!=this and source!=source_ and source->canMakeSource(this)) {
            sourceIs(source);
          }
        }
      }

    protected:
      std::string text_;
      Color textColor_,socketColor_,connectorColor_;
      Direction direction_;
      Socket* source_=NULL;

      bool isSettingSource = false;
      float isSettingSourceX,isSettingSourceY;

      std::list<Socket*> sink_;

      // requests:
      std::function<bool()> canTakeSink;// return if allow
      std::function<bool(Socket*)> canMakeSource;// return if allow

      // reactions:
      std::function<void(Socket*)> onSinkIs;
      std::function<void(Socket*)> onSinkDel;
      std::function<void(Socket*)> onSourceIs;
      std::function<void(Socket*)> onSourceDel;
    };

    class Block : public Area {
    public:
      Block(const std::string& name,Area* const parent,
             const float x,const float y,const float dx,const float dy,
             const Color bgColor = Color(0.5,0.5,0.5)
            )
      : Area(name,parent,x,y,dx,dy){
        colorIs(bgColor);
      }

      virtual bool onMouseDownStart(const bool isFirstDown,const float x,const float y) {
        if (isFirstDown) {setFocus();}
        return true;
      }

      virtual bool onMouseDown(const bool isCaptured,const float x,const float y,float &dx, float &dy,Area* const over) {
        float moveX=0, moveY=0;
        float cx,cy,cdx,cdy;
        parent_->childSize(cdx,cdy);parent_->childOffset(cx,cy);

        if (isCaptured) {
          moveX=std::max(cx-x_,std::min(cx+cdx-x_-dx_,dx));
          dx=moveX;
          moveY=std::max(cy-y_,std::min(cy+cdy-y_-dy_,dy));
          dy=moveY;
        }
        if (moveX!=0 || moveY!=0) {positionIs(x_+moveX,y_+moveY);}
        return true;
      }
      virtual void onMouseDownEnd(const bool isCaptured, const bool isLastDown,const float x,const float y,Area* const over) {
      }
    protected:
    };

    class MasterWindow {
    public:
      MasterWindow(const size_t dx,const size_t dy,std::string title,const bool fullScreen=false) {
        sf::ContextSettings settings;
        settings.antialiasingLevel = 8;

        if (fullScreen) {
          renderWindow_ = new sf::RenderWindow(sf::VideoMode(dx, dy), title, sf::Style::Fullscreen, settings);
        } else {
          renderWindow_ = new sf::RenderWindow(sf::VideoMode(dx, dy), title, sf::Style::Default, settings);
        }

        renderWindow_->setVerticalSyncEnabled(true);
        renderWindow_->setKeyRepeatEnabled(false);

        sf::Vector2u size = renderWindow_->getSize();
        mainArea_ = (new EP::GUI::Area("main",NULL,0,0,size.x,size.y))->fillParentIs(true);

        sf::Vector2i mousepos = sf::Mouse::getPosition(*renderWindow_);
        lastMouseX_ = mousepos.x;
        lastMouseY_ = mousepos.y;
      }
      sf::RenderWindow* target() {return renderWindow_;}
      Area* area() {return mainArea_;}

      void update() {
        sf::Event event;
        while (renderWindow_->pollEvent(event)) {
          switch (event.type) {
            case sf::Event::Closed: {
              close();
              break;
            }
            case sf::Event::MouseWheelScrolled: {
              if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                int delta = event.mouseWheelScroll.delta;
              }
              break;
            }
            case sf::Event::MouseButtonPressed: {
              if(event.mouseButton.button == sf::Mouse::Left) {
                mouseDown_ = true;
                if (mouseOverArea_) {
                  mouseDownArea_ = mouseOverArea_;
                  mouseDownCaptured_ = mouseOverArea_->onMouseDownStart(true,lastMouseX_,lastMouseY_);
                  if (mouseDownCaptured_) {
                    std::cout << "mouseDownEvent captured by " << mouseOverArea_->fullName() << std::endl;
                  }
                }
              } else if(event.mouseButton.button == sf::Mouse::Right) {
                // forget for now
              }
              break;
            }
            case sf::Event::MouseButtonReleased: {
              if(event.mouseButton.button == sf::Mouse::Left) {
                mouseDown_ = false;
                if (mouseDownArea_) {
                  mouseDownArea_->onMouseDownEnd(mouseDownCaptured_,true,lastMouseX_,lastMouseY_,mouseOverArea_);
                }
                mouseDownReset();
              } else if(event.mouseButton.button == sf::Mouse::Right) {
                // forget for now
              }
              break;
            }
            case sf::Event::MouseMoved: {
              float mouseX = event.mouseMove.x;
              float mouseY = event.mouseMove.y;
              const float origMouseDX = mouseX-lastMouseX_+mouseDownKeepDragDX_;
              const float origMouseDY = mouseY-lastMouseY_+mouseDownKeepDragDY_;
              float mouseDX = origMouseDX;
              float mouseDY = origMouseDY;

              bool keepDrag = true;

              if (mouseDown_) {
                // captured -> simply notify, update mouse
                // not captured: if has area: check, if leaves: update mouse, notify leaving, possibly start notify
                // no area: update mouse, possibly start notify after
                if (mouseDownArea_) {
                  keepDrag = mouseDownArea_->onMouseDown(mouseDownCaptured_,lastMouseX_,lastMouseY_,mouseDX,mouseDY,mouseOverArea_);
                  if (keepDrag) {
                    mouseDownKeepDragDX_+= origMouseDX-mouseDX;
                    mouseDownKeepDragDY_+= origMouseDY-mouseDY;
                  }
                }
              }

              if (keepDrag) {
                lastMouseX_+=origMouseDX;
                lastMouseY_+=origMouseDY;
              } else {
                lastMouseX_+=mouseDX;
                lastMouseY_+=mouseDX;
              }


              if (!(origMouseDX==mouseDX && origMouseDY==mouseDY) && !keepDrag) {
                sf::Mouse::setPosition(sf::Vector2i(lastMouseX_,lastMouseY_),*renderWindow_);
              }

              EP::GUI::Area* mouseOverNew = mainArea_->checkMouseOver(lastMouseX_,lastMouseY_);
              if (mouseOverArea_!=mouseOverNew) {
                // end old:
                if (!mouseDownCaptured_ && mouseDownArea_!=NULL && mouseDownArea_!=mouseOverNew) {
                  mouseDownArea_->onMouseDownEnd(mouseDownCaptured_,false,lastMouseX_,lastMouseY_,mouseOverArea_);
                  mouseDownArea_=NULL;
                }
                if (mouseOverArea_!=NULL) {
                  mouseOverArea_->onMouseOverEnd();
                  mouseOverArea_=NULL;
                }
                // start new:
                if (mouseOverNew!=NULL) {
                  mouseOverArea_ = mouseOverNew;
                  mouseOverArea_->onMouseOverStart();

                  if (mouseDown_ && !mouseDownArea_) {
                    mouseDownArea_ = mouseOverArea_;
                    mouseDownArea_->onMouseDownStart(false,lastMouseX_,lastMouseY_);
                    mouseDownCaptured_ = false;
                  }
                }
              }
              break;
            }
            case sf::Event::KeyPressed: {
              Area* const focus = mainArea_->getFocus();
              if (focus) {focus->onKeyPressed(event.key.code);}
              break;
            }
            case sf::Event::Resized: {
              sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
              renderWindow_->setView(sf::View(visibleArea));
              sf::Vector2u size = renderWindow_->getSize();
              mainArea_->sizeIs(size.x,size.y);
              break;
            }
            default: {break;}
          }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          close();
        }
      }
      void draw() {
        renderWindow_->clear();
        mainArea_->draw(0,0,*renderWindow_);
        // DrawDot(MOUSE_X, MOUSE_Y, window, sf::Color(255*MOUSE_RIGHT_DOWN,255*MOUSE_LEFT_DOWN,255));
        if (mouseOverArea_) {
          EP::DrawText(5,5, "over: " + mouseOverArea_->fullName(), 10, *renderWindow_, EP::Color(1.0,1.0,1.0));
        }
        if(mouseDown_) {
          std::string p = "down: " + (mouseDownArea_?mouseDownArea_->fullName():"None") + " " + (mouseDownCaptured_?"captured":"glide");
          EP::DrawText(5,20, p, 10, *renderWindow_, EP::Color(1.0,1.0,1.0));
        }
        {
          Area* const focus = mainArea_->getFocus();
          std::string p = "focus: " + (focus?focus->fullName():"None");
          EP::DrawText(5,35, p, 10, *renderWindow_, EP::Color(1.0,1.0,1.0));
        }

        renderWindow_->display();
      }
      void close() {renderWindow_->close();}
      bool isAlive() {return renderWindow_->isOpen();}
    protected:
      sf::RenderWindow *renderWindow_;
      Area* mainArea_;
      Area* mouseOverArea_;
      bool mouseDown_ = false;
      Area* mouseDownArea_;
      bool mouseDownCaptured_ = false;
      float mouseDownKeepDragDX_ = 0;
      float mouseDownKeepDragDY_ = 0;
      void mouseDownReset() {
        mouseDownArea_ = NULL;
        mouseDownCaptured_ = false;
        mouseDownKeepDragDX_ = 0;
        mouseDownKeepDragDY_ = 0;
      }
      float lastMouseX_;
      float lastMouseY_;
    };
  }// namespace GUI
}// namespace EP
#endif //EP_GUI_HPP
