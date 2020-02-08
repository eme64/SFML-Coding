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


#ifndef VORONOI_ROOM_HPP
#define VORONOI_ROOM_HPP

class VoronoiUserData : public evp::UserData {
public:
   float x=100,y=100;
   bool down = false;
private:
};

typedef int CInfo;

class VoronoiRoom : public evp::Room {
public:
   VoronoiRoom(const std::string &name, evp::RoomServer* server)
   : evp::Room(name,server) {
      vmap = new evp::VoronoiMap<CInfo>(3000, 1000,1000);
      for(int i=0; i<vmap->num_cells; i++) {
	 float x = vmap->cells[i].pos.x;
	 float y = vmap->cells[i].pos.y;
         vmap->cells[i].color = Color(
			 ((int)x % 256)/256.0,
			 ((int)y % 256)/256.0,
			 0 ).toSFML();
	 vmap->cells[i].info = 123;
      }
      vmap->create_mesh();
   }
   
   ~VoronoiRoom() {
      delete vmap;
   }

   virtual void draw(sf::RenderTarget &target) {
      vmap->draw(0,0,1,target);
      evp::Room::UserVisitF f = [&] (evp::User* user, evp::UserData* raw) {
         VoronoiUserData* data = dynamic_cast<VoronoiUserData*>(raw);
	 if(data->down) {
            DrawRect(data->x,data->y, 5,5, target, Color(0,1,0));
	 } else {
            DrawRect(data->x,data->y, 5,5, target, Color(1,0,0));
	 }
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
   evp::VoronoiMap<CInfo> *vmap;
};



#endif //VORONOI_ROOM_HPP

