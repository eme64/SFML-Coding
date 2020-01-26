#include <iostream>
#include <map>
#include <vector>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <cassert>
#include <string>

#include <fstream>
#include <streambuf>

#include "evp_server.hpp"

// Ideas:
//  - file adapter
//  - login: unique name - nonce


class Color {
public:
   float r,g,b,a;//0..1
   Color(const float r,const float g,const float b,const float a=1.0) {
     this->r = std::min(1.0f,std::max(0.0f,r));
     this->g = std::min(1.0f,std::max(0.0f,g));
     this->b = std::min(1.0f,std::max(0.0f,b));
     this->a = std::min(1.0f,std::max(0.0f,a));
   }
 
   Color operator*(const float scale) {return Color(scale*r,scale*g,scale*b,a);}
 
   sf::Color toSFML() const {
     return sf::Color(255.0*r,255.0*g,255.0*b,255.0*a);
   }
protected:
};


void DrawRect(float x, float y, float dx, float dy, sf::RenderTarget &target, const Color& color) {
   sf::RectangleShape rectangle;
   rectangle.setSize(sf::Vector2f(dx, dy));
   rectangle.setFillColor(color.toSFML());
   rectangle.setPosition(x, y);
   target.draw(rectangle, sf::BlendAlpha);//BlendAdd
}

class TestServer : public evp::FileServer {
public:
   double dx,dy;
   TestServer() {
      // register files:
      registerString("",std::string("")
              +"<html> <head> <title> REDIRECT </title>\n"
	      +"<meta http-equiv='Refresh' content='0; url=login.html'/>\n"
	      +"</head>\n"
              +"<body>\n"
              +"<p> redirecting...\n"
              +"</body> </html>"
      );
      registerFile("login.html","login.html");

      evp::FileServer::FunctionItem::F loginFunc = [&](const evp::URL &url) -> std::string {
	 std::string name = url.paramString("name","");
         return std::string("Hello ") + name + "!";
      };
      registerFunction("login2.html", loginFunc);

      registerFile("index.html","index.html");
      registerFile("script.js","script.js");
      registerFile("img.png","img.png");

      registerString("hello.html",std::string("")
              +"<html> <head> <title> TITLE </title> </head>\n"
              +"<body>\n"
              +"<p> hello world\n"
              +"<img src='img.png' alt='subtitle'>\n"
              +"</body> </html>"
      );

      evp::FileServer::FunctionItem::F func = [&](const evp::URL &url) -> std::string {
         dx = std::stod(url.paramString("dx","0"));
         dy = std::stod(url.paramString("dy","0"));
         return std::string("hello");
      };
      registerFunction("data", func);
   }
};


int main(int argc, char** argv) {
   std::cout << "Starting..." << std::endl;
   
   TestServer server;
   
   sf::ContextSettings settings;
   settings.antialiasingLevel = 8;
   sf::RenderWindow* window = new sf::RenderWindow(
       	    sf::VideoMode(800,600),
       	    "Window",
       	    sf::Style::Default,
       	    settings
       	    );
   
   float mouseX;
   float mouseY;
   
   float posX = 0;
   float posY = 0;

   while(true) {
      // --------------- Server
      server.run();
      
      // --------------- Events
      sf::Event event;
      while (window->pollEvent(event)) {
         switch (event.type) {
            case sf::Event::Closed: {
	      std::cout << "closing window?" << std::endl;
              break;
            }
  	    case sf::Event::MouseMoved: {
               mouseX = event.mouseMove.x;
               mouseY = event.mouseMove.y;
               break;
	    }
	 }
      }

      // --------------- DRAW
      window->clear();
      
      DrawRect(mouseX, mouseY, 5,5, *window, Color(1,1,1));
      
      posX = std::min(800.0, std::max(0.0, posX + 5*server.dx));
      posY = std::min(600.0, std::max(0.0, posY + 5*server.dy));
      DrawRect(posX-5, posY-5, 10,10, *window, Color(1,0,0));
      window->display();
   }
   
   return 0;
}
