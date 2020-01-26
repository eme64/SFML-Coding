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
      registerFile("","index.html");
      registerFile("index.html","index.html");
      registerFile("script.js","script.js");
      registerFile("img.png","img.png");
   }
   //void handleRequest(std::string &ret, const std::string &url) {
   //   std::cout << "url: " << url << std::endl;
   //   if(url == "img.png") {
   //      std::ifstream t(url);
   //      std::string data((std::istreambuf_iterator<char>(t)),
   //              std::istreambuf_iterator<char>());
   //      ret = std::string("HTTP/1.0 200 Ok\n")
   //     	 + "Content-Type: image/png\n"
   //     	 + "Content-Length: " + std::to_string(data.size()) + "\n"
   //     	 + "\n"
   //     	 +data;
   //      return;
   //   }
   //   if(url == "script.js") {
   //      std::ifstream t(url);
   //      std::string data((std::istreambuf_iterator<char>(t)),
   //              std::istreambuf_iterator<char>());
   //      ret = std::string("HTTP/1.0 200 Ok\n")
   //              + "Content-Type: text/javascript;charset=UTF-8\n"
   //     	 + "Content-Length: " + std::to_string(data.size()) + "\n"
   //     	 + "\n"
   //     	 +data;
   //      return;
   //   }
   //   if(url == "") {
   //      std::ifstream t("index.html");
   //      std::string data((std::istreambuf_iterator<char>(t)),
   //              std::istreambuf_iterator<char>());
   //      ret = evp::Server::HTTP_text
   //     	 +data;
   //      return;
   //   }
   //   
   //   if(url.find("data") == 0) {
   //      std::cout << "data/ -> " << url << std::endl;
   //      std::string data = "hello world";
   //      ret = std::string("HTTP/1.0 200 Ok\n")
   //              + "Content-Type: text/javascript;charset=UTF-8\n"
   //     	 + "Content-Length: " + std::to_string(data.size()) + "\n"
   //     	 + "\n"
   //     	 +data;

   //      std::vector<std::string> urlParts = evp::split(url,'?');
   //      if(urlParts.size()>1) {
   //         std::vector<std::string> parts = evp::split(urlParts[1],'&');
   //         for(std::string &s : parts) {
   //            std::vector<std::string> param = evp::split(s,'=');
   //            if(param.size()>1) {
   //               if(param[0]=="dx") {
   //     	     dx = std::stod(param[1]);
   //     	  } else if(param[0]=="dy") {
   //     	     dy = std::stod(param[1]);
   //     	  }
   //            }
   //         }
   //      }
   //      return;
   //   }

   //   
   //   ret = evp::Server::HTTP_text
   //           +"<html> <head> <title> TITLE </title> </head>\n"
   //           +"<body>\n"
   //           +"<p> hello world\n"
   //           +"<img src='img.png' alt='subtitle'>\n"
   //           +"<script src='script.js'></script>"
   //           +"</body> </html>";
   //}
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
