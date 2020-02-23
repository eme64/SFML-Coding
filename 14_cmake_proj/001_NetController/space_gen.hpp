#include "evp/voronoi.hpp"
#include "evp/noise.hpp"


#include "box2d/box2d.h"


#ifndef SPACE_GEN_HPP
#define SPACE_GEN_HPP

class SpaceObject {
public:
   SpaceObject(float scale = 1.0f) {
      //std::vector<evp::CellMapSubstructure*> subs;
      //subs.push_back(new evp::CellMapSubstructureGrid(10,10, 200,200,300,300));
      //subs.push_back(new evp::CellMapSubstructureGrid(10,10, 350,200,450,300));
      //subs.push_back(new evp::CellMapSubstructureGrid(25,5 , 200,400,450,450));
      //subs.push_back(new evp::CellMapSubstructureAntiCircle(100, 400,300, 250,270));
      //cm_ = new evp::CellMap<int>(10,10,790,590, subs, 4000);
   
      std::vector<evp::CellMapSubstructure*> subs;
      subs.push_back(new evp::CellMapSubstructureAntiCircle(200, 0,0, 0.9*scale,0.95*scale));
      subs.push_back(new evp::CellMapSubstructureGrid(5,5, -0.1*scale,-0.85*scale,0.1*scale,-0.65*scale));
      cm_ = new evp::CellMap<int>(-scale,-scale,scale,scale, subs, 300);
      
      int seed = 0;
      evp::DetailNoise n0(seed,1);
      evp::DetailNoise n1(seed,1);

      for(int i=0; i<cm_->numCells; i++) {
         evp::CellMapCell<int> &c = cm_->cells[i];
	 float h0 = n0.get(c.pos.x/scale,c.pos.y/scale);
	 float h1 = n0.get(c.pos.x/scale,c.pos.y/scale);

	 const float d2 = (c.pos.x*c.pos.x + c.pos.y*c.pos.y) / scale / scale;
	 const float a = atan2(c.pos.x, c.pos.y);
         
	 c.info = 0;

	 if(c.sub > 0) {
	    c.color = evp::Color(1,i%2,0.5).toSFML();
	    c.info = 1;
	 } else if(d2 > 0.4+0.4*n0.getCyclic(a,0.5)) {
	    c.color = evp::Color(0,0,h0, 0.2).toSFML();
	 } else if(d2 < 0.3){
	    c.info = 1;
            float fac = 1-std::sqrt(d2);
	    fac *= fac;
	    c.color = evp::Color((h0+h1)*fac, h1*fac, 0).toSFML();
	 } else {
	    c.info = 1;
	    c.color = evp::Color(d2*h1,d2*h1,d2*h1).toSFML();
	 }
      }
      cm_->create_mesh();
   }
   virtual void draw(sf::RenderTarget &target, float x,float y,float s, float mx, float my, float w) {
      cm_->draw(x,y,s,w,target);
      
      //const float ox = 400 + s*x;
      //const float oy = 300 + s*y;
      //cm_->draw(ox,oy,s,w,target);
      //mx-=ox;mx/=s;
      //my-=oy;my/=s;

      //size_t ci = cm_->getCell(mx,my,0);
      //int nn = cm_->cells[ci].neighbors.size();
      //evp::DrawText(0,50, "Neighbors: "+std::to_string(nn), 15, target, evp::Color(1,1,1));
   }

   virtual b2Body* makeBody(b2World* world, float x, float y, float s, float w) {
      b2BodyDef bodyDef;
      bodyDef.type = b2_dynamicBody;
      bodyDef.position.Set(x,y);
      bodyDef.angle = w;
      //bodyDef.bullet = true;
      bodyDef.angularDamping = 1.0;
      b2Body* body = world->CreateBody(&bodyDef);
      //b2PolygonShape dynamicBox;
      //dynamicBox.SetAsBox(s,s);
      //b2FixtureDef fixtureDef;
      //fixtureDef.shape = &dynamicBox;
      //fixtureDef.density = 0.1f;
      //fixtureDef.friction = 0.01f;
      //fixtureDef.restitution = 0.5f;
      //body->CreateFixture(&fixtureDef);

      for(int i=0; i<cm_->numCells; i++) {
         evp::CellMapCell<int> &c = cm_->cells[i];
	 
         b2Vec2 vertices[c.corners.size()];
	 for(int j=0; j<c.corners.size(); j++) {
	    vertices[j].Set(c.corners[j].x*s, c.corners[j].y*s);
	 }
	 if(c.corners.size() > b2_maxPolygonVertices) {
	    std::cout << "too many " << c.corners.size() << std::endl;
	 }
	 
	 if(c.info>0 and c.corners.size() >= 3 and c.corners.size() <=b2_maxPolygonVertices) {
	    b2PolygonShape poly;
	    poly.Set(vertices, c.corners.size());
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &poly;
            fixtureDef.density = 0.1f;
            fixtureDef.friction = 0.01f;
            fixtureDef.restitution = 0.5f;
            body->CreateFixture(&fixtureDef);
	 }
      }

      return body;
   }
private:
   evp::CellMap<int>* cm_;
};




#endif // SPACE_GEN_HPP
