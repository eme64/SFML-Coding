#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <SFML/Graphics.hpp>
// #include <SFML/Audio.hpp>
// #include <list>
// #include <queue>
// #include <algorithm>
// #include <thread>
// #include <valarray>
// #include <complex>
// #include <mutex>
// #include <forward_list>
// #include <functional>

#include "ep_gui.hpp"

//-------------------------------------------------------- Main
int main()
{
    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    //sf::Style style = sf::Style::Fullscreen;//Default
    sf::RenderWindow window(sf::VideoMode(1000, 600), "first attempts", sf::Style::Default, settings);

    window.setVerticalSyncEnabled(true);
    //window.setMouseCursorVisible(false);

    EP::GUI::Area* windowArea;
    {
      sf::Vector2u size = window.getSize();
      windowArea = (new EP::GUI::Area(NULL,0,0,size.x,size.y))->fillParentIs(true);
    }


    // ------------------------------------ FPS
    clock_t last_stamp = clock();
    clock_t deltaTime = 1000;
    float framesPerSec = 10;
    float framesPerSec_avg = 10;
    float averageDelta = 1000;
    size_t ticker = 0;

    auto FPS_tick = [&last_stamp, &deltaTime, &framesPerSec, &framesPerSec_avg, &averageDelta, &ticker]()
    {
      clock_t newTime = clock();
      deltaTime = newTime - last_stamp;

      averageDelta = 0.01 * deltaTime + 0.99 * averageDelta;

      framesPerSec = CLOCKS_PER_SEC / (float)deltaTime;
      framesPerSec_avg = CLOCKS_PER_SEC / (float)averageDelta;

      last_stamp = newTime;
      ticker++;
    };

    // --------------------------- Render Loop
    while (window.isOpen())
    {
        // MOUSE_LEFT_HIT = false;
        // MOUSE_RIGHT_HIT = false;

        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
                // window closed
                case sf::Event::Closed:
                  window.close();
                  break;
                case sf::Event::MouseWheelScrolled:
                  if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
                  {
                    // int delta = event.mouseWheelScroll.delta;
                    // if(delta > 0)
                    // {
                    //   screen_zoom*= 1.1;
                    // }else{
                    //   screen_zoom*= 0.9;
                    // }
                  }
                  break;
                case sf::Event::MouseButtonPressed:
                  // if(event.mouseButton.button == sf::Mouse::Left)
                  // {
                  //   MOUSE_LEFT_HIT = true;
                  //   MOUSE_LEFT_DOWN = true;
                  // }else if(event.mouseButton.button == sf::Mouse::Right)
                  // {
                  //   MOUSE_RIGHT_HIT = true;
                  //   MOUSE_RIGHT_DOWN = true;
                  // }
                  break;
                case sf::Event::MouseButtonReleased:
                  // if(event.mouseButton.button == sf::Mouse::Left)
                  // {
                  //   MOUSE_LEFT_DOWN = false;
                  // }else if(event.mouseButton.button == sf::Mouse::Right)
                  // {
                  //   MOUSE_RIGHT_DOWN = false;
                  // }
                  break;
                case sf::Event::Resized:
                  {
                    // update the view to the new size of the window
                    sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                    window.setView(sf::View(visibleArea));
                    sf::Vector2u size = window.getSize();
                    windowArea->sizeIs(size.x,size.y);
                  }
                  break;
                // we don't process other types of events
                default:
                  break;
            }
        }

        // sf::Vector2i mousepos = sf::Mouse::getPosition(window);
        // MOUSE_X = mousepos.x;
        // MOUSE_Y = mousepos.y;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }


        window.clear();

        windowArea->draw(0,0,1.0,window);

        // DrawDot(MOUSE_X, MOUSE_Y, window, sf::Color(255*MOUSE_RIGHT_DOWN,255*MOUSE_LEFT_DOWN,255));

        FPS_tick();

        EP::DrawText(5,5, "FPS: " + std::to_string(framesPerSec), 10, window, EP::Color(1.0,1.0,1.0));
        EP::DrawText(5,20, "avg: " + std::to_string(framesPerSec_avg), 10, window, EP::Color(1.0,1.0,1.0));
        EP::DrawText(5,35, "ticker: " + std::to_string(ticker), 10, window, EP::Color(1.0,1.0,1.0));


        window.display();
    }

    return 0;
}
