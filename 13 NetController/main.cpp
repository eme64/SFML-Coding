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
         // validate name:
	 bool success = false;
	 std::string id;
	 login(name, id, success);
	 if(!success) {
            return std::string("")
              +"<html> <head> <title>LOGIN FAILED</title>\n"
	      +"<meta http-equiv='Refresh' content='10; url=login.html'/>\n"
	      +"</head>\n"
              +"<body>\n"
	      +"<p>Login failed, name "+name+" already in use!</br>\n"
	      +"<button onclick='window.location=\"login.html\"'>Try Again</button>\n"
	      +"</body> </html>";
	 } else {
            return std::string("")
              +"<html> <head> <title>LOGIN SUCESS - Redirect</title>\n"
	      +"<meta http-equiv='Refresh' content='1; url=index.html'/>\n"
	      +"</head>\n"
              +"<body>\n"
              +"<script>\n"
	      +"document.cookie = 'user="+name+"'\n"
	      +"document.cookie = 'userid="+id+"'\n"
              +"</script>\n"
	      +"<p> Welcome, "+name+"!\n"
	      +"</body> </html>";
	 }
      };
      registerFunction("login2.html", loginFunc);

      registerFile("index.html","index.html");
      registerFile("test.html","test.html");
      registerFile("script.js","script.js");
      registerFile("util.js","util.js");
      registerFile("img.png","img.png");
      registerFile("favicon.ico","favicon.ico");

      registerString("hello.html",std::string("")
              +"<html> <head> <title> TITLE </title> </head>\n"
              +"<body>\n"
              +"<p> hello world\n"
              +"<img src='img.png' alt='subtitle'>\n"
              +"</body> </html>"
      );

      evp::FileServer::FunctionItem::F func = [&](const evp::URL &url) -> std::string {
	 std::string id = url.paramString("uid","");
	 User* u = id2user(id);
	 if(u) {
            std::string s = url.paramString("0","");
	    if(s=="f") {
               u->set(0,0);
	    } else {
               const auto& pp = evp::split(s,',');
	       float dx = std::stod(pp[0]);
               float dy = std::stod(pp[1]);
               u->set(dx,dy);
	    }

            std::string s2 = url.paramString("5","");
	    u->setDown(s2=="t");

	    return std::string("ok");
	 } else {
            return std::string("error: user");
	 }
      };
      registerFunction("data", func);
   }
   
   void draw(sf::RenderTarget &target) {
      for(auto &it : name2user_) {
         it.second->draw(target);
      }
   }
   
   class User {
   public:
      User(const std::string &name) : name_(name) {}
      void setId(const std::string &id) {id_=id;}
      std::string name() const {return name_;}
      std::string id() const {return id_;}
      void set(float dx, float dy) {dx_=dx; dy_=dy;}
      void draw(sf::RenderTarget &target) {
         x_ = std::min(800.0, std::max(0.0, x_ + 0.1*dx_));
         y_ = std::min(600.0, std::max(0.0, y_ + 0.1*dy_));
         DrawRect(x_-5, y_-5, 10,10, target, Color(down_,1-down_,0));
      }
      void setDown(bool d) {down_=d;}
      bool down() {return down_;}
   private:
      std::string name_;
      std::string id_;
      double x_,y_,dx_,dy_;
      bool down_;
   };
private:
   void login(const std::string &inName, std::string &outId, bool &outSuccess) {
      const auto &it = name2user_.find(inName);
      if(it == name2user_.end()) {
         // user does not exist yet
	 outSuccess = true;
         User* u = new User(inName);
	 outId = newId();
	 u->setId(outId);
	 name2user_[inName] = u;
	 id2user_[outId] = u;
      } else {
         // user exists
	 outSuccess = false;
      }
   }
   std::string newId() {
      return std::to_string(++idCnt);
   }
   User* id2user(const std::string &id) {
      const auto &it = id2user_.find(id);
      if(it==id2user_.end()) {
         return NULL;
      } else {
         return it->second;
      }
   }
   
   int idCnt = 0;
   std::map<std::string, User*> name2user_;
   std::map<std::string, User*> id2user_;
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
      server.draw(*window);
      
      window->display();
   }
   
   return 0;
}
