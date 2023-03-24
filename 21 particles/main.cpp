#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <SFML/Graphics.hpp>
#include <list>
#include <queue>
#include <algorithm>
#include <vector>
#include <set>

//----------------------------------- STATIC VARIABLES
float SCREEN_SIZE_X = 1500;
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

bool DRAW_CELLS = false;

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
  return sf::Color(255.0,0,255.0-255.0*hue);
}

sf::Color heat(float h) {
  if (h >= 0) {
    return sf::Color(255.0*h,0,0);
  } else {
    return sf::Color(0,-255.0*h,-255.0*h);
  }
}

sf::Color heat2(float h) {
  if (h >= 0) {
    if (h <= 0.1) {
      h *= 10.0;
      return sf::Color(255.0*h,0,255.0*h);
    } else if (h <= 0.3) {
      h = (h-0.1) * 5.0;
      return sf::Color(255.0,0,255.0 - 255.0*h);
    } else {
      h = (h-0.3) * (1.0 / 0.7);
      return sf::Color(255.0,255.0*h,0);
    }
  } else {
    h = -h;
    if (h <= 0.1) {
      h *= 10.0;
      return sf::Color(0,0,255.0*h);
    } else if (h <= 0.3) {
      h = (h-0.1) * 5.0;
      return sf::Color(0,255.0*h,255.0);
    } else {
      h = (h-0.3) * (1.0 / 0.7);
      return sf::Color(0,255.0,255.0-255.0*h);
    }
  }
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

struct Particle {
  float x, y;
  float dx, dy;
  int t;
  Particle(float x_, float y_, float t_) : x(x_), y(y_), t(t_) {}
};

struct Arena {
  int size_x, size_y, scale;
  std::vector<std::vector<std::set<int>>> cells;
  std::vector<Particle> particles;

  Arena(int x, int y, int scale, int nump, int numt) : size_x(x), size_y(y), scale(scale) {
    // cells
    cells.resize(size_y);
    for (int i = 0; i < size_y; i++) {
      cells[i].resize(size_x);
    }
    // particles
    particles.reserve(nump);
    for (int i = 0; i < nump; i++) {
      particles.push_back(Particle(
        (((float) rand() / (RAND_MAX)) * size_x),
        (((float) rand() / (RAND_MAX)) * size_y),
        rand() % numt
      ));
    }
  }

  void remap() {
    for (int y = 0; y < size_y; y++) {
      for (int x = 0; x < size_x; x++) {
        cells[y][x].clear();
      }
    }
    for (int i = 0; i < particles.size(); i++) {
      Particle &p = particles[i];
      int x = ((((int) p.x) % size_x) + size_x) % size_x;
      int y = ((((int) p.y) % size_y) + size_y) % size_y;
      cells[y][x].insert(i);
    }
  }

  // void reset() {
  //   for (int y = 0; y < size_y; y++) {
  //     for (int x = 0; x < size_x; x++) {
  //       cells[y][x].feromone = 0;
  //     }
  //   }
  //   for (Ant &ant : ants) {
  //     ant.x = (((float) rand() / (RAND_MAX)) * size_x);
  //     ant.y = (((float) rand() / (RAND_MAX)) * size_y);
  //     ant.w = (((float) rand() / (RAND_MAX)) * M_PI * 2);
  //   }
  // }

  void update() {
    remap();
    for (int ip1 = 0; ip1 < particles.size(); ip1++) {
      Particle &p = particles[ip1];
      int x = ((((int) p.x) % size_x) + size_x) % size_x;
      int y = ((((int) p.y) % size_y) + size_y) % size_y;
      float dx = (((float) rand() / (RAND_MAX))*0.04 - 0.02);
      float dy = (((float) rand() / (RAND_MAX))*0.04 - 0.02);
      for (int xx = -1; xx <= 1; xx++) {
        for (int yy = -1; yy <= 1; yy++) {
          int xxx = (xx + x + size_x) % size_x;
          int yyy = (yy + y + size_y) % size_y;
          for (int ip2 : cells[yyy][xxx]) {
            if (ip1 != ip2) {
              Particle &p2 = particles[ip2];
              float ddx = p.x - p2.x;
              float ddy = p.y - p2.y;
              if (ddx > size_x  * 0.5) { ddx = size_x - ddx; }
              if (ddx < size_x  *-0.5) { ddx = size_x + ddx; }
              if (ddy > size_y  * 0.5) { ddy = size_y - ddy; }
              if (ddy < size_y  *-0.5) { ddy = size_y + ddy; }
              float d = std::sqrt(ddx*ddx + ddy*ddy);
              if (d > 0 && d < 0.5) {
                float f = (0.5-d) / d;
                dx += ddx*f;
                dy += ddy*f;
              }
            }
          }
        }
      }
      p.dx = dx;
      p.dy = dy;
    }
    for (int i = 0; i < particles.size(); i++) {
      Particle &p = particles[i];
      p.x = fmod(fmod(p.x + p.dx, size_x) + size_x, size_x);
      p.y = fmod(fmod(p.y + p.dy, size_y) + size_y, size_y);
    }
 
    // for (Ant &ant : ants) {
    //   int x = ((((int) ant.x) % size_x) + size_x) % size_x;
    //   int y = ((((int) ant.y) % size_y) + size_y) % size_y;
    //   Cell &cell = cells[y][x];
    //   cell.feromone += ant.f;
    //   float w = ant.w;
    //   float ax = ant.x + cos(w) * ANTS_SPEED;
    //   float ay = ant.y + sin(w) * ANTS_SPEED;
    //   int axl = ant.x + cos(w-M_PI*0.2) * 1.5;
    //   int ayl = ant.y + sin(w-M_PI*0.2) * 1.5;
    //   int axr = ant.x + cos(w+M_PI*0.2) * 1.5;
    //   int ayr = ant.y + sin(w+M_PI*0.2) * 1.5;
    //   axl = ((axl % size_x) + size_x) % size_x;
    //   ayl = ((ayl % size_y) + size_y) % size_y;
    //   axr = ((axr % size_x) + size_x) % size_x;
    //   ayr = ((ayr % size_y) + size_y) % size_y;
    //   float fl = cells[ayl][axl].feromone;
    //   float fr = cells[ayr][axr].feromone;
    //   float wr = (((float) rand() / (RAND_MAX))*0.04 - 0.02);
    //   ant.w += ((fl > fr) ? -ANTS_AGILITY : ANTS_AGILITY) * ant.f;
    //   ant.x = fmod(fmod(ax, size_x) + size_x, size_x);
    //   ant.y = fmod(fmod(ay, size_y) + size_y, size_y);
    // }
  }

  void draw(sf::RenderWindow &window) {
    if (DRAW_CELLS) {
      for (int y = 0; y < size_y; y++) {
        for (int x = 0; x < size_x; x++) {
          if ((x + y) % 2 == 0) {
            DrawRect(x*scale, y*scale, scale, scale, window, sf::Color(50,50,50));
          }
        }
      }
    }
    for (Particle &p : particles) {
      DrawDot(p.x*scale, p.y*scale, 3, window, HueToRGB(p.t * 0.1));
    }
  }
};

//-------------------------------------------------------- Main
int main()
{
    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    //settings.antialiasingLevel = 8;
    //sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "feromones", sf::Style::Fullscreen, settings);
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "feromones", sf::Style::Default, settings);
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

    int scale = 20;
    Arena arena(SCREEN_SIZE_X/scale, SCREEN_SIZE_Y/scale, scale, 10000, 5);

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
            DRAW_CELLS = false;
          } else {
            DRAW_CELLS = true;
          }
        }

        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)){
        //   if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
        //     ANTS_SPEED /= 1.05;
        //   } else {
        //     ANTS_SPEED *= 1.05;
        //   }
        //   ANTS_SPEED = std::min(10.0f, std::max(0.1f, ANTS_SPEED));
        // }

        /// if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
        ///   arena.reset();
        /// }

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
        DrawText(5,10, "FPS: " + std::to_string(framesPerSec), 16, window, sf::Color(255,255,255));
        DrawText(5,30, "avg: " + std::to_string(framesPerSec_avg), 16, window, sf::Color(255,255,255));
        DrawText(5,50, "ticker: " + std::to_string(ticker), 16, window, sf::Color(255,255,255));

	window.display();
    }
    return 0;
}
