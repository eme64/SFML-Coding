#include "evp/server.hpp"
#include "evp/draw.hpp"

#include "box2d/box2d.h"


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
   b2Body* body = NULL;
private:
};

class RocketRoom : public evp::Room {
public:
   RocketRoom(const std::string &name, evp::RoomServer* server)
   : evp::Room(name,server), world_(b2Vec2(0,10)) {
      //sf::ConvexShape polygon;
      //polygon.setPointCount(5);
      //polygon.setPoint(0, sf::Vector2f(1, 0));
      //polygon.setPoint(1, sf::Vector2f(0.7, 0.2));
      //polygon.setPoint(2, sf::Vector2f(-1, 0.2));
      //polygon.setPoint(3, sf::Vector2f(-1, -0.2));
      //polygon.setPoint(4, sf::Vector2f(0.7, -0.2));
      //rocket_.push_back(polygon);

      x0_=10;
      y0_=10;
      x1_=790;
      y1_=590;

      b2BodyDef groundBodyDef;
      groundBodyDef.position.Set(0,0);
      groundBody = world_.CreateBody(&groundBodyDef);
      b2Vec2 vs[5];
      vs[0].Set(1,1);
      vs[1].Set(1,59);
      vs[2].Set(79,59);
      vs[3].Set(79,1);
      vs[4].Set(1,1);
      b2ChainShape chain;
      chain.CreateChain(vs, 5);
      groundBody->CreateFixture(&chain, 0.0f);

      for(int i=0; i<100;i++) {
         b2BodyDef bodyDef;
         bodyDef.type = b2_dynamicBody;
         bodyDef.position.Set(10.0+5*(i%10), 5.0f+i*0.2);
         bodyDef.angle = M_PI*0.1*i;
         bodyDef.angularDamping = 1.0;
         b2Body* body = world_.CreateBody(&bodyDef);
         b2PolygonShape dynamicBox;
         dynamicBox.SetAsBox(1.0,1.0);
         b2FixtureDef fixtureDef;
         fixtureDef.shape = &dynamicBox;
         fixtureDef.density = 0.05f;
         fixtureDef.friction = 0.1f;
         body->CreateFixture(&fixtureDef);
      }
   }
   virtual void draw(sf::RenderTarget &target) {
      DrawRect(x0_,y0_, x1_-x0_, y1_-y0_, target, evp::Color(0.1,0.1,0.1));
      evp::Room::UserVisitF f = [&] (evp::User* user, evp::UserData* raw) {
         RocketUserData* data = dynamic_cast<RocketUserData*>(raw);
	 
	 if(data->left) {data->body->ApplyTorque(-20,true);}
	 if(data->right) {data->body->ApplyTorque(20,true);}
	 if(data->up) {
            b2Vec2 vv = data->body->GetTransform().q.GetYAxis();
            b2Vec2 pos = data->body->GetPosition();
            b2Vec2 vel = data->body->GetLinearVelocity();
	    data->body->ApplyForceToCenter(50.0*vv ,true);
	    
	    float dxx = (vel.x-20*vv.x)/60.0;
	    float dyy = (vel.y-20*vv.y)/60.0;
	    particles_.insert(new Particle(pos.x*10, pos.y*10, dxx*10, dyy*10, 0,0, 3, evp::Color(1,0.5,0.2), 60));

	 }
      };
      visitUsers(f);
      
      for(auto it = particles_.begin(); it!=particles_.end(); it++) {
         Particle *p = *it;
	 if(! p->draw(target)) {
	    delete p;
	    particles_.erase(it);
	 }
      }

      // box2d:
      world_.Step(timeStep, velocityIterations, positionIterations);
      for(b2Body* it = world_.GetBodyList(); it!=NULL; it=it->GetNext()) {
         b2Vec2 position = it->GetPosition();
         const b2Transform& t = it->GetTransform();
         DrawRect(10*position.x,10*position.y,2,2, target, evp::Color(1,1,1));
         for(b2Fixture* f = it->GetFixtureList(); f!=NULL; f=f->GetNext()) {
            b2PolygonShape* s = dynamic_cast<b2PolygonShape*>(f->GetShape());
	    if(s) {
	       for(int i=0; i<s->m_count; i++) {
                  b2Vec2 v = b2Mul(t,s->m_vertices[i]);
                  
		  DrawRect(10*(v.x),10*(v.y),2,2, target, evp::Color(0.5,0.5,0.5));
	       }
	    } else {
	       //std::cout << f->GetShape()->m_type << std::endl;
	    }
	 }
      }
      //float angle = body->GetAngle();
      //printf("%4.2f %4.2f %4.2f\n", position.x, position.y, angle);
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
      
      b2BodyDef bodyDef;
      bodyDef.type = b2_dynamicBody;
      bodyDef.position.Set(10.0f, 20.0f);
      bodyDef.angle = M_PI*0.2;
      bodyDef.bullet = true;
      bodyDef.angularDamping = 1.0;
      data->body = world_.CreateBody(&bodyDef);
      b2PolygonShape dynamicBox;
      dynamicBox.SetAsBox(0.5,0.7);
      b2FixtureDef fixtureDef;
      fixtureDef.shape = &dynamicBox;
      fixtureDef.density = 1.0f;
      fixtureDef.friction = 0.01f;
      data->body->CreateFixture(&fixtureDef);
   }
private:
   float x0_,y0_,x1_,y1_; // limits of room
   std::set<Particle*> particles_;
   //std::vector<sf::ConvexShape> rocket_;

   //void drawRocket(float x,float y, float scale, float w, sf::RenderTarget &target, const evp::Color& color) {
   //   for(sf::Shape &s : rocket_) {
   //      s.setFillColor(color.toSFML());
   //      s.setPosition(x,y);
   //      s.setScale(scale,scale);
   //      s.setRotation(w);
   //      target.draw(s, sf::BlendAlpha);
   //   }
   //}

   // box2d:
   b2World world_;
   b2Body* groundBody = NULL;

   float timeStep = 1.0f / 60.0f;
   int32 velocityIterations = 8;
   int32 positionIterations = 3;
};



#endif // ROCKET_ROOM_HPP
