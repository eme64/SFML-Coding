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

class LobbyRoom : public evp::Room {
public:
   LobbyRoom(const std::string &name, evp::RoomServer* server)
   : evp::Room(name,server) {}
   virtual void draw(sf::RenderTarget &target) {
      DrawRect(10,10, 100,100, target, Color(0.5,0.5,0));
      //evp::Room::UserVisitF f = [&] (evp::User* user, evp::UserData* raw) {
      //   MyUserData* data = dynamic_cast<MyUserData*>(raw);
      //   if(data->down) {
      //      DrawRect(data->x,data->y, 5,5, target, Color(0,1,0));
      //   } else {
      //      DrawRect(data->x,data->y, 5,5, target, Color(1,0,0));
      //   }
      //};
      //visitUsers(f);
   }
   virtual void onActivate() {
      std::cout << "MyActivate " << name() << std::endl;
   }
   virtual void onActivateUser(evp::User* const u,evp::UserData* const raw) {
      std::cout << "MyActivateUser " << name() << " " << u->name() << std::endl;
   
      u->clearControls();
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.05,0.9,0.4,"Digit1",
			      [this](bool down) {
				 if(down) {this->server()->setActive("game1");}
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.55,0.9,0.4,"Digit2",
			      [this](bool down) {
				 if(down) {this->server()->setActive("game2");}
			      }));
   }
private:
};




class MyUserData : public evp::UserData {
public:
   float x=100,y=100;
   bool down = false;
private:
};

class MyRoom : public evp::Room {
public:
   MyRoom(const std::string &name, evp::RoomServer* server)
   : evp::Room(name,server) {}
   virtual void draw(sf::RenderTarget &target) {
      evp::Room::UserVisitF f = [&] (evp::User* user, evp::UserData* raw) {
         MyUserData* data = dynamic_cast<MyUserData*>(raw);
	 if(data->down) {
            DrawRect(data->x,data->y, 5,5, target, Color(0,1,0));
	 } else {
            DrawRect(data->x,data->y, 5,5, target, Color(1,0,0));
	 }
      };
      visitUsers(f);
   }
   virtual evp::UserData* newUserData(const evp::User* u) { return new MyUserData();}
   virtual void onActivate() {
      std::cout << "MyActivate " << name() << std::endl;
   }
   virtual void onActivateUser(evp::User* const u,evp::UserData* const raw) {
      MyUserData* data = dynamic_cast<MyUserData*>(raw);
      std::cout << "MyActivateUser " << name() << " " << u->name() << std::endl;
   
      u->clearControls();
      u->registerControl(new evp::KnobControl(u->nextControlId(),0.05,0.05,0.4,0.4,true,1,
			      [data](bool down, float dx, float dy) {
				 data->x += dx*100;
				 data->y += dy*100;
				 data->down = down;
			      }));
      u->registerControl(new evp::KnobControl(u->nextControlId(),0.55,0.05,0.4,0.4,false,0.15,
			      [data](bool down, float dx, float dy) {
				 data->x += dx;
				 data->y += dy;
				 data->down = down;
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.55,0.9,0.4,"Escape",
			      [this](bool down) {
				 if(down) {this->server()->setActive("lobby");}
			      }));

   }
private:
};


class PlatformUserData : public evp::UserData {
public:
   float x=100, y=100;
   float dx=0, dy=0;
   bool up=false, left=false, right=false;
private:
};

class PlatformRoom : public evp::Room {
public:
   PlatformRoom(const std::string &name, evp::RoomServer* server)
   : evp::Room(name,server) {}
   virtual void draw(sf::RenderTarget &target) {
      evp::Room::UserVisitF f = [&] (evp::User* user, evp::UserData* raw) {
         PlatformUserData* data = dynamic_cast<PlatformUserData*>(raw);
	 
	 if(data->left) {data->dx-=0.01;}
	 if(data->right) {data->dx+=0.01;}
	 if(data->y <= 0 and data->up) {data->dy=2;}

         data->dx = std::min(data->dx,2.0f);
         data->dx = std::max(data->dx,-2.0f);
	 data->x += data->dx;
	 data->dx *= 0.99;
	 data->x = std::max(100.0f, std::min(700.0f, data->x));

	 data->dy -= 0.01;
	 data->y += data->dy;
	 data->y = std::max(0.0f, data->y);
         
         DrawRect(data->x,500 - data->y, 5,5, target, Color(0.5 + data->left,0.5 + data->right, 0.5 + data->up));
      };
      visitUsers(f);
   }
   virtual evp::UserData* newUserData(const evp::User* u) { return new PlatformUserData();}
   virtual void onActivate() {
      std::cout << "PlatformActivate " << name() << std::endl;
   }
   virtual void onActivateUser(evp::User* const u,evp::UserData* const raw) {
      PlatformUserData* data = dynamic_cast<PlatformUserData*>(raw);
      std::cout << "PlatformActivateUser " << name() << " " << u->name() << std::endl;
   
      u->clearControls();
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.05,0.9,0.2,"Escape",
			      [this](bool down) {
				 if(down) {this->server()->setActive("lobby");}
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0,0.4,0.3,0.5,"KeyA",
			      [data](bool down) {
				 data->left = down;
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.35,0.3,0.3,0.5,"KeyW",
			      [data](bool down) {
				 data->up = down;
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.7,0.4,0.3,0.5,"KeyD",
			      [data](bool down) {
				 data->right = down;
			      }));
   }
private:
};



int main(int argc, char** argv) {
   std::cout << "Starting..." << std::endl;
   
   evp::RoomServer server;
   LobbyRoom* lobby = new LobbyRoom("lobby",&server);
   MyRoom* game1 = new MyRoom("game1",&server);
   PlatformRoom* game2 = new PlatformRoom("game2",&server);
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
