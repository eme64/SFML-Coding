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

class TestServer : public evp::Server {
   void handleRequest(std::string &ret, const std::string &url) {
      std::cout << "url: " << url << std::endl;
      if(url == "img.png") {
	 std::ifstream t(url);
	 std::string data((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
         ret = std::string("HTTP/1.0 200 Ok\n")
		 + "Content-Type: image/png\n"
		 + "Content-Length: " + std::to_string(data.size()) + "\n"
		 + "\n"
		 +data;
         return;
      }
      if(url == "script.js") {
	 std::ifstream t(url);
	 std::string data((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
         ret = std::string("HTTP/1.0 200 Ok\n")
                 + "Content-Type: text/javascript;charset=UTF-8\n"
		 + "Content-Length: " + std::to_string(data.size()) + "\n"
		 + "\n"
		 +data;
         return;
      }
      if(url == "") {
	 std::ifstream t("index.html");
	 std::string data((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
         ret = evp::Server::HTTP_text
		 +data;
         return;
      }
      
      if(url.find("data/") == 0) {
         std::cout << "data/ -> " << url << std::endl;
	 std::string data = "hello world";
         ret = std::string("HTTP/1.0 200 Ok\n")
                 + "Content-Type: text/javascript;charset=UTF-8\n"
		 + "Content-Length: " + std::to_string(data.size()) + "\n"
		 + "\n"
		 +data;
	 return;
      }

      
      ret = evp::Server::HTTP_text
	      +"<html> <head> <title> TITLE </title> </head>\n"
	      +"<body>\n"
	      +"<p> hello world\n"
	      +"<img src='img.png' alt='subtitle'>\n"
	      +"<script src='script.js'></script>"
	      +"</body> </html>";
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

      window->display();
   }
   
   return 0;
}
