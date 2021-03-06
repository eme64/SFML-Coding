#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <cmath>
#include <cassert>
#include <string>

#include "evp/server.hpp"
#include "evp/draw.hpp"
#include "evp/voronoi.hpp"


#include "evp/noise.hpp"

#ifndef VORONOI_ROOM_HPP
#define VORONOI_ROOM_HPP

class VoronoiUserData : public evp::UserData {
public:
   evp::Color color = evp::Color(1,1,1);
   float x=20,y=300;
   float dx=0,dy=0;
   float w=0;
   bool left,right;
   std::vector<float> trail;
   int score = 0;
private:
};


struct CInfo {
   enum Type {Normal, Blocked, Fast, Slow, Goal};
   Type t;
};

class VoronoiRoom : public evp::Room {
public:
   VoronoiRoom(const std::string &name, evp::RoomServer* server,float dx, float dy)
   : evp::Room(name,server), dx_(dx), dy_(dy) {
      vmap = new evp::VoronoiMap<CInfo>(3000, dx_,dy_);
      
      seed_=0;
      setupMap();
      setupUsers();
   }
   
   void setupMap() {
      evp::DetailNoise n0(seed_);
      evp::DetailNoise n1(seed_);
      evp::DetailNoise n2(seed_);
         
      for(int i=0; i<vmap->num_cells; i++) {
	 float x = vmap->cells[i].pos.x;
	 float y = vmap->cells[i].pos.y;
	 float h0 = n0.get(x,y)*(x<100?x*0.01:1.0)*((x>dx_-100)?(1.0-(x-dx_+100)*0.01):1.0);
	 float h1 = n1.get(x,y);
	 float h2 = n2.get(x,y)*(x<100?x*0.01:1.0)*((x>dx_-100)?(1.0-(x-dx_+100)*0.01):1.0);
         
	 evp::Color c(h0,h0,h0);
	 if(h0 > 0.57) {
	    vmap->cells[i].info.t = CInfo::Blocked;
	 } else {
            if(h2 > 0.55) {
	       vmap->cells[i].info.t = CInfo::Slow;
	       c = evp::Color((1-h1)*0.7,h0*0.3,h0*0.3);
	    } else if(h1>0.58) {
	       vmap->cells[i].info.t = CInfo::Fast;
	       c = evp::Color(h0*0.5,h1,h0*0.5);
	       c = evp::Color(h0*0.3,h1*0.7,h0*0.3);
	    } else {
	       vmap->cells[i].info.t = CInfo::Normal;
	       c = evp::Color(h0*0.2,h0*0.2,h0*0.2);
	    }
	 }
	 vmap->cells[i].color = c.toSFML();
      }

      auto &goal = vmap->cells[vmap->getCell(dx_-50,dy_*0.5,0)];
      goal.info.t = CInfo::Goal;
      goal.color = evp::Color(1,1,0).toSFML();

      vmap->create_mesh();
   }
   void setupUsers() {
      evp::Room::UserVisitF freset = [&] (evp::User* user, evp::UserData* raw) {
         VoronoiUserData* data = dynamic_cast<VoronoiUserData*>(raw);
         data->x = 50;
         data->y = dy_*0.5;
         data->dx = 0;
         data->dy = 0;
         data->w = 0;
	 data->trail.clear();
      };
      visitUsers(freset);
   }

   ~VoronoiRoom() {
      delete vmap;
   }

   virtual void draw(sf::RenderTarget &target) {
      if(winner_) {
         winner_=NULL;
	 setupMap();
         setupUsers();
      }
      
      vmap->draw(0,0,1,target);
      int ui = 0;
      evp::Room::UserVisitF f = [&] (evp::User* user, evp::UserData* raw) {
         VoronoiUserData* data = dynamic_cast<VoronoiUserData*>(raw);
	 
	 data->w += (1.0*data->right-1.0*data->left)*0.1;
	 float dxx = std::cos(data->w);
	 float dyy = std::sin(data->w);
	 data->dx += dxx*0.01;
	 data->dy += dyy*0.01;
         data->x += data->dx;
         data->y += data->dy;
	 data->dx *= 0.98;
	 data->dy *= 0.98;
	 
	 if(data->x < 10) {data->x = 10;data->dx=0;}
	 if(data->y < 10) {data->y = 10;data->dy=0;}
	 if(data->x > dx_-10) {data->x = dx_-10;data->dx=0;}
	 if(data->y > dy_-10) {data->y = dy_-10;data->dy=0;}
	 
         size_t cellI = vmap->getCell(data->x, data->y, 0);
         auto &cell = vmap->cells[cellI];
         
	 evp::Color c = data->color;
	 if(cell.info.t == CInfo::Goal) {
	    setWinner(user);
	    data->score++;
	 } else if(cell.info.t == CInfo::Blocked) {
	    float ddx = data->x - cell.pos.x;
	    float ddy = data->y - cell.pos.y;
	    float dd = std::sqrt(ddx*ddx + ddy*ddy);
	    data->dx = ddx / dd;
	    data->dy = ddy / dd;
	 } else if(cell.info.t == CInfo::Fast) {
	    data->dx += dxx*0.02;
	    data->dy += dyy*0.02;
	 } else if(cell.info.t == CInfo::Slow) {
	    data->dx *= 0.8;
	    data->dy *= 0.8;
	 }
	 
         if(data->trail.size() == 0) {
	    data->trail.push_back(data->x);
            data->trail.push_back(data->y);
	 }

	 int ts = data->trail.size();
	 float tdx = data->trail[ts-2] - data->x;
	 float tdy = data->trail[ts-1] - data->y;
         float td2 = tdx*tdx + tdy*tdy;
	 if(td2 > 100) {
	    data->trail.push_back(data->x);
            data->trail.push_back(data->y);
	 }
	 
         float tx = data->trail[0];
         float ty = data->trail[1];
	 int ii = 2;
	 while(ii<data->trail.size()) {
	    float ttx = data->trail[ii];
	    float tty = data->trail[ii+1];
	    DrawLine(tx,ty,ttx,tty,target, c);
	    tx = ttx;
	    ty = tty;
	    ii+=2;
	 }

         DrawRect(data->x-3,data->y-3, 6,6, target, evp::Color(0,0,0));
         DrawRect(data->x-2 + 5*dxx,data->y-2 + 5*dyy, 4,4, target, evp::Color(0,0,0));
         DrawRect(data->x-2,data->y-2, 4,4, target, c);
         DrawRect(data->x-1 + 5*dxx,data->y-1 + 5*dyy, 2,2, target, c);

         DrawRect(300,580-20*ui, 200, 15, target, evp::Color(0,0,0,0.6));
	 DrawText(300,580-20*ui, user->name() + " " + std::to_string(data->score),
	          15, target, c);
      };
      visitUsers(f);
   }
   virtual evp::UserData* newUserData(const evp::User* u) { return new VoronoiUserData();}
   virtual void onActivate() {
      std::cout << "VoronoiActivate " << name() << std::endl;
   }
   virtual void onActivateUser(evp::User* const u,evp::UserData* const raw) {
      VoronoiUserData* data = dynamic_cast<VoronoiUserData*>(raw);
      std::cout << "VoronoiActivateUser " << name() << " " << u->name() << std::endl;
   
      data->color = nextColor();
      u->clearControls();
      u->registerControl(new evp::BGColorControl(u->nextControlId(),data->color));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.55,0.4,0.4,"KeyA",
			      [data](bool down) {
			         data->left = down;
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.55,0.55,0.4,0.4,"KeyD",
			      [data](bool down) {
				 data->right = down;
			      }));
//      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.55,0.4,0.4,"Space",
//			      [this](bool down) {
//				 if(down) {setupMap();}
//			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.55,0.05,0.4,0.4,"Escape",
			      [this](bool down) {
				 if(down) {this->server()->setActive("lobby");}
			      }));
   }
   void setWinner(evp::User* w) {winner_ = w;}
private:
   evp::Color nextColor() {
      int i = colorCnt++;
      return evp::Color::hue((i*5 % 17) / 17.0);
   }
   int colorCnt = 1;
   evp::VoronoiMap<CInfo> *vmap;
   int seed_;
   float dx_;
   float dy_;
   evp::User* winner_=NULL;
};



#endif //VORONOI_ROOM_HPP

