#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <list>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <valarray>
#include <complex>
#include <mutex>

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

class MyRecorder : public sf::SoundRecorder {
    virtual bool onStart() {
        std::cout << "Start" << std::endl;
        setProcessingInterval(sf::milliseconds(0.1));
        return true;
    }

    virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount) {
        const size_t newSize = std::pow(2.0, (size_t) std::log2(sampleCount));
        std::cout << "process " << samples[0] << " " << sampleCount << " " << newSize << std::endl;
        std::lock_guard<std::mutex> lock(mutex_);
        array_.resize(newSize);
        for (size_t i = 0; i < newSize; i++) {
          array_[i] = samples[i];
        }
        cooleyTukeyFFT(array_);
        std::cout << array_[0] << std::endl;
        return true;
    }

    virtual void onStop() {
        std::cout << "Stop" << std::endl;
    }
public:
		float getValue() {
			// return value between 0..1, if out of bounds: consider off
			std::lock_guard<std::mutex> lock(mutex_);
			float res = transformInv((double)getMaxIndex()*2 / array_.size());
			return (res-0.05)*10.0;
		}
    void draw(sf::RenderWindow &window)
    {
        std::cout << "draw" << std::endl;
        if (array_.size()==0) {
          return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        const size_t N = 1000, dx=1, dy=1;
        const double A = 0.0001;
        for (size_t i = 0; i < N; i++) {
          const size_t index = array_.size()*0.5*transform((double)i/(double)N);
          const double value = std::abs(array_[index]);
          sf::RectangleShape rectangle;
          rectangle.setSize(sf::Vector2f(dx, dy));
          rectangle.setPosition(i*dx, value*A*dy+100);
          rectangle.setFillColor(sf::Color(200,0,0));
          window.draw(rectangle);
        }
        {
          int x = N*transformInv((double)getMaxIndex()*2 / array_.size());
          sf::RectangleShape rectangle;
          rectangle.setSize(sf::Vector2f(5,5));
          rectangle.setPosition(x*dx, 100);
          rectangle.setFillColor(sf::Color(0,200,0));
          window.draw(rectangle);
        }
    }
    double transform(const double in) const {
      //return in;
      return std::pow(2.0,in)-1.0;
    }
    double transformInv(const double out) const {
      //return out;
      return std::log2(out+1.0);
    }
    size_t getMaxIndex() const {
      size_t m = 0;
      double maxVal = 0;
      for (size_t i = 0; i < array_.size()/2; i++) {
        const double loc = std::abs(array_[i]);
        if (maxVal < loc) {
          maxVal = loc;
          m = i;
        }
      }
      std::cout << m << std::endl;
      return m;
    }
private:
  std::mutex mutex_;
  SampleArray array_;
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

    if (!MyRecorder::isAvailable()) {
        std::cout << "not available" << std::endl;
    }
    MyRecorder recorder;
    recorder.start();


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

        if (MouseDown_left) {
          //board.set(true, sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
        }
        if (MouseDown_right) {
          //board.set(false, sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
        }



        window.clear();

        recorder.draw(window);
				float res = recorder.getValue();
				if (res>=0 and res<=1) {
					std::cout << res << std::endl;
				}

        // -------------- draw text:
        sf::Text text;
        text.setFont(font);
        text.setString(std::to_string(res));
        text.setCharacterSize(20);
        window.draw(text);

        window.display();
    }
    recorder.stop();

    return 0;
}
