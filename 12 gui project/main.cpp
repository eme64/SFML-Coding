#include <map>
#include <queue>
#include <mutex>
#include <valarray>
#include <complex>
#include <SFML/Audio.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

#include "ep_gui.hpp"

namespace EP {
  namespace FFT {
    // ------------------------------------------------------ FFT
    const double PI  = 3.141592653589793238463;
    typedef std::complex<double> ComplexVal;
    typedef std::valarray<ComplexVal> SampleArray;
    void cooleyTukeyFFT(SampleArray& values) {
    	const size_t N = values.size();
    	if (N <= 1) return; //base-case

    	//separate the array of values into even and odd indices
    	SampleArray evens = values[std::slice(0, N / 2, 2)]; //slice starting at 0, N/2 elements, increment by 2
    	SampleArray odds = values[std::slice(1, N / 2, 2)]; //slice starting at 1

    	//now call recursively
    	cooleyTukeyFFT(evens);
    	cooleyTukeyFFT(odds);

    	//recombine
    	for(size_t i=0; i<N/2; i++) {
    		ComplexVal index = std::polar(1.0, -2 * PI*i / N) * odds[i];
    		values[i] = evens[i] + index;
    		values[i + N / 2] = evens[i] - index;
    	}
    }
  }

  std::vector<std::string> split(const std::string &text, char sep) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos) {
      tokens.push_back(text.substr(start, end - start));
      start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
  }

  namespace FlowGUI {
    class Entity;

    // Ideas:
    // when to compile? how recompile?
    // list of tasks. each one has in (from task*, port), out.
    // both in/out ports are numbered -> clear reference, double vector
    // internal processing/storage allowed.
    // list makes sure the tasks are ordered.
    // recompiled incrementally, on events.

    // ------------------------------------------------------ AUDIO QUEUE
    static std::queue<std::vector<double>> audioInBlocks;
    // push to enqueue block
    static size_t audioInBlocksOffset;
    static double audioInBlocksValueCached;
    static double audioInBlockValue(){
      if (audioInBlocks.empty()) {return 0;}
      std::vector<double> *front = &(audioInBlocks.front());
      audioInBlocksValueCached = (*front)[audioInBlocksOffset];
      return audioInBlocksValueCached;
    }
    static double audioInBlockNext(){
      if (audioInBlocks.empty()) {return 0;}
      std::vector<double> *front = &(audioInBlocks.front());
      audioInBlocksOffset++;
      if (audioInBlocksOffset>=front->size()) {
        audioInBlocksOffset = 0;
        audioInBlocks.pop();
      }
      return audioInBlockValue();
    }

    // Task: on purpose separate, so that can be exported without too many dependencies.
    class Task {
    public:
      Task(const size_t inSize,const size_t outSize) {
        inTask_.resize(inSize,NULL);
        inTaskPort_.resize(inSize,0);
        inValue_.resize(inSize,std::vector<double>{0});
        out_.resize(outSize,std::vector<double>{0});
      }
      inline void fetchInput() {// runs before tick of this task, and after tick of dependency tasks
        for (size_t i = 0; i < inValue_.size(); i++) {
          if (inTask_[i]!=NULL) {
            std::vector<double>& o = inTask_[i]->out(inTaskPort_[i]);
            if (inValue_[i].size()!=o.size()) {inValue_[i].resize(o.size());}
            for (size_t j = 0; j < o.size(); j++) {inValue_[i][j]=o[j];}
          }
        }
      }
      virtual void tick(const double dt) = 0;// calculate output from input and internal state
      inline std::vector<double>& out(const size_t i) {return out_[i];}
      void inIs(const size_t i,Task* const task,const size_t port) {inTask_[i]=task;inTaskPort_[i]=port;}//inValue_[i]=task->out(port);}
      void inIs(const size_t i,const std::vector<double> value) {inTask_[i]=NULL;inTaskPort_[i]=0;inValue_[i]=value;}
      bool inHasTask(const size_t i) {return inTask_[i]!=NULL;}
      void print() {
        std::cout << "{ " << this << std::endl;
        for (size_t i = 0; i < inTask_.size(); i++) {
          std::cout << "  in " << i << " " << inTask_[i] << " " << inTaskPort_[i] << " : ";
          for(auto& n: inValue_[i]) {std::cout << n << " ";}
          std::cout << std::endl;
        }
        for (size_t i = 0; i < out_.size(); i++) {
          std::cout << "  out " << i << " : ";
          for(auto& n: out_[i]) {std::cout << n << " ";}
          std::cout << std::endl;
        }
        std::cout << "}" << std::endl;
      }
      const std::vector<Task*>& inTask() const {return inTask_;}
      const std::vector<size_t>& inTaskPort() const {return inTaskPort_;}
      const std::vector<std::vector<double>>& inValue() const {return inValue_;}
    protected:
      std::vector<Task*> inTask_;
      std::vector<size_t> inTaskPort_;
      std::vector<std::vector<double>> inValue_;// set if inTask_!=NULL
      std::vector<std::vector<double>> out_;
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
        const size_t sFreq = inValue_[In::Freq].size();
        const size_t sMod = inValue_[In::Mod].size();
        const size_t sModFactor = inValue_[In::ModFactor].size();
        const size_t maxSize = std::max(sFreq,std::max(sMod,sModFactor));

        if (t_.size()!=maxSize) {t_.resize(maxSize,0);}
        if (out_[Out::Signal].size()!=maxSize) {out_[Out::Signal].resize(maxSize,0);}

        for (size_t i = 0; i < maxSize; i++) {
          const float freq = inValue_[In::Freq][sFreq==maxSize?i:0];
          const float mod = inValue_[In::Mod][sMod==maxSize?i:0];
          const float modFactor = inValue_[In::ModFactor][sModFactor==maxSize?i:0];

          t_[i]+=dt*(freq+freq*mod*modFactor)*2.0*M_PI;
          out_[Out::Signal][i] =std::sin(t_[i]);
        }
      }
    protected:
      std::vector<double> t_{0};
    };
    class TaskAudioOut : public Task {
    public:
      enum In : size_t {Signal=0,Amplitude=1};
      TaskAudioOut() : Task(2,0) {}
      virtual void tick(const double dt) {
        double sum = 0;
        if (inValue_[In::Signal].size()==inValue_[In::Amplitude].size()) {
          for (size_t i = 0; i < inValue_[In::Signal].size(); i++) {
            sum+=inValue_[In::Signal][i]*inValue_[In::Amplitude][i];
          }
        }else{
          for(auto& n : inValue_[In::Signal]){sum+=n;}
          sum*=inValue_[In::Amplitude][0];
        }

        audioOut = sum;// send signal to out.
      }
    protected:
    };
    class TaskTrigger : public Task {
    public:
      enum Out : size_t {Signal=0};
      TaskTrigger() : Task(0,1) {}
      virtual void tick(const double dt) {
        out_[Out::Signal][0] = trigger_?1:0;
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
        // nothing
      }
      void setValue(const std::vector<double> value) {
        out_[Out::Signal].resize(value.size());
        for (size_t i = 0; i < value.size(); i++) {out_[Out::Signal][i]=value[i];}
      }
    protected:
    };

    class TaskAudioIn : public Task {
    public:
      enum Out : size_t {Signal=0};
      TaskAudioIn() : Task(0,1) {}
      virtual void tick(const double dt) {
        out_[Out::Signal].resize(1);
        out_[Out::Signal][0] = audioInBlocksValueCached;
      }
    protected:
    };

    class TaskQuantizer : public Task {
    public:
      enum In : size_t {Input=0,Chord=1};
      enum Out : size_t {Freq=0};
      TaskQuantizer() : Task(2,1) {}
      virtual void tick(const double dt) {
        const size_t sInput = inValue_[In::Input].size();
        const size_t sChord = inValue_[In::Chord].size();
        if (sChord==0) {
          out_[Out::Freq].resize(0);
          return; // abort
        }else if (sChord==1) {
          out_[Out::Freq].resize(1);
          out_[Out::Freq][0] = inValue_[In::Chord][0];
          return; // simple case
        }
        const double chordOctave = inValue_[In::Chord][sChord-1]/inValue_[In::Chord][0];

        if (out_[Out::Freq].size()!=sInput) {out_[Out::Freq].resize(sInput,0);}
        for (size_t i = 0; i < sInput; i++) {
          const size_t octave = int(std::floor(inValue_[In::Input][i]))/(sChord-1);
          const size_t offset = int(std::floor(inValue_[In::Input][i])) % (sChord-1);
          out_[Out::Freq][i] = std::pow(chordOctave,octave)*inValue_[In::Chord][offset];
        }
      }
    protected:
    };

    class TaskMultiplyAndAdd : public Task {
    public:
      enum In : size_t {Input=0,Factor=1,Shift=2};
      enum Out : size_t {Signal=0};
      TaskMultiplyAndAdd() : Task(3,1) {}
      virtual void tick(const double dt) {
        const size_t sInput = inValue_[In::Input].size();
        const size_t sFactor = inValue_[In::Factor].size();
        const size_t sShift = inValue_[In::Shift].size();
        const size_t maxSize = std::max(sInput,std::max(sFactor,sShift));

        if (out_[Out::Signal].size()!=maxSize) {out_[Out::Signal].resize(maxSize,0);}

        for (size_t i = 0; i < maxSize; i++) {
          const float input = inValue_[In::Input][sInput==maxSize?i:0];
          const float factor = inValue_[In::Factor][sFactor==maxSize?i:0];
          const float shift = inValue_[In::Shift][sShift==maxSize?i:0];
          out_[Out::Signal][i] = input*factor+shift;
        }
      }
    protected:
    };

    class TaskKeyPad : public Task {
    public:
      enum In : size_t {Clock=0};
      enum Out : size_t {Signal=0,Row=1};
      TaskKeyPad(size_t numRows, size_t numCols) : Task(1,2),numCols_(numCols),numRows_(numRows) {
        adjustGrid();
      }
      virtual void tick(const double dt) {
        const double thisPulse = inValue_[In::Clock][0];
        if (lastPulse_<=0 and thisPulse>0) {// rising edge above 0
          lastCol_++;
          if (lastCol_>=numCols_) {lastCol_=0;}

          for (size_t i = 0; i < numRows_; i++) {
            if (cell(i,lastCol_)) {out_[Out::Signal][i] = 1.0;}
          }

        } else {
          for (size_t i = 0; i < numRows_; i++) {
            out_[Out::Signal][i] = 0;
          }
        }
        lastPulse_=thisPulse;
      }
      void adjustGrid() {
        grid_.resize(numCols_);
        for (size_t i = 0; i < numCols_; i++) {
          grid_[i].resize(numRows_);
        }
        out_[Out::Signal].resize(numRows_,0);
        out_[Out::Row].resize(numRows_);
        for (size_t i = 0; i < numRows_; i++) {out_[Out::Row][i]=i;}
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
        const size_t sPulse = inValue_[In::Pulse].size();
        const size_t sInput = inValue_[In::Input].size();
        const size_t sAttack = inValue_[In::Attack].size();
        const size_t sDecay = inValue_[In::Decay].size();
        const size_t sSustain = inValue_[In::Sustain].size();
        const size_t sSustainAmp = inValue_[In::SustainAmp].size();
        const size_t sRelease = inValue_[In::Release].size();
        const size_t maxSize = std::max(std::max(sPulse,std::max(sInput,sAttack)),std::max(std::max(sDecay,sSustain),std::max(sSustainAmp,sRelease)));

        if (out_[Out::Amplitude].size()!=maxSize) {out_[Out::Amplitude].resize(maxSize,0);}
        if (out_[Out::Output].size()!=maxSize) {out_[Out::Output].resize(maxSize,0);}
        if (lastPulse_.size()!=maxSize) {lastPulse_.resize(maxSize,0);}
        if (t_.size()!=maxSize) {t_.resize(maxSize,100);}


        for (size_t i = 0; i < maxSize; i++) {
          const float thisPulse = inValue_[In::Pulse][sPulse==maxSize?i:0];
          const float inp = inValue_[In::Input][sInput==maxSize?i:0];
          const float attack = inValue_[In::Attack][sAttack==maxSize?i:0];
          const float decay = inValue_[In::Decay][sDecay==maxSize?i:0];
          const float sustain = inValue_[In::Sustain][sSustain==maxSize?i:0];
          const float sustainAmp = inValue_[In::SustainAmp][sSustainAmp==maxSize?i:0];
          const float release = inValue_[In::Release][sRelease==maxSize?i:0];

          t_[i]+=dt;
          const double lastPulse = lastPulse_[i];
          if (lastPulse<=0 and thisPulse>0) {// rising edge above 0
            t_[i]=0;
          }
          double amplitude = 0;

          if (t_[i]<attack) {
            amplitude = t_[i]/attack;
          } else {
            const double t2 = t_[i]-attack;
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
          out_[Out::Amplitude][i] = amplitude;
          out_[Out::Output][i] = amplitude*inp;
          lastPulse_[i]=thisPulse;
        }
      }
    protected:
      std::vector<double> lastPulse_{0};
      std::vector<double> t_{100};// far in future where zero
    };

    class TaskSignalAnalyzer : public Task {
    public:
      enum In : size_t {Signal=0};
      enum Out : size_t {Amplitude=0,Freq=0};
      TaskSignalAnalyzer() : Task(1,2) {}
      virtual void tick(const double dt) {
        // ensure buffer/output size
        const size_t sSignal = inValue_[In::Signal].size();
        if (buffers_.size()!=sSignal) {
          out_[Out::Amplitude].resize(sSignal,0);
          out_[Out::Freq].resize(sSignal,0);
          buffers_.resize(sSignal);
          for (size_t i = 0; i < sSignal; i++) {
            buffers_[i].resize(bufferSize_);
          }
          bufferOffset_=0;
        }

        // write to buffer
        for (size_t i = 0; i < sSignal; i++) {
          buffers_[i][bufferOffset_]=inValue_[In::Signal][i];
        }

        // advance offset, maybe update output
        bufferOffset_++;
        if (bufferOffset_%updateRate_==0) {
          if (bufferOffset_>=bufferSize_) {
            bufferOffset_=0;
          }
          EP::FFT::SampleArray fftBuffer(bufferSize_);
          for (size_t i = 0; i < sSignal; i++) {
            double maxAmp = 0;
            for (size_t j = bufferOffset_; j < bufferOffset_+updateRate_; j++) {
              maxAmp = std::max(maxAmp,std::abs(buffers_[i][j]));
            }
            out_[Out::Amplitude][i] = maxAmp;
            EP::FFT::SampleArray fftBuffer = buffers_[i].cshift(bufferOffset_);
            EP::FFT::cooleyTukeyFFT(fftBuffer);
            size_t m = 0;
            double maxVal = 0;
            for (size_t j = 0; j < bufferSize_/2; j++) {
              const double loc = std::abs(fftBuffer[j])+std::abs(fftBuffer[bufferSize_-j-1]);
              if (maxVal < loc) {m = j; maxVal = loc;}
            }
            const double fq = 1.0*(double)m/(double)bufferSize_/dt;
            out_[Out::Freq][i] = fq;
          }
        }

      }
    protected:
      const size_t updateRate_=256;
      const size_t bufferSize_=updateRate_*8;
      size_t bufferOffset_=0;
      std::vector<EP::FFT::SampleArray> buffers_;
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
          audioInBlockNext();

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

    class AudioInStream : public sf::SoundRecorder {
        virtual bool onStart() {
            std::cout << "Start Recording" << std::endl;
            setProcessingInterval(sf::milliseconds(0.1));
            return true;
        }

        virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount) {
          std::vector<double> block;
          block.reserve(sampleCount);
          for (size_t i = 0; i < sampleCount; i++) {
            block.push_back(samples[i]*0.0003);
          }
          audioInBlocks.push(block);
          return true;
        }

        virtual void onStop() {
            std::cout << "Stop" << std::endl;
        }
    public:
    private:
    };



    static std::map<EP::GUI::Block*,Entity*> blockToEntity;

    struct TaskPort {
      TaskPort(Task* const task,const size_t port) : task(task),port(port) {}
      TaskPort() : TaskPort(NULL,0) {}

      Task* const task;
      const size_t port;
    };

    static std::map<EP::GUI::Socket*,TaskPort> socketToTaskPort;// for both in and out sockets.

    struct EntityData {
      EntityData(size_t id,std::string type,float const x,float const y):id(id),type(type),x(x),y(y){}
      size_t id;
      std::string type;
      float x,y;
      std::map<std::string, std::vector<std::string>> values;
    };

    class Entity {
    public:
      Entity() {}
      ~Entity() {
        blockIs(NULL);
        for(auto &s : inSockets_) {socketToTaskPort.erase(s);}
        for(auto &s : outSockets_) {socketToTaskPort.erase(s);}
      }
      virtual void doDelete() {
        if (isDeleted_) {return;}// prevent multiple entry
        isDeleted_ = true;

        std::cout << "doDelete" << std::endl;
        block()->doDelete();
        std::cout << "blockIs NULL" << std::endl;
        blockIs(NULL);
        std::cout << "removeTask" << std::endl;
        std::lock_guard<std::mutex> lock(taskListMutex);
      }
      void blockIs(EP::GUI::Block* block) {
        if (block==block_) {return;}
        if (block_) {blockToEntity.erase(block_);}
        block_ = block;
        if (block_) {
          block_->onDeleteIs([this](EP::GUI::Area* const a) {
            if (a==block_) {
              doDelete();
            }
          });
          blockToEntity.insert({block_,this});
        }
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
            task()->inIs(port,std::vector<double>{defaultVal()});
          }
          recompileTasks();
        });
        inSockets_.push_back(socket);
        return socket;
      }
      void packageInIs(EP::GUI::Block* const block,const float x,const float y,std::string name,
                       Task* const task,const size_t port,EP::Function* func,double const value=1.0) {
        EP::GUI::Knob* knob = new EP::GUI::Knob("knob",block,x,y+30,35,35,func);
        knob->onValueIs([knob,task,port](double val) {
          std::lock_guard<std::mutex> lock(taskListMutex);
          if (!task->inHasTask(port)) {
            task->inIs(port,std::vector<double>{val});
          }
        });
        knob->valueIs(value);
        // EP::GUI::Label*  label = new EP::GUI::Label("label"+name,block,x,y+30,10,"XXX",EP::Color(1,1,1));
        // EP::GUI::Slider* slider = new EP::GUI::Slider("slider"+name,block,x,y+45,20,50,false,0.0,1.0,0,0.2);
        // slider->onValIs([label,task,sliderToVal,port](float val) {
        //   label->textIs(std::to_string(int(sliderToVal(val))));
        //   std::lock_guard<std::mutex> lock(taskListMutex);
        //   if (!task->inHasTask(port)) {
        //     task->inIs(port,std::vector<double>{sliderToVal(val)});
        //   }
        // });
        // slider->valIs(sliderDefault);
        EP::GUI::Socket* socket = socketInIs(block,x,y,name,port,[knob]() {return knob->value();});
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
        outSockets_.push_back(socket);
        return socket;
      }
      std::vector<EP::GUI::Socket*>& outSockets() {return outSockets_;}
      std::vector<EP::GUI::Socket*>& inSockets() {return inSockets_;}
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
          const std::vector<Task*> &inTask = t->inTask();
          for (auto &t2 : inTask) {
            if (t2!=NULL) {taskToDependencies[t2]++;}
          }
        }
        std::function<void(Task*)> process = [&taskToDependencies,&newTaskList,&process](Task* t) {
          taskToDependencies[t] = -1;
          newTaskList.push_back(t);
          const std::vector<Task*> &inTask = t->inTask();
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

        for (auto &t : newTaskList) {// bfs each dag
          t->print();
        }

        {
          std::lock_guard<std::mutex> lock(taskListMutex);
          newTaskList.swap(taskList);
        }
      }
      std::string serialize(std::map<Entity*, size_t> &entityToId, std::map<Task*, Entity*> &taskToEntity){
        const size_t id = entityToId[this];
        std::string ret = "@"+std::to_string(id)+" "+serializeName()+"\n";
        ret+="position="+std::to_string(block_->x())+","+std::to_string(block_->y())+"\n";
        Task* t = task();
        for (size_t i = 0; i < task()->inTask().size(); i++) {
          ret+="inTask_"+std::to_string(i)+"="
          +std::to_string(entityToId[taskToEntity[task()->inTask()[i]]])+","
          +std::to_string(task()->inTaskPort()[i])+"\n";
          ret+="inValue_"+std::to_string(i)+"=";
          for (size_t j = 0; j < task()->inValue()[i].size(); j++) {
            if (j>0) {ret+=",";}
            ret+=std::to_string(task()->inValue()[i][j]);
          }
          ret+="\n";
        }
        ret+=serializeData();
        return ret;
      }
      virtual std::string serializeName() {return "Entity";}
      virtual std::string serializeData() {return "";}
      void valueIs(size_t const port, std::vector<double> &values) {
        EP::GUI::Knob* s = inKnobs_[port];
        if (s) {
          s->valueIs(values[0]);
        }
      }
    protected:
      EP::GUI::Block* block_ = NULL;
      std::vector<EP::GUI::Socket*> inSockets_;//by port
      std::vector<EP::GUI::Socket*> outSockets_;//by port
      std::map<size_t, EP::GUI::Knob*> inKnobs_;//by port
      bool isDeleted_ = false;
    };

    class EntityFM : public Entity {
    public:
      EntityFM(EP::GUI::Area* const parent,EntityData* const data) {
        taskOsc_ = new TaskOscillator();

        EP::GUI::Block* block = new EP::GUI::Block("blockFM",parent,data->x,data->y,130,200,EP::Color(0.5,0.5,1));
        blockIs(block);

        packageInIs(block,5 ,5,"Fq" ,task(),TaskOscillator::In::Freq,new EP::FunctionExp(2,20000),440.0);
        packageInIs(block,45,5,"Mod",task(),TaskOscillator::In::Mod,new EP::FunctionLin(-1,1),0);
        packageInIs(block,85,5,"Fac",task(),TaskOscillator::In::ModFactor,new EP::FunctionLin(0,1),0);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,105,20,"FM Osc.",EP::Color(0.5,0.5,1));

        socketOutIs(block,5,150,"Out",TaskOscillator::Out::Signal);
      }
      virtual Task* task() {return taskOsc_;}
      virtual std::string serializeName() {return "EntityFM";}
    protected:
      TaskOscillator* taskOsc_;
    };

    class EntityLFO : public Entity {
    public:
      EntityLFO(EP::GUI::Area* const parent,EntityData* const data) {
        taskOsc_ = new TaskOscillator();

        EP::GUI::Block* block = new EP::GUI::Block("blockLFO",parent,data->x,data->y,130,200,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        packageInIs(block,5 ,5,"Fq" ,task(),TaskOscillator::In::Freq,new EP::FunctionExp(0.0001,100),1);
        packageInIs(block,45,5,"Mod",task(),TaskOscillator::In::Mod,new EP::FunctionLin(-1,1),0);
        packageInIs(block,85,5,"Fac",task(),TaskOscillator::In::ModFactor,new EP::FunctionLin(0,1),0);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,105,20,"LFO Osc.",EP::Color(0.5,0.5,1));

        socketOutIs(block,5,150,"Out",TaskOscillator::Out::Signal);
      }
      virtual Task* task() {return taskOsc_;}
      virtual std::string serializeName() {return "EntityLFO";}
    protected:
      TaskOscillator* taskOsc_;
    };

    class EntityTrigger : public Entity {
    public:
      EntityTrigger(EP::GUI::Area* const parent,EntityData* const data) {
        taskTrigger_ = new TaskTrigger();

        EP::GUI::Block* block = new EP::GUI::Block("blockTrigger",parent,data->x,data->y,100,100,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,5,20,"Trigger",EP::Color(0.5,0.5,1));

        EP::GUI::Button* button = new EP::GUI::Button("buttonTrigger",block,5,5,90,20,"Trigger");
        button->onClickIs([this]() {taskTrigger_->setTrigger();});

        socketOutIs(block,5,50,"Out",TaskOscillator::Out::Signal);
      }
      virtual Task* task() {return taskTrigger_;}
      virtual std::string serializeName() {return "EntityTrigger";}
    protected:
      TaskTrigger* taskTrigger_;
    };

    class EntityValuePicker : public Entity {
    public:
      EntityValuePicker(EP::GUI::Area* const parent,EntityData* const data) {
        taskInputValue_ = new TaskInputValue();

        EP::GUI::Block* block = new EP::GUI::Block("blockValuePicker",parent,data->x,data->y,50,85,EP::Color(0.5,0.5,0.1));
        blockIs(block);

        knob_ = new EP::GUI::Knob("knob",block,5,5,40,40,new EP::FunctionExp(0.00001,100000));
        std::function<void(double)> onValF = [this](double val){
          taskInputValue_->setValue(std::vector<double>{val});
        };

        knob_->onValueIs(onValF);
        knob_->valueIs(1);

        if (data->values.find("value") != data->values.end()) {
          knob_->valueIs(std::atof(data->values["value"][0].c_str()));
        }

        socketOutIs(block,5,50,"Out",TaskInputValue::Out::Signal);
      }
      virtual Task* task() {return taskInputValue_;}
      virtual std::string serializeName() {return "EntityValuePicker";}
      virtual std::string serializeData() {return "value="+std::to_string(knob_->value())+"\n";}
    protected:
      TaskInputValue* taskInputValue_;
      EP::GUI::Knob* knob_;
    };

    class EntityChordPicker : public Entity {
    public:
      EntityChordPicker(EP::GUI::Area* const parent,EntityData* const data) {
        taskInputValue_ = new TaskInputValue();

        EP::GUI::Block* block = new EP::GUI::Block("blockChordPicker",parent,data->x,data->y,200,70,EP::Color(0.5,0.1,0.1));
        blockIs(block);

        const double ddx = 15;
        const double ddy = 20;
        const size_t numFreq = 13;
        frequency_.resize(numFreq);
        buttons_.resize(numFreq);
        activated_.resize(numFreq,true);


        std::function<void(size_t)> flipButton = [this](const size_t i) {
          activated_[i] = not activated_[i];
          buttons_[i]->buttonColorIs(std::vector<EP::Color>{
            EP::Color(activated_[i]*0.6+0.4,0.3,0.1),
            EP::Color(activated_[i]*0.5+0.5,0.4,0.2),
            EP::Color(activated_[i]*0.4+0.6,0.4,0.2),
            EP::Color(activated_[i]*0.4+0.6,0.4,0.2),
          });
        };
        std::function<void(void)> recomputeValues = [this,numFreq]() {
          std::vector<double> tmp_;
          for (size_t i = 0; i < numFreq; i++) {
            if (activated_[i]) {
              tmp_.push_back(frequency_[i]);
            }
          }
          taskInputValue_->setValue(tmp_);
        };

        for (size_t i = 0; i < numFreq; i++) {
          frequency_[i] = 1*std::pow(2.0,(double)(i)/12.0);
          buttons_[i] = new EP::GUI::Button("button",block,5+i*ddx,5,ddx-1,ddy-1,"");
          buttons_[i]->onClickIs([this,i,flipButton,recomputeValues]() {
            flipButton(i);
            recomputeValues();
          });
          flipButton(i);
          if (i%12==0) {
            flipButton(i);
          }
        }
        recomputeValues();

        socketOutIs(block,5,30,"Out",TaskInputValue::Out::Signal);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,40,30,20,"Chord Picker",EP::Color(1,0.6,0.1));
      }
      virtual Task* task() {return taskInputValue_;}
      virtual std::string serializeName() {return "EntityChordPicker";}
    protected:
      TaskInputValue* taskInputValue_;
      std::vector<EP::GUI::Button*> buttons_;
      std::vector<double> frequency_;
      std::vector<bool> activated_;
    };


    class EntityEnvelope : public Entity {
    public:
      EntityEnvelope(EP::GUI::Area* const parent,EntityData* const data) {
        taskEnvelope_ = new TaskEnvelope();

        EP::GUI::Block* block = new EP::GUI::Block("blockEnvelope",parent,data->x,data->y,260,100,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,5,20,"Envelope",EP::Color(0.5,0.5,1));

        socketInIs(block,5,5,"Main",TaskEnvelope::In::Pulse,[]() {return 0;});
        socketInIs(block,30,5,"In",TaskEnvelope::In::Input,[]() {return 0;});

        packageInIs(block,55 ,5,"A" ,task(),TaskEnvelope::In::Attack,new EP::FunctionExp(0.0001,10),0.01);
        packageInIs(block,95 ,5,"D" ,task(),TaskEnvelope::In::Decay,new EP::FunctionExp(0.0001,10),0.1);
        packageInIs(block,135,5,"S" ,task(),TaskEnvelope::In::Sustain,new EP::FunctionLin(0,10),0);
        packageInIs(block,175,5,"SA",task(),TaskEnvelope::In::SustainAmp,new EP::FunctionLin(0,1),0.2);
        packageInIs(block,215,5,"R" ,task(),TaskEnvelope::In::Release,new EP::FunctionExp(0.0001,10),0.2);

        socketOutIs(block,5,50,"Amp",TaskEnvelope::Out::Amplitude);
        socketOutIs(block,30,50,"Out",TaskEnvelope::Out::Output);
      }
      virtual Task* task() {return taskEnvelope_;}
      virtual std::string serializeName() {return "EntityEnvelope";}
    protected:
      TaskEnvelope* taskEnvelope_;
    };

    class EntityMultiplyAndAdd : public Entity {
    public:
      EntityMultiplyAndAdd(EP::GUI::Area* const parent,EntityData* const data) {
        task_ = new TaskMultiplyAndAdd();

        EP::GUI::Block* block = new EP::GUI::Block("blockMultiplyAndAdd",parent,data->x,data->y,80,100,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,5,20,"Mult Add",EP::Color(0.5,0.5,1));

        socketInIs(block,5,5,"In",TaskMultiplyAndAdd::In::Input,[]() {return 0;});
        socketInIs(block,30,5,"Fact",TaskMultiplyAndAdd::In::Factor,[]() {return 0;});
        socketInIs(block,55,5,"Add",TaskMultiplyAndAdd::In::Shift,[]() {return 0;});

        socketOutIs(block,5,50,"Out",TaskMultiplyAndAdd::Out::Signal);
      }
      virtual Task* task() {return task_;}
      virtual std::string serializeName() {return "EntityMultiplyAndAdd";}
    protected:
      TaskMultiplyAndAdd* task_;
    };

    class EntityQuantizer : public Entity {
    public:
      EntityQuantizer(EP::GUI::Area* const parent,EntityData* const data) {
        task_ = new TaskQuantizer();

        EP::GUI::Block* block = new EP::GUI::Block("blockTaskQuantizer",parent,data->x,data->y,80,100,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,5,20,"Quantizer",EP::Color(0.5,0.5,1));

        socketInIs(block,5,5,"In",TaskQuantizer::In::Input,[]() {return 0;});
        socketInIs(block,30,5,"Fq",TaskQuantizer::In::Chord,[]() {return 0;});

        socketOutIs(block,5,50,"Fq",TaskQuantizer::Out::Freq);
      }
      virtual Task* task() {return task_;}
      virtual std::string serializeName() {return "EntityQuantizer";}
    protected:
      TaskQuantizer* task_;
    };


    class EntityKeyPad : public Entity {
    public:
      EntityKeyPad(EP::GUI::Area* const parent,EntityData* const data) {
        task_ = new TaskKeyPad(10,32);

        EP::GUI::Block* block = new EP::GUI::Block("blockKeyPad",parent,data->x,data->y,350,200,EP::Color(0.2,0.1,0.1));
        blockIs(block);

        socketInIs(block,5,5,"Clk",TaskKeyPad::In::Clock,[]() {return 0;});

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
        socketOutIs(block,30,170,"Row",TaskKeyPad::Out::Row);

      }
      virtual Task* task() {return task_;}
      virtual std::string serializeName() {return "EntityKeyPad";}
    protected:
      std::vector<std::vector<EP::GUI::Button*>> buttons_;
      TaskKeyPad* task_;
    };

    class EntityAudioOut : public Entity {
    public:
      EntityAudioOut(EP::GUI::Area* const parent,EntityData* const data) {
        taskAudioOut_ = new TaskAudioOut();

        EP::GUI::Block* block = new EP::GUI::Block("blockAudioOut",parent,data->x,data->y,100,100,EP::Color(1,0.5,0.5));
        blockIs(block);
        socketMain_  = socketInIs(block,5,5,"Main",TaskAudioOut::In::Signal,[]() {return 0;});
        packageInIs(block,30,5,"A",task(),TaskAudioOut::In::Amplitude,new EP::FunctionLin(0,1),1);
        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,40,20,"Audio Out",EP::Color(1,0.5,0.5));
      }
      virtual Task* task() {return taskAudioOut_;}
      virtual std::string serializeName() {return "EntityAudioOut";}
    protected:
      EP::GUI::Socket* socketMain_=NULL;
      TaskAudioOut* taskAudioOut_;
    };

    class EntityAudioIn : public Entity {
    public:
      EntityAudioIn(EP::GUI::Area* const parent,EntityData* const data) {
        task_ = new TaskAudioIn();

        EP::GUI::Block* block = new EP::GUI::Block("blockAudioIn",parent,data->x,data->y,80,100,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,5,20,"AudioIn",EP::Color(0.5,0.5,1));

        socketOutIs(block,5,50,"In",TaskAudioIn::Out::Signal);
      }
      virtual Task* task() {return task_;}
      virtual std::string serializeName() {return "EntityAudioIn";}
    protected:
      TaskAudioIn* task_;
    };

    class EntitySignalAnalyzer : public Entity {
    public:
      EntitySignalAnalyzer(EP::GUI::Area* const parent,EntityData* const data) {
        task_ = new TaskSignalAnalyzer();

        EP::GUI::Block* block = new EP::GUI::Block("blockSignalAnalyzer",parent,data->x,data->y,80,100,EP::Color(0.5,0.1,0.5));
        blockIs(block);

        EP::GUI::Label* label = new EP::GUI::Label("label",block,5,5,20,"SignalAnalyzer",EP::Color(0.5,0.5,1));

        socketInIs(block,5,5,"In",TaskSignalAnalyzer::In::Signal,[]() {return 0;});

        socketOutIs(block,5,50,"Amp",TaskSignalAnalyzer::Out::Amplitude);
        socketOutIs(block,30,50,"Fq",TaskSignalAnalyzer::Out::Freq);
      }
      virtual Task* task() {return task_;}
      virtual std::string serializeName() {return "EntitySignalAnalyzer";}
    protected:
      TaskSignalAnalyzer* task_;
    };

    static Entity* entityFromData(EP::GUI::BlockHolder* const bh, EntityData* const data) {
      std::cout << "instantiate " << data->type << std::endl;
      if(data->type=="EntityFM") {
        return new EntityFM(bh,data);
      } else if(data->type=="EntityLFO") {
        return new EntityLFO(bh,data);
      } else if(data->type=="EntityAudioOut") {
        return new EntityAudioOut(bh,data);
      } else if(data->type=="EntityTrigger") {
        return new EntityTrigger(bh,data);
      } else if(data->type=="EntityEnvelope") {
        return new EntityEnvelope(bh,data);
      } else if(data->type=="EntityValuePicker") {
        return new EntityValuePicker(bh,data);
      } else if(data->type=="EntityMultiplyAndAdd") {
        return new EntityMultiplyAndAdd(bh,data);
      } else if(data->type=="EntityKeyPad") {
        return new EntityKeyPad(bh,data);
      } else if(data->type=="EntityChordPicker") {
        return new EntityChordPicker(bh,data);
      } else if(data->type=="EntityQuantizer") {
        return new EntityQuantizer(bh,data);
      } else if(data->type=="EntityAudioIn") {
        return new EntityAudioIn(bh,data);
      } else if(data->type=="EntitySignalAnalyzer") {
        return new EntitySignalAnalyzer(bh,data);
      } else {
        std::cout << "do not know " << data->type << std::endl;
        return NULL;
      }
    }


    static EP::GUI::Window* newBlockTemplateWindow(EP::GUI::Area* const parent) {
      EP::GUI::Window* window = new EP::GUI::Window("blockTemplateWindow",parent,10,10,200,400,"Block Templates");

      EP::GUI::Area* content = new EP::GUI::Area("templateHolder",NULL,0,0,100,400);

      float yNext = 5;
      std::function<void(std::string,std::string)> addBlockTemplate = [&content,&yNext](std::string type,std::string name) {
        EP::GUI::BlockTemplate* bt = new EP::GUI::BlockTemplate("blockTemplate"+type,content,5,yNext,90,20,name);
        yNext+=25;
        bt->doInstantiateIs([type](EP::GUI::BlockHolder* bh,const float x,const float y) {
          EntityData data(0,type,x,y);
          data.values["position"] = std::vector<std::string>{std::to_string(x),std::to_string(y)};
          return entityFromData(bh,&data);
        });
      };
      addBlockTemplate("EntityFM","FM Osc.");
      addBlockTemplate("EntityLFO","LFO Osc.");
      addBlockTemplate("EntityAudioOut","Audio Out");
      addBlockTemplate("EntityTrigger","Trigger");
      addBlockTemplate("EntityEnvelope","Envelope");
      addBlockTemplate("EntityValuePicker","V Picker");
      addBlockTemplate("EntityMultiplyAndAdd","Mult Add");
      addBlockTemplate("EntityKeyPad","KeyPad");
      addBlockTemplate("EntityChordPicker","Chord Picker");
      addBlockTemplate("EntityQuantizer","Quantizer");
      addBlockTemplate("EntityAudioIn","Audio In");
      addBlockTemplate("EntitySignalAnalyzer","Signal Analyzer");


      EP::GUI::Area* scroll = new EP::GUI::ScrollArea("scroll",window,content,0,0,100,100);
      content->colorIs(EP::Color(0.1,0,0));
      scroll->fillParentIs(true);
    }

    static std::string entitiesToString() {
      std::string res = "";
      size_t idCounter = 0;
      std::map<Entity*, size_t> entityToId;
      std::map<Task*, Entity*> taskToEntity;
      for (auto &it : blockToEntity) {
        idCounter++;
        Entity* const e = it.second;
        entityToId.insert({e,idCounter});
        taskToEntity.insert({e->task(),e});
      }
      for (auto &it : blockToEntity) {
        Entity* const e = it.second;
        res+=e->serialize(entityToId,taskToEntity);
      }
      std::cout << res << std::endl;
      return res;
    }

    static void stringToEntities(std::string &input,EP::GUI::BlockHolder* const bh) {
      std::cout << "clean" << std::endl;
      for (auto &it : blockToEntity) {
        Entity* const e = it.second;
        e->doDelete();
      }

      std::cout << "construct" << std::endl;
      std::map<size_t, EntityData*> idToData;
      size_t currentId = 0;
      std::vector<std::string> lines = split(input,'\n');
      for (auto &l : lines) {
        std::cout << l << std::endl;
        if (l.size()>0 and l[0]=='@') {
          std::vector<std::string> lparts = split(l,'@');
          currentId = std::atoi(lparts[1].c_str());
          std::cout << "## new id: " << currentId << std::endl;
          std::vector<std::string> llparts = split(lparts[1],' ');
          std::string type = llparts[1];
          std::cout << type << std::endl;
          idToData[currentId] = new EntityData(currentId,type,0,0);
        } else if(l.size()>0) {
          std::vector<std::string> lparts = split(l,'=');
          if (lparts.size()==2) {
            std::vector<std::string> llparts = split(lparts[1],',');
            idToData[currentId]->values[lparts[0]] = llparts;
            if (lparts[0]=="position") {
              idToData[currentId]->x = std::atof(llparts[0].c_str());
              idToData[currentId]->y = std::atof(llparts[1].c_str());
            }
          } else {
            std::cout << "ERROR: bad number of =" << std::endl;
          }
        }
      }
      std::map<size_t, Entity*> idToEntity;
      for (auto &it : idToData) { // generate entities
        EntityData* data = it.second;
        std::cout << data->id << " " << data->type << std::endl;
        idToEntity[data->id] = entityFromData(bh,data);
      }
      for (auto &it : idToData) { // link entities
        EntityData* data = it.second;
        Entity* e = idToEntity[data->id];
        for (size_t i = 0; data->values.find("inTask_"+std::to_string(i))!=data->values.end(); i++) {
          std::vector<std::string> &v = data->values["inTask_"+std::to_string(i)];
          size_t otherId = std::atoi(v[0].c_str());
          size_t otherPort = std::atoi(v[1].c_str());
          if (otherId>0) {
            std::cout << "connect " << data->id << " to " << otherId << "," << otherPort << std::endl;
            Entity* other = idToEntity[otherId];
            EP::GUI::Socket* here = e->inSockets()[i];
            EP::GUI::Socket* there = other->outSockets()[otherPort];
            here->sourceIs(there);
          }
          std::vector<std::string> &val = data->values["inValue_"+std::to_string(i)];
          std::vector<double> values;
          for(auto &vv : val) {values.push_back(std::atof(vv.c_str()));}
          e->valueIs(i,values);
        }
      }

      for (auto &it : idToData) {
        delete it.second;
      }
    }

    static EP::GUI::Window* newDebugWindow(EP::GUI::Area* const parent,EP::GUI::BlockHolder* const bh) {
      EP::GUI::Window* window = new EP::GUI::Window("debugWindow",parent,10,400,200,400,"Debugging");

      EP::GUI::Area* content = new EP::GUI::Area("templateHolder",NULL,0,0,100,400);

      EP::GUI::Button* buttonSerialize = new EP::GUI::Button("buttonSerialize",content,5,5,90,20,"serialize");
      buttonSerialize->onClickIs([]() {
        std::ofstream myfile;
        myfile.open ("save.txt");
        myfile << entitiesToString();
        myfile.close();
      });

      EP::GUI::Button* buttonDeSerialize = new EP::GUI::Button("buttonDeserialize",content,5,30,90,20,"deserialize");
      buttonDeSerialize->onClickIs([bh]() {
        std::ifstream myfile;
        myfile.open("save.txt");
        std::stringstream mystream;
        mystream << myfile.rdbuf();
        myfile.close();
        std::string str = mystream.str();
        std::cout << str << std::endl;
        stringToEntities(str,bh);
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
    EP::GUI::BlockHolder* blockHolder;

    EP::GUI::Window* window2 = new EP::GUI::Window("window2",masterWindow->area(),300,150,600,400,"Designer");
    {
      float x,y,dx,dy;
      window2->childSize(dx,dy);
      window2->childOffset(x,y);
      blockHolder = new EP::GUI::BlockHolder("blockHolder1",NULL,0,0,2000,2000);
      blockHolder->colorIs(EP::Color(0.1,0.1,0.2));
      EP::GUI::Area* scroll1 = new EP::GUI::ScrollArea("scroll1",window2, blockHolder,x,y,dx,dy);
      scroll1->fillParentIs(true);
      //EP::GUI::Area* button1 = new EP::GUI::Button("button1",content1,10,10,100,20,"X");
      //EP::GUI::Area* button2 = new EP::GUI::Button("button2",content1,300,300,100,20,"Y");
    }

    EP::FlowGUI::newBlockTemplateWindow(masterWindow->area());
    EP::FlowGUI::newDebugWindow(masterWindow->area(),blockHolder);

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
  }
  // ------------------------------------ AUDIO
  EP::FlowGUI::AudioOutStream audioOutStream;
  audioOutStream.play();
  // ------------------------------------ Recorder:
  if (!EP::FlowGUI::AudioInStream::isAvailable()) {
      std::cout << "recorder not available" << std::endl;
  }
  EP::FlowGUI::AudioInStream recorder;
  recorder.start();
  // ------------------------------------ MAIN
  while (masterWindow->isAlive()) {
    masterWindow->update();
    masterWindow->draw();
  }
  recorder.stop();
}
