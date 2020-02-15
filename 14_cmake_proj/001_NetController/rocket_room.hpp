

#include "evp/server.hpp"
#include "evp/draw.hpp"

#ifndef ROCKET_ROOM_HPP
#define ROCKET_ROOM_HPP



class Particle {
public:
   Particle(float x,float y, float dx, float dy, float w, float dw, float s, const evp::Color &color, int t)
   : x(x),y(y),dx(dx),dy(dy), w(w),dw(dw), s(s), color(color), t0(t),t(t) {}
   bool draw(sf::RenderTarget &target) {
      w+=dw;
      x+=dx;
      y+=dy;
      DrawRect(x, y, s,s, target, evp::Color(color.r, color.g, color.b, ((float) t) / ((float) t0)), w);
      return (t--)>0;
   }
private:
   float x,y,dx,dy,w,dw,s;
   int t0,t;
   evp::Color color;
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
      DrawRect(x0_,y0_, x1_-x0_, y1_-y0_, target, evp::Color(0.1,0.1,0.1));
      evp::Room::UserVisitF f = [&] (evp::User* user, evp::UserData* raw) {
         RocketUserData* data = dynamic_cast<RocketUserData*>(raw);
	 
	 if(data->left) {data->w-=5;}
	 if(data->right) {data->w+=5;}
         
	 if(data->up) {
            const float dxx = std::cos(data->w / 360.0 * 2.0 * M_PI);
            const float dyy = std::sin(data->w / 360.0 * 2.0 * M_PI);
            data->dx+=0.1*dxx;
            data->dy+=0.1*dyy;

	    particles_.insert(new Particle(data->x, data->y, data->dx-dxx*2, data->dy-dyy*2, 0,0, 3, evp::Color(1,0.5,0.2), 60));
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
         
	 drawRocket(data->x,data->y, 10, data->w, target, evp::Color(0.5 + data->left,0.5 + data->right, 0.5 + data->up));
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
      u->registerControl(new evp::BGColorControl(u->nextControlId(),evp::Color(0.1,0.2,0.3)));
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

   void drawRocket(float x,float y, float scale, float w, sf::RenderTarget &target, const evp::Color& color) {
      for(sf::Shape &s : rocket_) {
         s.setFillColor(color.toSFML());
         s.setPosition(x,y);
	 s.setScale(scale,scale);
	 s.setRotation(w);
	 target.draw(s, sf::BlendAlpha);
      }
   }
};



#endif // ROCKET_ROOM_HPP
