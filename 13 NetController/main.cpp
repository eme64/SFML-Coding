#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <cmath>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <cassert>
#include <string>

#include <fstream>
#include <streambuf>

#include "evp_server.hpp"
#include "evp_draw.hpp"

#include "voronoi_room.hpp"
#include "rocket_room.hpp"
#include "space_gen.hpp"

// Ideas:
//  - file adapter
//  - login: unique name - nonce
//  - logout / timeout: keep player name??? -> may depend on room?
//     - room defines if it keeps user info for disconnected users

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

// Ideas:
// - rts:
//   - commander places buildings
//   - maybe have multiple teleport locations?
//   - mine for mass and energy, relay to base
//   - build things with mass, weapons use energy
//   - commander cannot attack, only build
//   - commander can make units follow him
//
// - race game:
//   - run through voronoy map to reach goal, tiles have properties
//
// - other inspirations: overcooked, ibb and obb, snipperclips, bomberman, think of the children, catastronauts 
// - rts coop, tower defense style? or clean up map?
// - adios amigos (space exploration, rogue like, physics, collision)

// Next steps for space game:
// - phyics: collision, mass, impulse, rotations -> orbital mechanics - simple rocket demo.
// - integrate CellMap and physics.
// - rocket mechanics?
// - possibly LOD CellMap
// - editor: terrain + rockets?
// - proper interaction mechanics - maybe script?

class LobbyRoom : public evp::Room {
public:
   LobbyRoom(const std::string &name, evp::RoomServer* server)
   : evp::Room(name,server) {
      so_ = new SpaceObject();
   }
   virtual void draw(sf::RenderTarget &target) {
      so_->draw(target,x,y,s,mx,my);
   }
   virtual void onActivate() {
      std::cout << "MyActivate " << name() << std::endl;
   }
   virtual void onActivateUser(evp::User* const u,evp::UserData* const raw) {
      std::cout << "MyActivateUser " << name() << " " << u->name() << std::endl;
   
      u->clearControls();
      u->registerControl(new evp::BGColorControl(u->nextControlId(),evp::Color(0.9,0.9,0.9)));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.0,0.9,0.2,"Digit1",
			      [this](bool down) {
				 if(down) {this->server()->setActive("game1");}
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.25,0.9,0.2,"Digit2",
			      [this](bool down) {
				 if(down) {this->server()->setActive("game2");}
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.5,0.9,0.2,"Digit3",
			      [this](bool down) {
				 if(down) {this->server()->setActive("game3");}
			      }));
      u->registerControl(new evp::KnobControl(u->nextControlId(),0.05,.75,0.4,0.25,true,1,
			      [&](bool down, float dx, float dy) {
				 x+= dx;
				 y+= dy;
			      }));
      u->registerControl(new evp::KnobControl(u->nextControlId(),0.55,.75,0.4,0.25,true,1,
			      [&](bool down, float dx, float dy) {
				 s *= std::pow(10,dy);
			      }));
   }
   void setM(float x,float y) {mx=x; my=y;}
private:
   SpaceObject* so_;
   float mx,my;
   float x=0,y=0,s=290;
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
            evp::DrawRect(data->x,data->y, 5,5, target, evp::Color(0,1,0));
	 } else {
	    evp::DrawRect(data->x,data->y, 5,5, target, evp::Color(1,0,0));
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
      u->registerControl(new evp::BGColorControl(u->nextControlId(),evp::Color(0.7,0.9,0.9)));
      u->registerControl(new evp::KnobControl(u->nextControlId(),0.05,0.55,0.4,0.4,true,1,
			      [data](bool down, float dx, float dy) {
				 data->x += dx*100;
				 data->y += dy*100;
				 data->down = down;
			      }));
      u->registerControl(new evp::KnobControl(u->nextControlId(),0.55,0.55,0.4,0.4,false,0.15,
			      [data](bool down, float dx, float dy) {
				 data->x += dx;
				 data->y += dy;
				 data->down = down;
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.05,0.9,0.4,"Escape",
			      [this](bool down) {
				 if(down) {this->server()->setActive("lobby");}
			      }));

   }
private:
};


int main(int argc, char** argv) {
   std::cout << "Starting..." << std::endl;
   
   evp::RoomServer server;
   LobbyRoom* lobby = new LobbyRoom("lobby",&server);
   MyRoom* game1 = new MyRoom("game1",&server);
   RocketRoom* game2 = new RocketRoom("game2",&server);
   VoronoiRoom* game3 = new VoronoiRoom("game3",&server, 800,600);
   server.setActive("lobby");

   sf::ContextSettings settings;
   settings.antialiasingLevel = 8;
   sf::RenderWindow* window = new sf::RenderWindow(
       	    sf::VideoMode(800,600),
       	    "Window",
       	    sf::Style::Default,
       	    settings
       	    );
   window->setFramerateLimit(60);
   
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
     
      lobby->setM(mouseX,mouseY);
      server.draw(*window);
      evp::DrawRect(mouseX, mouseY, 5,5, *window, evp::Color(1,1,1));
      
      evp::DrawRect(0,0, 200,25, *window, evp::Color(0,0,0,0.8));
      evp::DrawText(0,0, "Port: "+std::to_string(server.getPort()), 20, *window, evp::Color(0.7,0.7,1));

      window->display();
   }
   
   return 0;
}
