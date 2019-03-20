#include <map>
#include <mutex>
#include <SFML/Audio.hpp>

#include "ep_gui.hpp"

namespace EP {
  namespace FlowGUI {
    class Entity;

    // Ideas:
    // when to compile? how recompile?
    // list of tasks. each one has in (from task*, port), out.
    // both in/out ports are numbered -> clear reference, double vector
    // internal processing/storage allowed.
    // list makes sure the tasks are ordered.
    // recompiled incrementally, on events.

    // Task: on purpose separate, so that can be exported without too many dependencies.

    class Task {
    public:
      Task(const size_t inSize,const size_t outSize) {
        inTask_.resize(inSize,NULL);
        inTaskPort_.resize(inSize,0);
        inValue_.resize(inSize,0);
        out_.resize(outSize,0);
      }
      inline void fetchInput() {// runs before tick of this task, and after tick of dependency tasks
        for (size_t i = 0; i < inValue_.size(); i++) {
          if (inTask_[i]!=NULL) {inValue_[i]=inTask_[i]->out(inTaskPort_[i]);}
        }
      }
      virtual void tick(const double dt) = 0;// calculate output from input and internal state
      inline double out(const size_t i) {return out_[i];}
      void inIs(const size_t i,Task* const task,const size_t port) {inTask_[i]=task;inTaskPort_[i]=port;inValue_[i]=0;}
      void inIs(const size_t i,const double value) {inTask_[i]=NULL;inTaskPort_[i]=0;inValue_[i]=value;}
      bool inHasTask(const size_t i) {return inTask_[i]!=NULL;}
      void print() {
        std::cout << "{ " << this << std::endl;
        for (size_t i = 0; i < inTask_.size(); i++) {
          std::cout << "  in " << i << " " << inTask_[i] << " " << inTaskPort_[i] << " " << inValue_[i] << std::endl;
        }
        for (size_t i = 0; i < out_.size(); i++) {
          std::cout << "  out " << i << " " << out_[i] << std::endl;
        }
        std::cout << "}" << std::endl;
      }
      std::vector<Task*>& inTask() {return inTask_;}
    protected:
      std::vector<Task*> inTask_;
      std::vector<size_t> inTaskPort_;
      std::vector<double> inValue_;// set if inTask_!=NULL
      std::vector<double> out_;
    };

    static double audioOut;
    static std::vector<Task*> taskList;
    std::mutex taskListMutex;

    class TaskOscillator : public Task {
    public:
      enum In : size_t {Freq=0, Mod=1, ModFactor=2};
      enum Out : size_t {Signal=0};
      TaskOscillator() : Task(3,1),t_(0) {}
      virtual void tick(const double dt) {
        t_+=dt*(inValue_[In::Freq]+inValue_[In::Freq]*inValue_[In::Mod]*inValue_[In::ModFactor])*2.0*M_PI;
        out_[Out::Signal] =std::sin(t_);
      }
    protected:
      double t_ = 0;
    };
    class TaskAudioOut : public Task {
    public:
      enum In : size_t {Signal=0};
      TaskAudioOut() : Task(1,0) {}
      virtual void tick(const double dt) {
        audioOut = inValue_[In::Signal];// send signal to out.
      }
    protected:
    };
    class TaskTrigger : public Task {
    public:
      enum Out : size_t {Signal=0};
      TaskTrigger() : Task(0,1) {}
      virtual void tick(const double dt) {
        out_[Out::Signal] = trigger_?1:0;
        trigger_=false;
      }
      void setTrigger() {trigger_=true;}
    protected:
      bool trigger_=false;
    };

    class TaskInputValue : public Task {
    public:
      enum Out : size_t {Signal=0};
      TaskInputValue() : Task(0,1) {}
      virtual void tick(const double dt) {
        out_[Out::Signal] = value_;
      }
      void setValue(double const value) {value_=value;}
    protected:
      double value_;
    };

    class TaskMultiplyAndAdd : public Task {
    public:
      enum In : size_t {Input=0,Factor=1,Shift=2};
      enum Out : size_t {Signal=0};
      TaskMultiplyAndAdd() : Task(3,1) {}
      virtual void tick(const double dt) {
        out_[Out::Signal] = inValue_[In::Input]*inValue_[In::Factor] + inValue_[In::Shift];
      }
      void setValue(double const value) {value_=value;}
    protected:
      double value_;
    };

    class TaskKeyPad : public Task {
    public:
      enum In : size_t {Clock=0,InFreq=1};
      enum Out : size_t {Signal=0,OutFreq=1};
      TaskKeyPad(size_t numRows, size_t numCols) : Task(2,2),numCols_(numCols),numRows_(numRows) {
        adjustGrid();
      }
      virtual void tick(const double dt) {
        const double thisPulse = inValue_[In::Clock];
        if (lastPulse_<=0 and thisPulse>0) {// rising edge above 0
          lastCol_++;
          if (lastCol_>=numCols_) {lastCol_=0;}
          out_[Out::OutFreq] = inValue_[In::InFreq];

          if (cell(0,lastCol_)) {out_[Out::Signal] = 1.0;}
        } else {
          out_[Out::Signal] = 0;
        }
        lastPulse_=thisPulse;
      }
      void adjustGrid() {
        grid_.resize(numCols_);
        for (size_t i = 0; i < numCols_; i++) {
          grid_[i].resize(numRows_);
        }
      }
      void invertCell(size_t row, size_t col) {grid_[col][row] = not grid_[col][row];}
      bool cell(size_t row, size_t col) {return grid_[col][row];}
      size_t numCols() {return numCols_;}
      size_t numRows() {return numRows_;}
      size_t currColl() {return lastCol_;}
    protected:
      size_t numRows_,numCols_;
      std::vector<std::vector<bool>> grid_;
      size_t lastCol_=0;
      double lastPulse_=0;
    };

    class TaskEnvelope : public Task {
    public:
      enum In : size_t {Pulse=0,Input=1,Attack=2,Decay=3,Sustain=4,SustainAmp=5,Release=6};
      enum Out : size_t {Amplitude=0,Output=1};
      TaskEnvelope() : Task(7,2) {}
      virtual void tick(const double dt) {
        t_+=dt;
        const double thisPulse = inValue_[In::Pulse];
        if (lastPulse_<=0 and thisPulse>0) {// rising edge above 0
          t_=0;
        }
        const double attack = inValue_[In::Attack];
        const double decay = inValue_[In::Decay];
        const double sustain = inValue_[In::Sustain];
        const double sustainAmp = inValue_[In::SustainAmp];
        const double release = inValue_[In::Release];
        double amplitude = 0;
        if (t_<attack) {
          amplitude = t_/attack;
        } else {
          const double t2 = t_-attack;
          if (t2<decay) {
            const double factor = t2/decay;
            amplitude = (1.0-factor)+factor*sustainAmp;
          } else {
            const double t3 = t2-decay;
            if (t3<sustain) {
              amplitude = sustainAmp;
            } else {
              const double t4 = t3-sustain;
              if (t4<release) {
                const double factor = t4/release;
                amplitude = (1.0-factor)*sustainAmp;
              }
            }
          }
        }
        out_[Out::Amplitude] = amplitude;
        out_[Out::Output] = amplitude*inValue_[In::Input];
        lastPulse_=thisPulse;
      }
    protected:
      double lastPulse_=0;
      double t_ = 100;// far in future where zero
    };

    class AudioOutStream : public sf::SoundStream {
    public:
      AudioOutStream()
      :sampleRate_(44100), sampleSize_(2000), amplitude_(30000)
      {
        initialize(1, sampleRate_);
      }
    private:
      const unsigned sampleRate_;
      const unsigned sampleSize_;
      const unsigned amplitude_;

      std::vector<sf::Int16> samples;
      //
      // std::mutex instruments_mutex;
      // std::forward_list<Instrument*> instruments;

      virtual bool onGetData(Chunk& data)
      {
        samples.clear();
        const double dt = (double)1.0 / (double)sampleRate_;

        std::lock_guard<std::mutex> lock(taskListMutex);
        for (size_t i = 0; i < sampleSize_; i++) {
          for (auto &t : taskList) {
            t->fetchInput();
            t->tick(dt);
          }
          samples.push_back(amplitude_ * audioOut);
        }

        data.samples = &samples[0];
        data.sampleCount = sampleSize_;
        return true;
      }

      virtual void onSeek(sf::Time timeOffset)
      {
        // not supported.
      }
    };


    static std::map<EP::GUI::Block*,Entity*> blockToEntity;

    struct TaskPort {
      TaskPort(Task* const task,const size_t port) : task(task),port(port) {}
      TaskPort() : TaskPort(NULL,0) {}

      Task* const task;
      const size_t port;
    };

    static std::map<EP::GUI::Socket*,TaskPort> socketToTaskPort;// for both in and out sockets.

    class Entity {
    public:
      Entity() {}
      ~Entity() {
        blockIs(NULL);
        for(auto &s : inSockets_) {socketToTaskPort.erase(s);}
        for(auto &s : outSockets_) {socketToTaskPort.erase(s);}
      }
      void blockIs(EP::GUI::Block* block) {
        if (block==block_) {return;}
        if (block_) {blockToEntity.erase(block_);}
        block_ = block;
        if (block_) {blockToEntity.insert({block_,this});}
      }
      EP::GUI::Block* block() {return block_;}
      EP::GUI::Socket* socketInIs(EP::GUI::Block* const parent,const float x,const float y,std::string name,const size_t port,std::function<double()> defaultVal) {
        EP::GUI::Socket* socket = new EP::GUI::Socket("socketIn"+name,parent,x,y,EP::GUI::Socket::Direction::Up,name);
        socketToTaskPort.insert({socket,TaskPort(task(),port)});
        socket->canTakeSinkIs([]() {return true;});
        socket->canMakeSourceIs([](EP::GUI::Socket* sink) {return false;});
        socket->onSourceIsIs([this,port](EP::GUI::Socket* source) {
          std::cout << "sourceIs" << std::endl;
          const TaskPort taskPort = socketToTaskPort[source];
          {
            std::lock_guard<std::mutex> lock(taskListMutex);
            task()->inIs(port,taskPort.task,taskPort.port);
          }
          recompileTasks();
        });
        socket->onSourceDelIs([this,port,defaultVal](EP::GUI::Socket* source) {
          std::cout << "sourceDel" << std::endl;
          {
            std::lock_guard<std::mutex> lock(taskListMutex);
            task()->inIs(port,defaultVal());
          }
          recompileTasks();
        });
        inSockets_.push_front(socket);
        return socket;
      }
      void packageInIs(EP::GUI::Block* const block,const float x,const float y,std::string name,
                       Task* const task,const size_t port,std::function<double(double)> sliderToVal,const double sliderDefault=1.0) {
        EP::GUI::Label*  label = new EP::GUI::Label("label"+name,block,x,y+30,10,"XXX",EP::Color(1,1,1));
        EP::GUI::Slider* slider = new EP::GUI::Slider("slider"+name,block,x,y+45,20,50,false,0.0,1.0,0,0.2);
        slider->onValIs([label,task,sliderToVal,port](float val) {
          label->textIs(std::to_string(int(sliderToVal(val))));
          std::lock_guard<std::mutex> lock(taskListMutex);
          if (!task->inHasTask(port)) {
            task->inIs(port,sliderToVal(val));
          }
        });
        slider->valIs(sliderDefault);
        EP::GUI::Socket* socket = socketInIs(block,x,y,name,port,[slider]() {return slider->val();});
      }
      EP::GUI::Socket* socketOutIs(EP::GUI::Block* const parent,const float x,const float y,std::string name,const size_t port) {
        EP::GUI::Socket* socket = new EP::GUI::Socket("socketOut"+name,parent,x,y,EP::GUI::Socket::Direction::Down,name);
        socketToTaskPort.insert({socket,TaskPort(task(),port)});
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
      virtual Task* task()  = 0; // override and return corresponding Task

      static void recompileTasks() {
        std::cout << "recompileTasks" << std::endl;
        std::map<Task*,int> taskToDependencies;
        std::vector<Task*> newTaskList;

        for (auto &it : blockToEntity) {// make entry for each task.
          Task* const t = it.second->task();
          taskToDependencies.insert({t,0});
        }
        for (auto &it : blockToEntity) {// count dependencies
          Task* const t = it.second->task();
          std::vector<Task*> &inTask = t->inTask();
          for (auto &t2 : inTask) {
            if (t2!=NULL) {taskToDependencies[t2]++;}
          }
        }
        std::function<void(Task*)> process = [&taskToDependencies,&newTaskList,&process](Task* t) {
          taskToDependencies[t] = -1;
          newTaskList.push_back(t);
          std::vector<Task*> &inTask = t->inTask();
          for (auto &t2 : inTask) {
            if (t2!=NULL) {
              taskToDependencies[t2]--;
              if (taskToDependencies[t2]==0) {process(t2);}
            }
          }
        };
        for (auto &it : blockToEntity) {// bfs each dag
          Task* const t = it.second->task();
          if (taskToDependencies[t]==0) {process(t);}
        }
        std::reverse(newTaskList.begin(),newTaskList.end());
        {
          std::lock_guard<std::mutex> lock(taskListMutex);
          newTaskList.swap(taskList);
        }
      }
    protected:
      EP::GUI::Block* block_ = NULL;
      std::list<EP::GUI::Socket*> inSockets_;
      std::list<EP::GUI::Socket*> outSockets_;
    };

    class EntityFM : Entity {
    public:
      EntityFM(EP::GUI::Area* const parent,const float x,const float y) {
        taskOsc_ = new TaskOscillator();

        EP::GUI::Block* block = new EP::GUI::Block("blockFM",parent,x,y,100,200,EP::Color(0.5,0.5,1));
        blockIs(block);

        packageInIs(block,5 ,5,"Fq" ,task(),TaskOscillator::In::Freq,[](double in) {return (1.0-in)*440.0+440.0;},1.0);
        packageInIs(block,30,5,"Mod",task(),TaskOscillator::In::Mod,[](double in) {return (1.0-in)*2.0-1.0;},0.5);
        packageInIs(block,55,5,"Fac",task(),TaskOscillator::In::ModFactor,[](double in) {return (1.0-in)*1.0;},1.0);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,105,20,"FM Osc.",EP::Color(0.5,0.5,1));

        socketOutIs(block,5,150,"Out",TaskOscillator::Out::Signal);
      }
      virtual Task* task() {return taskOsc_;}
    protected:
      TaskOscillator* taskOsc_;
    };

    class EntityLFO : Entity {
    public:
      EntityLFO(EP::GUI::Area* const parent,const float x,const float y) {
        taskOsc_ = new TaskOscillator();

        EP::GUI::Block* block = new EP::GUI::Block("blockLFO",parent,x,y,100,200,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        packageInIs(block,5 ,5,"Fq" ,task(),TaskOscillator::In::Freq,[](double in) {return (1.0-in)*10.0;},1.0);
        packageInIs(block,30,5,"Mod",task(),TaskOscillator::In::Mod,[](double in) {return (1.0-in)*2.0-1.0;},0.5);
        packageInIs(block,55,5,"Fac",task(),TaskOscillator::In::ModFactor,[](double in) {return (1.0-in)*1.0;},1.0);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,105,20,"LFO Osc.",EP::Color(0.5,0.5,1));

        socketOutIs(block,5,150,"Out",TaskOscillator::Out::Signal);
      }
      virtual Task* task() {return taskOsc_;}
    protected:
      TaskOscillator* taskOsc_;
    };

    class EntityTrigger : Entity {
    public:
      EntityTrigger(EP::GUI::Area* const parent,const float x,const float y) {
        taskTrigger_ = new TaskTrigger();

        EP::GUI::Block* block = new EP::GUI::Block("blockTrigger",parent,x,y,100,100,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,5,20,"Trigger",EP::Color(0.5,0.5,1));

        EP::GUI::Button* button = new EP::GUI::Button("buttonTrigger",block,5,5,90,20,"Trigger");
        button->onClickIs([this]() {taskTrigger_->setTrigger();});

        socketOutIs(block,5,50,"Out",TaskOscillator::Out::Signal);
      }
      virtual Task* task() {return taskTrigger_;}
    protected:
      TaskTrigger* taskTrigger_;
    };

    class EntityValuePicker : Entity {
    public:
      EntityValuePicker(EP::GUI::Area* const parent,const float x,const float y) {
        taskInputValue_ = new TaskInputValue();

        EP::GUI::Block* block = new EP::GUI::Block("blockValuePicker",parent,x,y,55,200,EP::Color(0.5,0.5,0.1));
        blockIs(block);

        EP::GUI::Slider* sliderExp = new EP::GUI::Slider("expSlider",block,5,5,20,150,false,0.0,1.0,0.5,0.1);
        EP::GUI::Slider* sliderLin = new EP::GUI::Slider("linSlider",block,30,5,20,150,false,0.0,1.0,1,0.1);
        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,160,15,"XXX",EP::Color(1,1,0.5));

        std::function<void(double)> recomputeVal = [this,sliderExp,sliderLin,label](double disregard){
          const double val = std::pow(10.0,(1.0-sliderExp->val())*10.0-5.0)*((1.0-sliderLin->val())*9.0+1.0);
          taskInputValue_->setValue(val);
          label->textIs(std::to_string(val));
        };

        sliderExp->onValIs(recomputeVal);
        sliderLin->onValIs(recomputeVal);

        recomputeVal(0);

        socketOutIs(block,5,170,"Out",TaskInputValue::Out::Signal);
      }
      virtual Task* task() {return taskInputValue_;}
    protected:
      TaskInputValue* taskInputValue_;
    };


    class EntityEnvelope : Entity {
    public:
      EntityEnvelope(EP::GUI::Area* const parent,const float x,const float y) {
        taskEnvelope_ = new TaskEnvelope();

        EP::GUI::Block* block = new EP::GUI::Block("blockEnvelope",parent,x,y,200,100,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,5,20,"Envelope",EP::Color(0.5,0.5,1));

        socketInIs(block,5,5,"Main",TaskEnvelope::In::Pulse,[]() {return 0;});
        socketInIs(block,30,5,"In",TaskEnvelope::In::Input,[]() {return 0;});

        packageInIs(block,55 ,5,"A" ,task(),TaskEnvelope::In::Attack,[](double in) {return (1.0-in)*0.5;},0.1);
        packageInIs(block,80 ,5,"D" ,task(),TaskEnvelope::In::Decay,[](double in) {return (1.0-in)*1.0;},0.3);
        packageInIs(block,105,5,"S" ,task(),TaskEnvelope::In::Sustain,[](double in) {return (1.0-in)*2.0;},0);
        packageInIs(block,130,5,"SA",task(),TaskEnvelope::In::SustainAmp,[](double in) {return (1.0-in)*1.0;},0.5);
        packageInIs(block,155,5,"R" ,task(),TaskEnvelope::In::Release,[](double in) {return (1.0-in)*2.0;},0.1);

        socketOutIs(block,5,50,"Amp",TaskEnvelope::Out::Amplitude);
        socketOutIs(block,30,50,"Out",TaskEnvelope::Out::Output);
      }
      virtual Task* task() {return taskEnvelope_;}
    protected:
      TaskEnvelope* taskEnvelope_;
    };

    class EntityMultiplyAndAdd : Entity {
    public:
      EntityMultiplyAndAdd(EP::GUI::Area* const parent,const float x,const float y) {
        task_ = new TaskMultiplyAndAdd();

        EP::GUI::Block* block = new EP::GUI::Block("blockMultiplyAndAdd",parent,x,y,80,100,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,5,20,"Mult Add",EP::Color(0.5,0.5,1));

        socketInIs(block,5,5,"In",TaskMultiplyAndAdd::In::Input,[]() {return 0;});
        socketInIs(block,30,5,"Fact",TaskMultiplyAndAdd::In::Factor,[]() {return 0;});
        socketInIs(block,55,5,"Add",TaskMultiplyAndAdd::In::Shift,[]() {return 0;});

        socketOutIs(block,5,50,"Out",TaskMultiplyAndAdd::Out::Signal);
      }
      virtual Task* task() {return task_;}
    protected:
      TaskMultiplyAndAdd* task_;
    };

    class EntityKeyPad : Entity {
    public:
      EntityKeyPad(EP::GUI::Area* const parent,const float x,const float y) {
        task_ = new TaskKeyPad(4,16);

        EP::GUI::Block* block = new EP::GUI::Block("blockKeyPad",parent,x,y,300,200,EP::Color(0.2,0.1,0.1));
        blockIs(block);

        socketInIs(block,5,5,"Clk",TaskKeyPad::In::Clock,[]() {return 0;});
        socketInIs(block,30,5,"Fq",TaskKeyPad::In::InFreq,[]() {return 0;});

        buttons_.resize(task_->numCols());
        const double d = 10;
        for (size_t c = 0; c < task_->numCols(); c++) {
          buttons_[c].resize(task_->numRows());
          for (size_t r = 0; r < task_->numRows(); r++) {
            buttons_[c][r] = new EP::GUI::Button("button",block,5+c*d,50+r*d,d-1,d-1,"");
            buttons_[c][r]->onClickIs([this,r,c]() {task_->invertCell(r,c);});
          }
        }

        block->onDrawIs([this]() {
          size_t currentCol = task_->currColl();
          for (size_t c = 0; c < task_->numCols(); c++) {
            for (size_t r = 0; r < task_->numRows(); r++) {
              const bool cell = task_->cell(r,c);
              buttons_[c][r]->buttonColorIs(std::vector<EP::Color>{
                EP::Color(currentCol==c,cell,0.5),
                EP::Color(currentCol==c,cell,0.4),
                EP::Color(currentCol==c,cell,0.3),
                EP::Color(currentCol==c,cell,0.2)
              });
            }
          }
        });

        socketOutIs(block,5,170,"Sig",TaskKeyPad::Out::Signal);
        socketOutIs(block,30,170,"Fq",TaskKeyPad::Out::OutFreq);
      }
      virtual Task* task() {return task_;}
    protected:
      std::vector<std::vector<EP::GUI::Button*>> buttons_;
      TaskKeyPad* task_;
    };

    class EntityAudioOut : Entity {
    public:
      EntityAudioOut(EP::GUI::Area* const parent,const float x,const float y) {
        taskAudioOut_ = new TaskAudioOut();

        EP::GUI::Block* block = new EP::GUI::Block("blockAudioOut",parent,x,y,100,100,EP::Color(1,0.5,0.5));
        blockIs(block);
        socketMain_  = socketInIs(block,5,5,"Main",TaskAudioOut::In::Signal,[]() {return 0;});
        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,40,20,"Audio Out",EP::Color(1,0.5,0.5));
      }
      virtual Task* task() {return taskAudioOut_;}
    protected:
      EP::GUI::Socket* socketMain_=NULL;
      TaskAudioOut* taskAudioOut_;
    };

    static EP::GUI::Window* newBlockTemplateWindow(EP::GUI::Area* const parent) {
      EP::GUI::Window* window = new EP::GUI::Window("blockTemplateWindow",parent,10,10,300,200,"Block Templates");

      EP::GUI::Area* content = new EP::GUI::Area("templateHolder",NULL,0,0,100,400);
      EP::GUI::BlockTemplate* btFM = new EP::GUI::BlockTemplate("blockTemplateFM",content,5,5,90,20,"FM Osc.");
      btFM->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        new EntityFM(bh,x,y);
      });
      EP::GUI::BlockTemplate* btLFO = new EP::GUI::BlockTemplate("blockTemplateLFO",content,5,30,90,20,"LFO Osc.");
      btLFO->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        new EntityLFO(bh,x,y);
      });
      EP::GUI::BlockTemplate* btAudioOut = new EP::GUI::BlockTemplate("blockTemplateAudioOut",content,5,55,90,20,"Audio Out");
      btAudioOut->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        new EntityAudioOut(bh,x,y);
      });
      EP::GUI::BlockTemplate* btTrigger = new EP::GUI::BlockTemplate("blockTrigger",content,5,80,90,20,"Trigger");
      btTrigger->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        new EntityTrigger(bh,x,y);
      });
      EP::GUI::BlockTemplate* btEnvelope = new EP::GUI::BlockTemplate("blockEnvelope",content,5,105,90,20,"Envelope");
      btEnvelope->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        new EntityEnvelope(bh,x,y);
      });
      EP::GUI::BlockTemplate* btValuePicker = new EP::GUI::BlockTemplate("blockValuePicker",content,5,130,90,20,"V Picker");
      btValuePicker->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        new EntityValuePicker(bh,x,y);
      });

      EP::GUI::BlockTemplate* btMultAndAdd = new EP::GUI::BlockTemplate("blockMultAndAdd",content,5,155,90,20,"Mult Add");
      btMultAndAdd->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        new EntityMultiplyAndAdd(bh,x,y);
      });

      EP::GUI::BlockTemplate* btKeyPad = new EP::GUI::BlockTemplate("blockKeyPad",content,5,180,90,20,"KeyPad");
      btKeyPad->doInstantiateIs([](EP::GUI::BlockHolder* bh,const float x,const float y) {
        new EntityKeyPad(bh,x,y);
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
    EP::GUI::Window* window2 = new EP::GUI::Window("window2",masterWindow->area(),300,150,600,400,"Designer");
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
  // ------------------------------------ AUDIO
  EP::FlowGUI::AudioOutStream audioOutStream;
  audioOutStream.play();
  // ------------------------------------ MAIN
  while (masterWindow->isAlive()) {
    masterWindow->update();
    masterWindow->draw();
  }
}
