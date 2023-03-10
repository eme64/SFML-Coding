#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <SFML/Graphics.hpp>
#include <list>
#include <queue>
#include <algorithm>

#include "FastNoise/FastNoise.h"
//https://github.com/Auburns/FastNoise/wiki

//----------------------------------- STATIC VARIABLES
float SCREEN_SIZE_X = 1600;
float SCREEN_SIZE_Y = 1000;

float MOUSE_X;
float MOUSE_Y;
float MOUSE_X_MAP;
float MOUSE_Y_MAP;

sf::Font FONT;

// --------- MOUSE
bool MOUSE_LEFT_HIT = false;
bool MOUSE_LEFT_DOWN = false;

bool MOUSE_RIGHT_HIT = false;
bool MOUSE_RIGHT_DOWN = false;

bool ENABLE_CELLS = false;
bool ENABLE_ANTS = true;

float CELLS_MAX = 1.0;

// --------------------------------------------------------- DRAWING
sf::Color HueToRGB(float hue) {
  hue = fmod(fmod(hue,1.0)+1.0,1.0);
  hue*=6.0;
  if (hue<1.0) {
    return sf::Color(255.0,255.0*hue,0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(255.0-255.0*hue,255.0,0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(0,255.0,255.0*hue);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(0,255.0-255.0*hue,255.0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(255.0*hue,0,255.0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(255.0,0,255.0-255.0*hue);
  }
  return sf::Color(255,255,255);
}

void DrawDot(float x, float y, float r, sf::RenderWindow &window, sf::Color color = sf::Color(255,0,0))
{
  sf::CircleShape shape(r);
  shape.setFillColor(color);
  shape.setPosition(x-r, y-r);
  window.draw(shape, sf::BlendAdd);
}

void DrawLine(float x1, float y1, float x2, float y2, sf::RenderWindow &window)
{
  sf::Vertex line[] =
  {
    sf::Vertex(sf::Vector2f(x1,y1), sf::Color(0,0,0)),
    sf::Vertex(sf::Vector2f(x2,y2), sf::Color(255,255,255))
  };
  window.draw(line, 2, sf::Lines, sf::BlendAdd);
}

void DrawRect(float x, float y, float dx, float dy, sf::RenderWindow &window, sf::Color color)
{
  sf::RectangleShape rectangle;
  rectangle.setSize(sf::Vector2f(dx, dy));
  rectangle.setFillColor(color);
  rectangle.setPosition(x, y);
  window.draw(rectangle, sf::BlendAdd);
}

void DrawText(float x, float y, std::string text, float size, sf::RenderWindow &window, sf::Color color = sf::Color(255,255,255))
{
  sf::Text shape(text, FONT);//(text, FONT);
  //shape.setFont(FONT);
  //shape.setString(text); //std::to_string(input[i])
  shape.setCharacterSize(size);
  shape.setColor(color);
  shape.setPosition(x,y);
  //shape.setStyle(sf::Text::Bold | sf::Text::Underlined);
  window.draw(shape, sf::BlendAdd);
}

struct Cell {
  float feromone = 0;
};

struct Ant {
  float x, y, w, c = 0;
};

struct Arena {
  int size_x, size_y, scale;
  std::vector<std::vector<Cell>> cells;
  std::vector<Ant> ants;

  Arena(int x, int y, int scale, int num) : size_x(x), size_y(y), scale(scale) {
    cells.resize(size_y);
    for (int i = 0; i < size_y; i++) {
      cells[i].resize(size_x);
    }
    ants.resize(num);
    for (Ant &ant : ants) {
      ant.x = (((float) rand() / (RAND_MAX)) * size_x);
      ant.y = (((float) rand() / (RAND_MAX)) * size_y);
      ant.w = (((float) rand() / (RAND_MAX)) * M_PI * 2);
    }
  }

  void update() {
    for (Ant &ant : ants) {
      int x = ant.x;
      int y = ant.y;
      Cell &cell = cells[y][x];
      cell.feromone += 0.01;
      float w = ant.w;
      float ax = ant.x + cos(w) * 0.5;
      float ay = ant.y + sin(w) * 0.5;
      float axl = ant.x + cos(w-M_PI*0.2) * 1.5;
      float ayl = ant.y + sin(w-M_PI*0.2) * 1.5;
      float axr = ant.x + cos(w+M_PI*0.2) * 1.5;
      float ayr = ant.y + sin(w+M_PI*0.2) * 1.5;
      axl = fmod(fmod(axl, size_x) + size_x, size_x);
      ayl = fmod(fmod(ayl, size_y) + size_y, size_y);
      axr = fmod(fmod(axr, size_x) + size_x, size_x);
      ayr = fmod(fmod(ayr, size_y) + size_y, size_y);
      float fl = cells[ayl][axl].feromone;
      float fr = cells[ayr][axr].feromone;
      float wr = (((float) rand() / (RAND_MAX))*0.04 - 0.02);
      ant.w += (fl > fr) ? -0.1 : 0.1;
      ant.c = ant.w;
      ant.x = fmod(fmod(ax, size_x) + size_x, size_x);
      ant.y = fmod(fmod(ay, size_y) + size_y, size_y);
    }
    for (int y = 0; y < size_y; y++) {
      for (int x = 0; x < size_x; x++) {
        cells[y][x].feromone *= 0.99;
      }
    }
  }

  void draw(sf::RenderWindow &window) {
    if (ENABLE_CELLS) {
      float factor = 0.6 / CELLS_MAX;
      float new_max = 0.1;
      for (int y = 0; y < size_y; y++) {
        for (int x = 0; x < size_x; x++) {
          float f = cells[y][x].feromone;
          new_max = std::max(new_max, f);
          DrawRect(x*scale, y*scale, scale, scale, window, HueToRGB(f*factor));
        }
      }
    }
    if (ENABLE_ANTS) {
      for (Ant &ant : ants) {
        DrawDot(ant.x*scale, ant.y*scale, 3, window, HueToRGB(ant.c / (M_PI*2)));
      }
    }
  }
};

//-------------------------------------------------------- Main
int main()
{
    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    //sf::Style style = sf::Style::Fullscreen;//Default
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "feromones", sf::Style::Fullscreen, settings);
    sf::Vector2u size = window.getSize();
    SCREEN_SIZE_X = size.x;
    SCREEN_SIZE_Y = size.y;
    window.setVerticalSyncEnabled(true);
    window.setMouseCursorVisible(false);

    // ------------------------------------ FONT
    if (!FONT.loadFromFile("arial.ttf"))
    {
        std::cout << "font could not be loaded!" << std::endl;
        return 0;
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

    int scale = 5;
    Arena arena(SCREEN_SIZE_X/scale, SCREEN_SIZE_Y/scale, scale, 20000);

    // --------------------------- Render Loop
    while (window.isOpen())
    {
        MOUSE_LEFT_HIT = false;
        MOUSE_RIGHT_HIT = false;

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
                  if(event.mouseButton.button == sf::Mouse::Left)
                  {
                    MOUSE_LEFT_HIT = true;
                    MOUSE_LEFT_DOWN = true;
                  }else if(event.mouseButton.button == sf::Mouse::Right)
                  {
                    MOUSE_RIGHT_HIT = true;
                    MOUSE_RIGHT_DOWN = true;
                  }
                  break;
                case sf::Event::MouseButtonReleased:
                  if(event.mouseButton.button == sf::Mouse::Left)
                  {
                    MOUSE_LEFT_DOWN = false;
                  }else if(event.mouseButton.button == sf::Mouse::Right)
                  {
                    MOUSE_RIGHT_DOWN = false;
                  }
                  break;
                // we don't process other types of events
                default:
                  break;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)){
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
            ENABLE_CELLS = false;
          } else {
            ENABLE_CELLS = true;
          }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)){
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
            ENABLE_ANTS = false;
          } else {
            ENABLE_ANTS = true;
          }
        }

        sf::Vector2i mousepos = sf::Mouse::getPosition(window);
        MOUSE_X = mousepos.x;
        MOUSE_Y = mousepos.y;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }

	window.clear();

        // Draw Mouse
        DrawRect(MOUSE_X, MOUSE_Y, 3, 3, window, sf::Color(255,255,255));

        for (int i=0; i<10; i++) {
          arena.update();
        }
        arena.draw(window);

        FPS_tick();
        DrawText(5,5, "FPS: " + std::to_string(framesPerSec), 10, window, sf::Color(255,255,255));
        DrawText(5,20, "avg: " + std::to_string(framesPerSec_avg), 10, window, sf::Color(255,255,255));
        DrawText(5,35, "ticker: " + std::to_string(ticker), 10, window, sf::Color(255,255,255));

	window.display();
    }
    return 0;
}
