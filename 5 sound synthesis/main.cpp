#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <list>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <forward_list>
#include <mutex>

class Instrument
{
public:
  bool isAlive = true;

  Instrument(double frequency, std::function<double(double, double)> oscillator)
  :frequency(frequency), oscillator(oscillator)
  {
    // empty
  }

  static double oscillator_sin(double t, double freq)
  {
    return std::sin(t * freq*2.0*M_PI + 0.1*std::sin(t * 5.0 * 2.0 * M_PI));
  }

  static double oscillator_mod(double t, double freq)
  {
    return 2.0*std::fmod(t * freq + 0.1*std::sin(t * 5.0 * 2.0 * M_PI), 1.0)-1.0;
  }

  static double oscillator_tria(double t, double freq)
  {
    double saw = std::fmod(t * freq + 0.1*std::sin(t * 5.0 * 2.0 * M_PI), 1.0);
    return std::abs(saw-0.5)*2.0;
  }

  double sample(double t)
  {
    if (attacked == false) { // first time playing:
      attacked = true;
      when_attacked = t;
    }

    double res = oscillator(t, frequency);

    if(attacked)
    {
      res *= attack_envelope(t - when_attacked);
    }

    if(released)
    {
      res *= release_envelope(t - when_released);
    }

    t_last = t;
    return res;
  }

  void release()
  {
    released = true;
    when_released = t_last;
  }

private:
  std::function<double(double, double)> oscillator; // t, freq

  double t_last;
  bool attacked = false;
  bool released = false;
  double when_attacked;
  double when_released;
  double frequency;

  double attack_envelope(double t)// t since attack:
  {
    const double attack_time = 0.005;
    const double decay_time = 0.01;
    const double sustain_level = 0.3;

    if (t < attack_time) {
      return t/attack_time;
    }
    t-=attack_time;
    if (t < decay_time) {
      return (1.0-t/decay_time) *(1.0-sustain_level) + sustain_level;
    }

    return sustain_level;
  }

  double release_envelope(double t) // t since release
  {
    double res = std::max(0.0, 1.0-t/0.2);
    if (res < 0.001) {
      isAlive = false;
    }
    return res;
  }
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

  void playInstrument(Instrument *instrument)
  {
    std::lock_guard<std::mutex> lock(instruments_mutex);
    instruments.push_front(instrument);
  }

  void stopInstrument(Instrument *instrument)
  {
    instrument->release();
  }

  void setTicker(std::function<void()> ticker)
  {
    this->ticker = ticker;
  }

private:
  std::function<void()> ticker = [](){};

  const unsigned mySampleRate;
  const unsigned mySampleSize;
  const unsigned myAmplitude;

  std::vector<sf::Int16> samples;

  size_t ticks;

  std::mutex instruments_mutex;
  std::forward_list<Instrument*> instruments;

  virtual bool onGetData(Chunk& data)
  {
    samples.clear();

    for (size_t i = 0; i < mySampleSize; i++) {
      ticks++;
      ticker();
      double t = (double)(ticks) / (double)mySampleRate;

      double res = 0.0;

      std::vector<Instrument*> delete_vec;

      std::lock_guard<std::mutex> lock(instruments_mutex);
      for (Instrument *i : instruments) {
        res += i->sample(t)*0.05;

        if(!i->isAlive)
        {
          delete_vec.push_back(i);
        }
      }

      for (Instrument *i : delete_vec) {
        instruments.remove(i);
        delete(i);
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


class MusicBoard
{
public:
  MusicBoard(MyStream &stream, double size_x, double size_y, int n_x, int n_y)
  :size_x(size_x), size_y(size_y), n_x(n_x), n_y(n_y),
  dx(size_x/n_x), dy(size_y/n_y),
  stream(stream),
  oscillator(Instrument::oscillator_sin)
  {
    grid.resize(n_x);
    instruments.resize(n_x);
    for (size_t i = 0; i < n_x; i++) {
      grid[i].resize(n_y);
      instruments[i].resize(n_y);
    }

    resetFrequencies();
  }

  void resize(int n_x, int n_y)
  {
    clear();

    this->n_x = n_x;
    this->n_y = n_y;


    dx = size_x/n_x;
    dy = size_y/n_y;

    grid.resize(n_x);
    instruments.resize(n_x);
    for (size_t i = 0; i < n_x; i++) {
      grid[i].resize(n_y);
      instruments[i].resize(n_y);
    }

    clear();

    resetFrequencies();

    active_column = 0;
    count_ticks = 0;
  }

  void setFrequencies(std::vector<double> freq)
  {
    for (size_t j = 0; j < n_y; j++) {
      int octave = j/freq.size() + 1;
      size_t offset = j % freq.size();
      frequencies[n_y-j-1] = freq[offset]*std::pow(2.0,octave);
    }
  }

  void resetFrequencies()
  {
    frequencies.resize(n_y);
    for (size_t j = 0; j < n_y; j++) {
      frequencies[j] = 110*std::pow(2.0,(double)(n_y-j)/12.0);
    }
  }

  void setOscillator(std::function<double(double, double)> o)
  {
    oscillator = o;
  }

  void draw(sf::RenderWindow &window)
  {
    for (size_t i = 0; i < n_x; i++) {
      for (size_t j = 0; j < n_y; j++) {
        sf::RectangleShape rectangle;
        rectangle.setSize(sf::Vector2f(dx-1, dy-1));
        rectangle.setPosition(i*dx, j*dy);
        if(grid[i][j])
        {
          if (i == active_column) {
            rectangle.setFillColor(sf::Color(200,0,0));
          }else{
            rectangle.setFillColor(sf::Color(100,0,0));
          }
        }else{
          if (i == active_column) {
            rectangle.setFillColor(sf::Color(0,150,200));
          }else{
            rectangle.setFillColor(sf::Color(0,100,150));
          }
        }
        window.draw(rectangle);
      }
    }
  }

  void set(bool value, double x, double y)
  {
    int i = std::min(n_x-1.0, std::max(0.0, x / dx));
    int j = std::min(n_y-1.0, std::max(0.0, y / dy));

    grid[i][j] = value;
  }

  void tick()
  {
    count_ticks++;
    if (count_ticks>=ticks_per_column) {
      count_ticks = 0;

      size_t old_col = active_column;
      active_column++;
      if (active_column>=n_x) {
        active_column = 0;
      }

      for (size_t j = 0; j < n_y; j++) {
        if (instruments[old_col][j] == NULL) {
          // start new if needed:
          if (grid[active_column][j]) {
            instruments[active_column][j] = new Instrument(frequencies[j], oscillator);
            stream.playInstrument(instruments[active_column][j]);
          }
        }else{
          if (grid[active_column][j]) {
            // move instrument:
            instruments[active_column][j] = instruments[old_col][j];
            instruments[old_col][j] = NULL;
          }else{
            // stop old.
            stream.stopInstrument(instruments[old_col][j]);
            instruments[old_col][j] = NULL;
          }
        }
      }
    }
  }

  void clear()
  {
    for (size_t i = 0; i < n_x; i++) {
      for (size_t j = 0; j < n_y; j++) {
        grid[i][j] = false;
        if (instruments[i][j] != NULL) {
          stream.stopInstrument(instruments[i][j]);
          instruments[i][j] = NULL;
        }
      }
    }
  }
private:
  MyStream &stream;

  double size_x; // screen
  double size_y;

  double dx;
  double dy;

  int n_x; // grid units
  int n_y;

  std::vector<std::vector<bool>> grid;
  std::vector<std::vector<Instrument*>> instruments;
  size_t active_column = 0;
  size_t ticks_per_column = 5000;
  size_t count_ticks = 0;

  std::vector<double> frequencies;

  std::function<double(double, double)> oscillator;
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(1200, 900), "Music Board");
    window.setVerticalSyncEnabled(true);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf"))
    {
        // error...
    }

    MyStream stream;
    stream.play();

    MusicBoard board(stream, 1200, 900, 32, 32);
    stream.setTicker([&board](){board.tick();});

    bool MouseDown_left = false;
    bool MouseDown_right = false;

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
                case sf::Event::MouseButtonPressed:
                  if (event.mouseButton.button == sf::Mouse::Left)
                  {
                    MouseDown_left = true;
                  }else if (event.mouseButton.button == sf::Mouse::Right) {
                    MouseDown_right = true;
                  }
                  break;
                case sf::Event::MouseButtonReleased:
                  if (event.mouseButton.button == sf::Mouse::Left)
                  {
                    MouseDown_left = false;
                  }else if (event.mouseButton.button == sf::Mouse::Right) {
                    MouseDown_right = false;
                  }
                  break;
                // we don't process other types of events
                default:
                  break;
            }
        }


        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)){
          board.clear();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)){
          board.resetFrequencies();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)){
          board.setFrequencies(std::vector<double>{30,36,40,45,54});
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)){
          board.setFrequencies(std::vector<double>{24,27,30,36,40});
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)){
          board.setFrequencies(std::vector<double>{24,27,32,36,42});
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)){
          board.setFrequencies(std::vector<double>{15,18,20,24,27});
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)){
          board.setFrequencies(std::vector<double>{24,27,32,36,40});
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num7)){
          board.setFrequencies(std::vector<double>{32.70,36.71,41.20,43.65,49.00,55.00,61.74});
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)){
          board.setFrequencies(std::vector<double>{32.70,36.71,38.89,43.65,49.00,51.91,58.27});
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)){
          board.setOscillator(Instrument::oscillator_sin);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)){
          board.setOscillator(Instrument::oscillator_mod);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)){
          board.setOscillator(Instrument::oscillator_tria);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)){
          board.resize(16, 32);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)){
          board.resize(32, 32);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)){
          board.resize(64, 32);
        }

        if (MouseDown_left) {
          board.set(true, sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
        }
        if (MouseDown_right) {
          board.set(false, sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
        }

        window.clear();

        board.draw(window);

        // -------------- draw text:
        sf::Text text;
        text.setFont(font);
        text.setString("[1-8] Frequency Set (Harmony)\n\n[QWE] Wave forms\n\n[Space] Clear\n\n[ASD] Resolution");
        text.setCharacterSize(20);
        window.draw(text);

        window.display();
    }

    return 0;
}
