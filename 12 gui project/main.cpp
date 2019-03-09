
//#include <cstddef>

// #include <SFML/Audio.hpp>
// #include <list>
// #include <queue>
// #include <algorithm>
// #include <thread>
// #include <valarray>
// #include <complex>
// #include <mutex>
// #include <forward_list>


#include "ep_gui.hpp"

void addBlockFM(EP::GUI::Area* const parent,const float x,const float y) {
  EP::GUI::Block* block = new EP::GUI::Block("parent",parent,x,y,100,100,EP::Color(0.2,0,0));
  EP::GUI::Socket* socketFq = new EP::GUI::Socket("socketFq",block,5,5,EP::GUI::Socket::Direction::Up,"Fq");
  EP::GUI::Socket* socketMod = new EP::GUI::Socket("socketMod",block,30,5,EP::GUI::Socket::Direction::Up,"Mod");
  EP::GUI::Socket* socketOut = new EP::GUI::Socket("socketOut",block,5,50,EP::GUI::Socket::Direction::Down,"Out");
}



//-------------------------------------------------------- Main
int main()
{
  // ------------------------------------ WINDOW
  EP::GUI::MasterWindow* masterWindow = new EP::GUI::MasterWindow(1000,600,"window title",false);
  {
    EP::GUI::Window* window1 = new EP::GUI::Window("window1",masterWindow->area(),10,10,200,300,"MyWindow1");
    EP::GUI::Window* w = window1;
    for (size_t i = 3; i < 7; i++) {
      float x,y,dx,dy;
      w->childSize(dx,dy);
      w->childOffset(x,y);
      EP::GUI::Area* content1 = new EP::GUI::Area("content"+std::to_string(i),w,x,y,dx,dy);
      content1->colorIs(EP::Color(0.2,0.2,0.3));
      content1->fillParentIs(true);
      EP::GUI::Window* ww = new EP::GUI::Window("w"+std::to_string(i),content1,10,10,100,100,"w"+std::to_string(i));
      w = ww;
    }
    EP::GUI::Window* window2 = new EP::GUI::Window("window2",masterWindow->area(),300,150,300,200,"MyWindow2");
    {
      float x,y,dx,dy;
      window2->childSize(dx,dy);
      window2->childOffset(x,y);
      EP::GUI::Area* content1 = new EP::GUI::Area("content1",NULL,0,0,500,500);
      content1->colorIs(EP::Color(0.2,0.2,0.3));
      EP::GUI::Area* scroll1 = new EP::GUI::ScrollArea("scroll1",window2, content1,x,y,dx,dy);
      scroll1->colorIs(EP::Color(0.5,0.5,1));
      scroll1->fillParentIs(true);
      EP::GUI::Area* button1 = new EP::GUI::Button("button1",content1,10,10,100,20,"X");
      EP::GUI::Area* button2 = new EP::GUI::Button("button2",content1,300,300,100,20,"Y");
      addBlockFM(content1,100,100);
      addBlockFM(content1,100,200);
    }
  }
  while (masterWindow->isAlive()) {
    masterWindow->update();
    masterWindow->draw();
  }
}
