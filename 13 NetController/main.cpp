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


void DrawRect(float x, float y, float dx, float dy, sf::RenderTarget &target, const Color& color, float w=0.0) {
   sf::RectangleShape rectangle;
   rectangle.setSize(sf::Vector2f(dx, dy));
   rectangle.setFillColor(color.toSFML());
   rectangle.setPosition(x, y);
   rectangle.setRotation(w);
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

class Particle {
public:
   Particle(float x,float y, float dx, float dy, float w, float dw, float s, const Color &color, int t)
   : x(x),y(y),dx(dx),dy(dy), w(w),dw(dw), s(s), color(color), t0(t),t(t) {}
   bool draw(sf::RenderTarget &target) {
      w+=dw;
      x+=dx;
      y+=dy;
      DrawRect(x, y, s,s, target, Color(color.r, color.g, color.b, ((float) t) / ((float) t0)), w);
      return (t--)>0;
   }
private:
   float x,y,dx,dy,w,dw,s;
   int t0,t;
   Color color;
};

class RocketUserData : public evp::UserData {
public:
   float x=100, y=100;
   float dx=0, dy=0;
   float w = 90;
   bool up=false, left=false, right=false;
private:
};

class RocketRoom : public evp::Room {
public:
   RocketRoom(const std::string &name, evp::RoomServer* server)
   : evp::Room(name,server) {
      sf::ConvexShape polygon;
      polygon.setPointCount(5);
      polygon.setPoint(0, sf::Vector2f(1, 0));
      polygon.setPoint(1, sf::Vector2f(0.7, 0.2));
      polygon.setPoint(2, sf::Vector2f(-1, 0.2));
      polygon.setPoint(3, sf::Vector2f(-1, -0.2));
      polygon.setPoint(4, sf::Vector2f(0.7, -0.2));
      rocket_.push_back(polygon);

      x0_=20;
      y0_=20;
      x1_=780;
      y1_=580;
   }
   virtual void draw(sf::RenderTarget &target) {
      DrawRect(x0_,y0_, x1_-x0_, y1_-y0_, target, Color(0.1,0.1,0.1));
      evp::Room::UserVisitF f = [&] (evp::User* user, evp::UserData* raw) {
         RocketUserData* data = dynamic_cast<RocketUserData*>(raw);
	 
	 if(data->left) {data->w-=5;}
	 if(data->right) {data->w+=5;}
         
	 if(data->up) {
            const float dxx = std::cos(data->w / 360.0 * 2.0 * M_PI);
            const float dyy = std::sin(data->w / 360.0 * 2.0 * M_PI);
            data->dx+=0.1*dxx;
            data->dy+=0.1*dyy;

	    particles_.insert(new Particle(data->x, data->y, data->dx-dxx*2, data->dy-dyy*2, 0,0, 3, Color(1,0.5,0.2), 60));
	 }
         
	 data->dy+=0.01;
	 data->x += data->dx;
	 data->y += data->dy;
	 data->dx*=0.99;
	 data->dy*=0.99;
	 
         if(data->x < x0_) {data->x = x0_; data->dx = 0;}
         if(data->x > x1_) {data->x = x1_; data->dx = 0;}
         if(data->y < y0_) {data->y = y0_; data->dy = 0;}
         if(data->y > y1_) {data->y = y1_; data->dy = 0;}
         
	 drawRocket(data->x,data->y, 10, data->w, target, Color(0.5 + data->left,0.5 + data->right, 0.5 + data->up));
      };
      visitUsers(f);
      
      for(auto it = particles_.begin(); it!=particles_.end(); it++) {
         Particle *p = *it;
	 if(! p->draw(target)) {
	    delete p;
	    particles_.erase(it);
	 }
      }
   }
   virtual evp::UserData* newUserData(const evp::User* u) { return new RocketUserData();}
   virtual void onActivate() {
      std::cout << "RocketActivate " << name() << std::endl;
   }
   virtual void onActivateUser(evp::User* const u,evp::UserData* const raw) {
      RocketUserData* data = dynamic_cast<RocketUserData*>(raw);
      std::cout << "RocketActivateUser " << name() << " " << u->name() << std::endl;
   
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
   float x0_,y0_,x1_,y1_; // limits of room
   std::set<Particle*> particles_;
   std::vector<sf::ConvexShape> rocket_;

   void drawRocket(float x,float y, float scale, float w, sf::RenderTarget &target, const Color& color) {
      for(sf::Shape &s : rocket_) {
         s.setFillColor(color.toSFML());
         s.setPosition(x,y);
	 s.setScale(scale,scale);
	 s.setRotation(w);
	 target.draw(s, sf::BlendAlpha);
      }
   }
};



int main(int argc, char** argv) {
   std::cout << "Starting..." << std::endl;
   
   evp::RoomServer server;
   LobbyRoom* lobby = new LobbyRoom("lobby",&server);
   MyRoom* game1 = new MyRoom("game1",&server);
   RocketRoom* game2 = new RocketRoom("game2",&server);
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
      
      DrawRect(mouseX, mouseY, 5,5, *window, Color(1,1,1));
      server.draw(*window);
      
      window->display();
   }
   
   return 0;
}
