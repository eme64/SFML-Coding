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

      virtual bool onMouseDown(const bool isCaptured,const float x,const float y,float &dx, float &dy) {return true;}
      // px,py: relative to parent, position of mouse in last frame.
      //dx/dy the moving the mouse is trying to do. If change (eg =0), mouse position will be changed accordingly.
      //return true: keep drag, false: chain mouse

      virtual void onMouseDownEnd(const bool isCaptured, const bool isLastDown,const float x,const float y) {
        std::cout << "onMouseDownEnd " << fullName() << " captured:" << std::to_string(isCaptured) << " last:" << std::to_string(isLastDown) << std::endl;
      }
      // isLastDown: if true indicates that mouse physically released, else only left scope bc not captured


      // report absolute shift (drag), report relative speed time (keep mouse position), remember original position (up to Area?)
      // lock to this element?

      Area* checkMouseOver(const float px,const float py,const bool doNotify=true) {// relative to parent
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

      void setFocus() {
        if (parent_) {
          parent_->firstChildIs(this);
          parent_->setFocus();
        }
      }
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
      virtual void onMouseOver(const float px,const float py) {// relative to parent
        if (state_==0) {state_=1;}
      }
      virtual void onMouseOverStart() {if (state_==0) {state_=1;}}
      virtual bool onMouseDown(const bool isCaptured,const float x,const float y,float &dx, float &dy) {dx=0,dy=0;return false;}
      virtual void onMouseOverEnd() {if (state_==1) {state_=0;}}

      virtual bool onMouseDownStart(const bool isFirstDown,const float x,const float y) {
        std::cout << "onMouseDownStart " << fullName() << " fist:" << std::to_string(isFirstDown) << std::endl;
        if (isFirstDown) {setFocus();}
        return true;
      }

    private:
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

        DrawRect(gx, gy, dx_, dy_, target, bgColor_);
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

      virtual bool onMouseDown(const bool isCaptured,const float x,const float y,float &dx, float &dy) {
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
      virtual void onMouseDownEnd(const bool isCaptured, const bool isLastDown,const float x,const float y) {
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


    private:
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

    class ScrollArea : public Area {
    public:
      ScrollArea(const std::string& name,Area* const parent, Area* const child,const float x,const float y,const float dx,const float dy)
      : Area(name,parent,x,y,dx,dy),child_(child){
        child_->parentIs(this);
        childIs(child_);
      }

      virtual void draw(const float px,const float py, sf::RenderTarget &target) {
        float gx = x_+px;
        float gy = y_+py;
        DrawRect(gx, gy, dx_, dy_, target, bgColor_);
        for (std::list<Area*>::reverse_iterator rit=children_.rbegin(); rit!=children_.rend(); ++rit) {
          (*rit)->draw(x_+px+childOffsetX_, y_+py+childOffsetY_,target);
        }
      }

    private:
      Area* child_;
      float childOffsetX_=0;
      float childOffsetY_=0;
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
                  mouseDownArea_->onMouseDownEnd(mouseDownCaptured_,true,lastMouseX_,lastMouseY_);
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
                  keepDrag = mouseDownArea_->onMouseDown(mouseDownCaptured_,lastMouseX_,lastMouseY_,mouseDX,mouseDY);
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
                  mouseDownArea_->onMouseDownEnd(mouseDownCaptured_,false,lastMouseX_,lastMouseY_);
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
        renderWindow_->display();
      }
      void close() {renderWindow_->close();}
      bool isAlive() {return renderWindow_->isOpen();}
    private:
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
