#include <map>

#include "ep_gui.hpp"

namespace EP {
  namespace FlowGUI {
    class Entity;
    static std::map<EP::GUI::Block*,Entity*> blockToEntity;
    class Entity {
    public:
      Entity(EP::GUI::Block* block) {
        blockToEntity.insert({block,this});
      }
      ~Entity() {
        blockToEntity.erase(block_);
      }
      EP::GUI::Block* block() {return block_;}
      void socketInIs(EP::GUI::Socket* socket) {inSockets_.push_front(socket);}
      void socketOutIs(EP::GUI::Socket* socket) {outSockets_.push_front(socket);}
      std::list<EP::GUI::Socket*>& outSockets() {return outSockets_;}
    protected:
      EP::GUI::Block* block_;
      std::list<EP::GUI::Socket*> inSockets_;
      std::list<EP::GUI::Socket*> outSockets_;
    };
    static bool canReach(Entity* e1,Entity* e2) {
      if (e1==e2) {return true;}
      for (auto &sOut : e1->outSockets()) {
        for (auto &sIn : sOut->sink()) {
          if(EP::GUI::Block* b = dynamic_cast<EP::GUI::Block*>(sIn->parent())) {
            if (Entity* e = blockToEntity[b]) {
              if (canReach(e,e2)) {return true;}// found it!

            }
          }
        }
      }
      return false;
    }

    // void canTakeSinkIs(std::function<bool()> f) {canTakeSink=f;}
    // void canMakeSourceIs(std::function<bool()> f) {canMakeSource=f;}
    // void onSinkIsIs(std::function<void(Socket*)> f) {onSinkIs=f;}
    // void onSinkDelIs(std::function<void(Socket*)> f) {onSinkDel=f;}
    // void onSourceIsIs(std::function<void(Socket*)> f) {onSourceIs=f;}
    // void onSourceDelIs(std::function<void(Socket*)> f) {onSourceDel=f;}

    static EP::GUI::Socket* newInSocket(EP::GUI::Block* const parent,const float x,const float y,std::string name) {
      EP::GUI::Socket* socket = new EP::GUI::Socket("socketIn"+name,parent,x,y,EP::GUI::Socket::Direction::Up,name);
      socket->canTakeSinkIs([]() {return true;});
      socket->canMakeSourceIs([](EP::GUI::Socket* sink) {return false;});
      return socket;
    }
    static EP::GUI::Socket* newOutSocket(EP::GUI::Block* const parent,const float x,const float y,std::string name) {
      EP::GUI::Socket* socket = new EP::GUI::Socket("socketOut"+name,parent,x,y,EP::GUI::Socket::Direction::Down,name);
      socket->canTakeSinkIs([]() {return false;});
      socket->canMakeSourceIs([parent](EP::GUI::Socket* sink) {
        return !canReach(
          blockToEntity[dynamic_cast<EP::GUI::Block*>(sink->parent())],
          blockToEntity[parent]
        );
      });
      return socket;
    }

    static EP::GUI::Block* newBlockFM(EP::GUI::Area* const parent,const float x,const float y) {
      EP::GUI::Block* block = new EP::GUI::Block("blockFM",parent,x,y,100,100,EP::Color(0.5,0.5,1));
      Entity* e = new Entity(block);
      e->socketInIs(newInSocket(block,5,5,"Fq"));
      e->socketInIs(newInSocket(block,30,5,"Mod"));
      e->socketOutIs(newOutSocket(block,5,65,"Out"));
      EP::GUI::Label* label = new EP::GUI::Label("label",block,5,40,20,"FM Osc.",EP::Color(0.5,0.5,1));
      return block;
    }

    static EP::GUI::Window* newBlockTemplateWindow(EP::GUI::Area* const parent) {
      EP::GUI::Window* window = new EP::GUI::Window("blockTemplateWindow",parent,10,10,300,200,"Block Templates");

      EP::GUI::Area* content = new EP::GUI::Area("blockHolder",NULL,0,0,100,400);
      EP::GUI::BlockTemplate* btFM = new EP::GUI::BlockTemplate("blockTemplateFM",content,5,5,90,20,"FM Osc.");
      btFM->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        newBlockFM(bh,x,y);
      });

      EP::GUI::BlockTemplate* btBlub = new EP::GUI::BlockTemplate("blockTemplateBlub",content,5,30,90,20,"Blub");

      EP::GUI::Area* scroll = new EP::GUI::ScrollArea("scroll",window,content,0,0,100,100);
      content->colorIs(EP::Color(0.1,0,0));
      scroll->fillParentIs(true);
    }
  }
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
    EP::GUI::Window* window2 = new EP::GUI::Window("window2",masterWindow->area(),300,150,300,200,"Designer");
    {
      float x,y,dx,dy;
      window2->childSize(dx,dy);
      window2->childOffset(x,y);
      EP::GUI::Area* content1 = new EP::GUI::BlockHolder("blockHolder1",NULL,0,0,2000,2000);
      content1->colorIs(EP::Color(0.1,0.1,0.2));
      EP::GUI::Area* scroll1 = new EP::GUI::ScrollArea("scroll1",window2, content1,x,y,dx,dy);
      scroll1->fillParentIs(true);
      //EP::GUI::Area* button1 = new EP::GUI::Button("button1",content1,10,10,100,20,"X");
      //EP::GUI::Area* button2 = new EP::GUI::Button("button2",content1,300,300,100,20,"Y");
    }

    EP::FlowGUI::newBlockTemplateWindow(masterWindow->area());
  }
  while (masterWindow->isAlive()) {
    masterWindow->update();
    masterWindow->draw();
  }
}
