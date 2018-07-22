#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <list>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>

class Instrument
{
public:
  Instrument(double frequency)
  :frequency(frequency), isOn(false)
  {
    // empty
  }

  double sample(double t)
  {
    if (isOn) {
      //return std::sin(t * frequency * 2.0 * M_PI + 1*std::sin(t * 5.0 * 2.0 * M_PI));
      return 2.0*std::fmod(t * frequency + 0.1*std::sin(t * 5.0 * 2.0 * M_PI), 1.0)-1;
    }
    return 0;
  }

  void press()
  {
    isOn = true;
  }

  void release()
  {
    isOn = false;
  }

private:
  bool isOn;
  double frequency;
};

// custom audio stream that plays a loaded buffer
class MyStream : public sf::SoundStream
{
public:
  MyStream()
  :mySampleRate(44100), mySampleSize(2000), myAmplitude(30000)
  {
    ticks = 0;
    initialize(1, mySampleRate);
  }

  void addInstrument(Instrument *instrument)
  {
    instruments.push_back(instrument);
  }

private:
  const unsigned mySampleRate;
  const unsigned mySampleSize;
  const unsigned myAmplitude;

  std::vector<sf::Int16> samples;

  size_t ticks;

  std::vector<Instrument*> instruments;

  virtual bool onGetData(Chunk& data)
  {
    samples.clear();

    for (size_t i = 0; i < mySampleSize; i++) {
      ticks++;
      double t = (double)(ticks) / (double)mySampleRate;

      double res = 0.0;

      for (Instrument *i : instruments) {
        res += i->sample(t)*0.5;
      }

      samples.push_back(myAmplitude * res);
    }

    data.samples = &samples[0];
    data.sampleCount = mySampleSize;
    return true;
  }

  virtual void onSeek(sf::Time timeOffset)
  {
    // not supported.
  }
};

struct Node
{
  float x; float y; float r; float a;
  sf::Color color;

  Node *parent;

  Node(float xpos, float ypos, float radius, float alpha, sf::Color drawcolor, Node *myparent)
  {
    x = xpos;
    y = ypos;
    color = drawcolor;
    r = radius;
    a = alpha;
    parent = myparent;
  }

  void update()
  {
    if(parent != NULL)
    {
      float pfac = 0.3;
      float cfac = 0.5;

      a = (1.0-pfac)*a + pfac*parent->a;
      x = (1.0-pfac)*x + pfac*parent->x;
      y = (1.0-pfac)*y + pfac*parent->y;


      color.r = (1.0-cfac)*color.r + cfac*parent->color.r;
      color.g = (1.0-cfac)*color.g + cfac*parent->color.g;
      color.b = (1.0-cfac)*color.b + cfac*parent->color.b;

    }else{
      // Independent movement
      float da = (((float) rand() / (RAND_MAX))-0.5)*0.9;
      float speed = 4.0;

      a+=da;
      x+=std::cos(a)*speed;
      y+=std::sin(a)*speed;

      if(x<0){x=0; a=0;}
      if(y<0){y=0; a=M_PI*0.5;}

      if(x>800){x=800; a=M_PI;}
      if(y>600){y=600; a=M_PI*1.5;}

      if(((float) rand() / (RAND_MAX)) < 0.1){
        color.r = 50 + 206*((float) rand() / (RAND_MAX));
        color.g = 50 + 206*((float) rand() / (RAND_MAX));
        color.b = 50 + 206*((float) rand() / (RAND_MAX));
      }
    }
  }

  void draw(sf::RenderWindow &window)
  {
    sf::CircleShape shape(r*2);
    shape.setFillColor(color);
    shape.setPosition(x-r, y-r);
    window.draw(shape, sf::BlendAdd);
  }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "first attempts");
    window.setVerticalSyncEnabled(true);


    std::list<Node*> node_list;

    Node *last_node = NULL;
    for(int i=0; i<1000; i++)
    {
      float xx = 800*((float) rand() / (RAND_MAX));
      float yy = 600*((float) rand() / (RAND_MAX));

      int r = 100 + 156*((float) rand() / (RAND_MAX));
      int g = 100 + 156*((float) rand() / (RAND_MAX));
      int b = 100 + 156*((float) rand() / (RAND_MAX));

      float radius;
      if(last_node !=NULL){
        radius = 2;
        xx = last_node->x;
        yy = last_node->y;
      }else{
        radius = 3;
      }

      Node* n = new Node(xx,yy, radius, 0, sf::Color(r,g,b), last_node);
      node_list.push_back(n);

      if( ((float) rand() / (RAND_MAX)) < 0.05)
      {
        last_node = NULL;
      }else{
        last_node = n;
      }
    }


    // ############## SOUND STUFF:
    /*
    const unsigned SAMPLE_RATE = 44100;
    const unsigned AMPLITUDE = 30000;
    std::vector<sf::Int16> samples;

    double frequency = 440.0;

    for (size_t i = 0; i < SAMPLE_RATE; i++) {
      double t = (double)i / (double)SAMPLE_RATE;
      samples.push_back(AMPLITUDE * std::sin(t * frequency * 2.0 * M_PI));
    }

    sf::SoundBuffer buffer;
    if (!buffer.loadFromSamples(&samples[0], SAMPLE_RATE, 1, SAMPLE_RATE)) {
  		std::cerr << "Loading failed!" << std::endl;
  		return 1;
  	}
    sf::Sound sound;
  	sound.setBuffer(buffer);
  	sound.setLoop(true);
  	sound.play();
    */

    MyStream stream;
    stream.play();

    std::vector<Instrument*> instruments(10);

    for (size_t i = 0; i < instruments.size(); i++) {
      instruments[i] = new Instrument(440*std::pow(2.0,(double)i/12.0)); // std::pow(2.0/12.0,i)
      stream.addInstrument(instruments[i]);
    }

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
                // window closed
                case sf::Event::Closed:
                    window.close();
                    break;
                // we don't process other types of events
                default:
                    break;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)){
          instruments[0]->press();
        }else{
          instruments[0]->release();
        }

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)){
          instruments[1]->press();
        }else{
          instruments[1]->release();
        }

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)){
          instruments[2]->press();
        }else{
          instruments[2]->release();
        }

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)){
          instruments[3]->press();
        }else{
          instruments[3]->release();
        }

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)){
          instruments[4]->press();
        }else{
          instruments[4]->release();
        }

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)){
          instruments[5]->press();
        }else{
          instruments[5]->release();
        }

        /*
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
          pos_x-= 1.0;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
          pos_x+= 1.0;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
          pos_y-= 1.0;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
          pos_y+= 1.0;
        }
        */
        window.clear();

        for (std::list<Node*>::iterator it = node_list.begin(); it != node_list.end(); it++)
        {
            (**it).update();
        }

        for (std::list<Node*>::iterator it = node_list.begin(); it != node_list.end(); it++)
        {
            (**it).draw(window);
        }

        window.display();
    }

    return 0;
}
