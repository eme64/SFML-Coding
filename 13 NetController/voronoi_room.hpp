#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <cmath>
#include <cassert>
#include <string>

#include "evp_server.hpp"
#include "evp_draw.hpp"
#include "evp_voronoi.hpp"


#include "FastNoise/FastNoise.h"
//https://github.com/Auburns/FastNoise/wiki


#ifndef VORONOI_ROOM_HPP
#define VORONOI_ROOM_HPP

class VoronoiUserData : public evp::UserData {
public:
   float x=20,y=300;
   float dx=0,dy=0;
   float w=0;
   bool left,right;
private:
};


struct DetailNoise {
   std::vector<FastNoise> noiseGen;
   DetailNoise(int &seed_, float baseFreq = 0.007, int depth=5) {
      noiseGen.resize(depth);
      for(int i=0; i<noiseGen.size(); i++) {
	 noiseGen[i].SetNoiseType(FastNoise::SimplexFractal);
	 noiseGen[i].SetSeed(seed_++);
	 noiseGen[i].SetFrequency(baseFreq*std::pow(2.0,i));
      }
   }
   inline float get(float x,float y) {
      float h0 = 0;
      for(int i=0; i<noiseGen.size(); i++) {
         h0 += (1.0+noiseGen[i].GetNoise(x,y))*0.5*std::pow(0.5,i);
      }
      return h0 * 0.5;  
   }
};

struct CInfo {
   enum Type {Normal, Blocked, Fast, Slow, Goal};
   Type t;
};

class VoronoiRoom : public evp::Room {
public:
   VoronoiRoom(const std::string &name, evp::RoomServer* server,float dx, float dy)
   : evp::Room(name,server), dx_(dx), dy_(dy) {
      vmap = new evp::VoronoiMap<CInfo>(3000, dx_-10,dy_-10);
      
      seed_=0;
      setupMap();
      setupUsers();
   }
   
   void setupMap() {
      DetailNoise n0(seed_);
      DetailNoise n1(seed_);
      DetailNoise n2(seed_);
         
      for(int i=0; i<vmap->num_cells; i++) {
	 float x = vmap->cells[i].pos.x;
	 float y = vmap->cells[i].pos.y;
	 float h0 = n0.get(x,y)*(x<100?x*0.01:1.0)*((x>dx_-100)?(1.0-(x-dx_+100)*0.01):1.0);
	 float h1 = n1.get(x,y);
	 float h2 = n2.get(x,y)*(x<100?x*0.01:1.0)*((x>dx_-100)?(1.0-(x-dx_+100)*0.01):1.0);
         
	 Color c(h0,h0,h0);
	 if(h0 > 0.57) {
	    vmap->cells[i].info.t = CInfo::Blocked;
	 } else {
            if(h2 > 0.55) {
	       vmap->cells[i].info.t = CInfo::Slow;
	       c = Color((1-h1)*0.7,h0*0.3,h0*0.3);
	    } else if(h1>0.58) {
	       vmap->cells[i].info.t = CInfo::Fast;
	       c = Color(h0*0.5,h1,h0*0.5);
	       c = Color(h0*0.3,h1*0.7,h0*0.3);
	    } else {
	       vmap->cells[i].info.t = CInfo::Normal;
	       c = Color(h0*0.2,h0*0.2,h0*0.2);
	    }
	 }
	 vmap->cells[i].color = c.toSFML();
      }

      auto &goal = vmap->cells[vmap->getCell(dx_-50,dy_*0.5,0)];
      goal.info.t = CInfo::Goal;
      goal.color = Color(1,1,0).toSFML();

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
         
	 Color c(0,1,0);
	 if(cell.info.t == CInfo::Goal) {
	    setWinner(user);
	 } else if(cell.info.t == CInfo::Blocked) {
	    c = Color(1,0,0);
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
         DrawRect(data->x-3,data->y-3, 6,6, target, Color(0,0,0));
         DrawRect(data->x-2 + 5*dxx,data->y-2 + 5*dyy, 4,4, target, Color(0,0,0));
         DrawRect(data->x-2,data->y-2, 4,4, target, c);
         DrawRect(data->x-1 + 5*dxx,data->y-1 + 5*dyy, 2,2, target, c);
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
   
      u->clearControls();
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.05,0.4,0.4,"KeyA",
			      [data](bool down) {
			         data->left = down;
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.55,0.05,0.4,0.4,"KeyD",
			      [data](bool down) {
				 data->right = down;
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.05,0.55,0.4,0.4,"Space",
			      [this](bool down) {
				 if(down) {setupMap();}
			      }));
      u->registerControl(new evp::ButtonControl(u->nextControlId(),0.55,0.55,0.9,0.4,"Escape",
			      [this](bool down) {
				 if(down) {this->server()->setActive("lobby");}
			      }));
   }
   void setWinner(evp::User* w) {winner_ = w;}
private:
   evp::VoronoiMap<CInfo> *vmap;
   int seed_;
   float dx_;
   float dy_;
   evp::User* winner_=NULL;
};



#endif //VORONOI_ROOM_HPP

