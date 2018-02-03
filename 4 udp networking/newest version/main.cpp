#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <list>
#include <queue>

struct NServer;
struct NClient;
//----------------------------------- STATIC VARIABLES
float SCREEN_SIZE_X = 800;
float SCREEN_SIZE_Y = 600;

sf::Font FONT;

bool NET_SERVER_MODE;
NServer* NET_SERVER;
NClient* NET_CLIENT;

// --------------------------------------------------------- DRAWING
void DrawDot(float x, float y, sf::RenderWindow &window, sf::Color color = sf::Color(255,0,0))
{
  float r = 3;
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

// ------------------------------------------------------- Networking
// just send a string
const int NET_HEADER_ECHO = 100;

// announce the recieve port of client
const int NET_HEADER_CLIENT_ANNOUNCE = 2000;


struct NServer
{
  unsigned short socket_port = 54000;
  sf::UdpSocket socket;

  NServer()
  {
    // bind socket:
    if(socket.bind(socket_port) != sf::Socket::Done)
    {
      std::cout << "could not bind socket!" << std::endl;
      return;
    }
    socket.setBlocking(false);
    std::cout << "Server Created." << std::endl;
  }

  void draw(sf::RenderWindow &window)
  {
    DrawText(5,5, "Server Mode.", 12, window, sf::Color(255,0,0));
  }

  void update()
  {
    sf::Packet p;
    sf::IpAddress remoteIp;
    unsigned short remotePort;
    auto status = socket.receive(p, remoteIp, remotePort);

    if(status == sf::Socket::Done)
    {
      int header;
      p >> header;

      switch (header)
      {
          case NET_HEADER_ECHO:
            {
              std::string txt;
              p >> txt;

              std::cout << "IP: " << remoteIp << " : " << remotePort << std::endl;
              std::cout << "Echo: " << txt << std::endl;

              break;
            }
            case NET_HEADER_CLIENT_ANNOUNCE:
              {
                unsigned short clientPort;
                p >> clientPort;

                std::cout << "IP: " << remoteIp << " : " << remotePort << std::endl;
                std::cout << "Port Announced: " << clientPort << std::endl;
                
                break;
              }

          // we don't process other types of events
          default:   // ------------ DEFAULT
            std::cout << "unreadable header: "<< header << std::endl;
            break;
      }
    }
  }
};

struct NClient
{
  sf::UdpSocket socket;
  unsigned short destination_port = 54000;
  sf::IpAddress recipient = "localhost";

  unsigned short socket_port;

  NClient()
  {
    if(socket.bind(sf::Socket::AnyPort) != sf::Socket::Done)
    {
      std::cout << "could not bind socket!" << std::endl;
      return;
    }
    socket_port = socket.getLocalPort();

    send(NET_HEADER_ECHO, "hello over there !");
    announcePort();

    std::cout << "Client Created. listen on " << socket_port << std::endl;
  }

  void draw(sf::RenderWindow &window)
  {
    DrawText(5,5, "Client Mode.", 12, window, sf::Color(0,255,0));
  }

  void update()
  {

  }

  void send(int header, const std::string &txt)
  {
    sf::Packet p;
    p << header << txt;

    if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void announcePort()
  {
    sf::Packet p;
    p << NET_HEADER_CLIENT_ANNOUNCE << socket_port;

    if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }
};


//-------------------------------------------------------- Main
int main(int argc, char** argv)
{
    // ----------------------------- Handle Commandline Input
    NET_SERVER_MODE = false;
    if(argc > 1)
    {
      std::string serv = "server";
      std::string arg0 = argv[1];
      if(!arg0.compare(serv))
      {
        std::cout << "Server Mode!" << std::endl;
        NET_SERVER_MODE = true;
      }
    }

    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "first attempts", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);

    // ------------------------------------ FONT
    if (!FONT.loadFromFile("arial.ttf"))
    {
        std::cout << "font could not be loaded!" << std::endl;
        return 0;
    }

    // --------------------------- Setup Networking

    if(NET_SERVER_MODE)
    {
      // server
      NET_SERVER = new NServer();
    }else{
      // client
      NET_CLIENT = new NClient();
    }

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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)){
          if(!NET_SERVER_MODE)
          {
            std::string txt = "hello " + std::to_string(rand());
            NET_CLIENT->send(NET_HEADER_ECHO, txt);
          }
        }

        window.clear();

        if(NET_SERVER_MODE)
        {
          // server
          NET_SERVER->update();
        }else{
          // client
          NET_CLIENT->update();
        }

        if(NET_SERVER_MODE)
        {
          // server
          NET_SERVER->draw(window);
        }else{
          // client
          NET_CLIENT->draw(window);
        }

        window.display();
    }

    return 0;
}
