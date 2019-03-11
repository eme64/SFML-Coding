#include <map>

#include "ep_gui.hpp"

namespace EP {
  namespace FlowGUI {
    class Entity;

    // Ideas:
    // when to compile? how recompile?
    // list of tasks. each one has in (from task*, port), out.
    // both in/out ports are numbered -> clear reference, float vector
    // internal processing/storage allowed.
    // list makes sure the tasks are ordered.
    // recompiled incrementally, on events.

    static std::map<EP::GUI::Block*,Entity*> blockToEntity;


    class Entity {
    public:
      Entity() {}
      ~Entity() {
        blockIs(NULL);
      }
      void blockIs(EP::GUI::Block* block) {
        if (block==block_) {return;}
        if (block_) {blockToEntity.erase(block_);}
        block_ = block;
        if (block_) {blockToEntity.insert({block_,this});}
      }
      EP::GUI::Block* block() {return block_;}
      EP::GUI::Socket* socketInIs(EP::GUI::Block* const parent,const float x,const float y,std::string name) {
        EP::GUI::Socket* socket = new EP::GUI::Socket("socketIn"+name,parent,x,y,EP::GUI::Socket::Direction::Up,name);
        socket->canTakeSinkIs([]() {return true;});
        socket->canMakeSourceIs([](EP::GUI::Socket* sink) {return false;});
        inSockets_.push_front(socket);
        return socket;
      }
      EP::GUI::Socket* socketOutIs(EP::GUI::Block* const parent,const float x,const float y,std::string name) {
        EP::GUI::Socket* socket = new EP::GUI::Socket("socketOut"+name,parent,x,y,EP::GUI::Socket::Direction::Down,name);
        socket->canTakeSinkIs([]() {return false;});
        socket->canMakeSourceIs([parent](EP::GUI::Socket* sink) {
          Entity* const e1 = blockToEntity[dynamic_cast<EP::GUI::Block*>(sink->parent())];
          Entity* const e2 = blockToEntity[parent];
          return !e1->canReach(e2);
        });
        outSockets_.push_front(socket);
        return socket;
      }
      std::list<EP::GUI::Socket*>& outSockets() {return outSockets_;}
      bool canReach(Entity* other) {
        if (this==other) {return true;}
        for (auto &sOut : this->outSockets()) {
          for (auto &sIn : sOut->sink()) {
            if(EP::GUI::Block* b = dynamic_cast<EP::GUI::Block*>(sIn->parent())) {
              if (Entity* e = blockToEntity[b]) {
                if (e->canReach(other)) {return true;}// found it!
              }
            }
          }
        }
        return false;
      }
    protected:
      EP::GUI::Block* block_ = NULL;
      std::list<EP::GUI::Socket*> inSockets_;
      std::list<EP::GUI::Socket*> outSockets_;
    };

    class EntityFM : Entity {
    public:
      EntityFM(EP::GUI::Area* const parent,const float x,const float y) {
        EP::GUI::Block* block = new EP::GUI::Block("blockFM",parent,x,y,100,200,EP::Color(0.5,0.5,1));
        blockIs(block);
        socketFq_  = socketInIs(block,5,5,"Fq");
        labelFq_ = new EP::GUI::Label("labelFq",block,5,35,10,"440",EP::Color(1,1,1));
        sliderFq_ = new EP::GUI::Slider("sliderFq",block,5,50,20,50,false,0.0,1.0,1.0,0.2);
        sliderFq_->onValIs([this](float val) {labelFq_->textIs(std::to_string(int(sliderFqFunction(val))));});

        socketMod_ = socketInIs(block,30,5,"Mod");

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,105,20,"FM Osc.",EP::Color(0.5,0.5,1));

        socketOut_ = socketOutIs(block,5,150,"Out");
      }
      float sliderFqFunction(float in) {return (1.0-in)*440.0+440.0;}
    protected:
      EP::GUI::Socket* socketFq_=NULL;
      EP::GUI::Slider* sliderFq_=NULL;
      EP::GUI::Label*  labelFq_ =NULL;

      EP::GUI::Socket* socketMod_=NULL;
      EP::GUI::Socket* socketOut_=NULL;
    };

    class EntityAudioOut : Entity {
    public:
      EntityAudioOut(EP::GUI::Area* const parent,const float x,const float y) {
        EP::GUI::Block* block = new EP::GUI::Block("blockAudioOut",parent,x,y,100,100,EP::Color(1,0.5,0.5));
        blockIs(block);
        socketMain_  = socketInIs(block,5,5,"Main");
        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,40,20,"Audio Out",EP::Color(1,0.5,0.5));
      }
    protected:
      EP::GUI::Socket* socketMain_=NULL;
    };

    static EP::GUI::Window* newBlockTemplateWindow(EP::GUI::Area* const parent) {
      EP::GUI::Window* window = new EP::GUI::Window("blockTemplateWindow",parent,10,10,300,200,"Block Templates");

      EP::GUI::Area* content = new EP::GUI::Area("templateHolder",NULL,0,0,100,400);
      EP::GUI::BlockTemplate* btFM = new EP::GUI::BlockTemplate("blockTemplateFM",content,5,5,90,20,"FM Osc.");
      btFM->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        new EntityFM(bh,x,y);
      });
      EP::GUI::BlockTemplate* btAudioOut = new EP::GUI::BlockTemplate("blockTemplateAudioOut",content,5,30,90,20,"Audio Out");
      btAudioOut->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        new EntityAudioOut(bh,x,y);
      });

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
