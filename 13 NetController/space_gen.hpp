#include "evp_voronoi.hpp"
#include "evp_noise.hpp"

class SpaceObject {
public:
   SpaceObject() {
      //std::vector<evp::CellMapSubstructure*> subs;
      //subs.push_back(new evp::CellMapSubstructureGrid(10,10, 200,200,300,300));
      //subs.push_back(new evp::CellMapSubstructureGrid(10,10, 350,200,450,300));
      //subs.push_back(new evp::CellMapSubstructureGrid(25,5 , 200,400,450,450));
      //subs.push_back(new evp::CellMapSubstructureAntiCircle(100, 400,300, 250,270));
      //cm_ = new evp::CellMap<int>(10,10,790,590, subs, 4000);
   
      std::vector<evp::CellMapSubstructure*> subs;
      subs.push_back(new evp::CellMapSubstructureAntiCircle(200, 0,0, 0.9,0.95));
      subs.push_back(new evp::CellMapSubstructureGrid(11,11, -0.1,-0.85,0.1,-0.65));
      cm_ = new evp::CellMap<int>(-1,-1,1,1, subs, 4000);
      
      int seed = 0;
      evp::DetailNoise n0(seed,1);
      evp::DetailNoise n1(seed,1);

      for(int i=0; i<cm_->numCells; i++) {
         evp::CellMapCell<int> &c = cm_->cells[i];
	 float h0 = n0.get(c.pos.x,c.pos.y);
	 float h1 = n0.get(c.pos.x,c.pos.y);

	 const float d2 = c.pos.x*c.pos.x + c.pos.y*c.pos.y;
	 if(c.sub > 0) {
	    c.color = evp::Color(1,i%2,0.5).toSFML();
	 } else if(d2 > 0.6) {
	    c.color = evp::Color(0,0,h0).toSFML();
	 } else if(d2 < 0.3){
            float fac = 1-std::sqrt(d2);
	    fac *= fac;
	    c.color = evp::Color((h0+h1)*fac, h1*fac, 0).toSFML();
	 } else {
	    c.color = evp::Color(d2*h1,d2*h1,d2*h1).toSFML();
	 }
      }
      cm_->create_mesh();
   }
   virtual void draw(sf::RenderTarget &target, float x,float y,float s, float mx, float my) {
      const float ox = 400 + s*x;
      const float oy = 300 + s*y;
      cm_->draw(ox,oy,s,target);
      mx-=ox;mx/=s;
      my-=oy;my/=s;

      size_t ci = cm_->getCell(mx,my,0);
      int nn = cm_->cells[ci].neighbors.size();
      evp::DrawText(0,50, "Neighbors: "+std::to_string(nn), 15, target, evp::Color(1,1,1));
   }
private:
   evp::CellMap<int>* cm_;
};




