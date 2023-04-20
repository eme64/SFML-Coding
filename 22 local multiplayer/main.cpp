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

#include "draw.hpp"
#include "arena.hpp"
#include "lobby.hpp"

// Some basic ideas:
//
// Lobby:
//  add / remove players / choose colors etc
//  Walk onto teleporter -> if all on it go into that game
//
// User:
//  Color, uid, controls
//  Per Room, some user-info
//

//----------------------------------- STATIC VARIABLES
float SCREEN_SIZE_X = 1500;
float SCREEN_SIZE_Y = 1200;

//-------------------------------------------------------- Main
int main()
{
    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    //settings.antialiasingLevel = 8;
    //sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "games", sf::Style::Fullscreen, settings);
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "games", sf::Style::Default, settings);
    sf::Vector2u size = window.getSize();
    SCREEN_SIZE_X = size.x;
    SCREEN_SIZE_Y = size.y;
    window.setVerticalSyncEnabled(true);

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

    int scale = 50;
    Arena arena(SCREEN_SIZE_X/scale, SCREEN_SIZE_Y/scale, scale);
    Control* ctrl_0 = arena.new_ctrl();
    Control* ctrl_1 = arena.new_ctrl();
    RoomLobby lobby("lobby");
    arena.add_room(&lobby);
    arena.new_user();
    arena.new_user();

    // --------------------------- Render Loop
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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)){ ctrl_0->up(); }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)){ ctrl_0->down(); }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)){ ctrl_0->left(); }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)){ ctrl_0->right(); }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up   )){ ctrl_1->up(); }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down )){ ctrl_1->down(); }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left )){ ctrl_1->left(); }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){ ctrl_1->right(); }

        bool display_fps = false;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F1)) {
          display_fps = true;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }

        window.clear();

        arena.update();
        arena.draw(window);

        FPS_tick();
        if (display_fps) {
          DrawText(5,10, "FPS: " + std::to_string(framesPerSec), 16, window, sf::Color(255,255,255));
          DrawText(5,30, "avg: " + std::to_string(framesPerSec_avg), 16, window, sf::Color(255,255,255));
          DrawText(5,50, "ticker: " + std::to_string(ticker), 16, window, sf::Color(255,255,255));
        }

        window.display();
    }
    return 0;
}
