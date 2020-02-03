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

// Idea Rooms:
//  - idea is to use a "room" for things like lobby, one per game, ...
//  - one room active, gets to display also.
//  - can switch to next room
//  - rooms set inputs / register listeners
//     - automatic id/string generation
//  - rooms can have their own userid->data map
//
// Impl:
//  - RoomServer extends FileServer
//  - map of id->Room, room has ptr to RoomServer
//  - RoomServer has active Room, Room can call other Room
//  - User holds what controls are active
//  - On room activation, users should be revisited to reset controls
//  - on draw, can take controls info from user

// ToDo:
//  - next: integrate controls
//  - timeout on user? / logout?


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

class MyUserData : public evp::UserData {
public:
   float x=100,y=100;
private:
};

class MyRoom : public evp::Room {
public:
   MyRoom(const std::string &name, evp::RoomServer* server)
   : evp::Room(name,server) {}
   virtual void draw(sf::RenderTarget &target) {
      evp::Room::UserVisitF f = [&] (evp::User* user, evp::UserData* raw) {
         MyUserData* data = dynamic_cast<MyUserData*>(raw);
         DrawRect(data->x,data->y, 5,5, target, Color(1,0,0));
      };
      visitUsers(f);
   }
   virtual evp::UserData* newUserData(const evp::User* u) { return new MyUserData();}
   virtual void onActivate() {
      std::cout << "MyActivate " << name() << std::endl;
   }
   virtual void onActivateUser(evp::User* const u) {
      std::cout << "MyActivateUser " << name() << " " << u->name() << std::endl;
   }
private:
};


int main(int argc, char** argv) {
   std::cout << "Starting..." << std::endl;
   
   evp::RoomServer server;
   MyRoom* room = new MyRoom("lobby",&server);
   server.setActive("lobby");

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
