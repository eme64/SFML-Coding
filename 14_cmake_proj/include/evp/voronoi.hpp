#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <cmath>
#include <cassert>
#include <string>

#include "evp/draw.hpp"



#ifndef EVP_VORONOI_HPP
#define EVP_VORONOI_HPP


//https://github.com/JCash/voronoi
#define JC_VORONOI_IMPLEMENTATION
// If you wish to use doubles
//#define JCV_REAL_TYPE double
//#define JCV_FABS fabs
//#define JCV_ATAN2 atan2
#include "jc_voronoi/jc_voronoi.h"


namespace evp {

// --------------------------------------------------------- HELPER FUNCTIONS
void voronoi_generate(int numpoints, const jcv_point* points, jcv_diagram &diagram, _jcv_rect* rect)
{
  memset(&diagram, 0, sizeof(jcv_diagram));
  jcv_diagram_generate(numpoints, points, rect, &diagram);
}

void voronoi_relax_points(const jcv_diagram &diagram, jcv_point* points, _jcv_rect* rect,
		          const std::vector<bool> &pointsFix)
{
  const jcv_site* sites = jcv_diagram_get_sites(&diagram);
  for( int i = 0; i < diagram.numsites; ++i )
  {
    const jcv_site* site = &sites[i];
    if(pointsFix[site->index]) {continue;}
    jcv_point sum = site->p;
    int count = 1;

    const jcv_graphedge* edge = site->edges;

    while( edge )
    {
  		sum.x += edge->pos[0].x;
  		sum.y += edge->pos[0].y;
  		++count;
  		edge = edge->next;
    }

    points[site->index].x = sum.x / count;
    points[site->index].y = sum.y / count;

    if(points[site->index].x < rect->min.x){points[site->index].x = rect->min.x;}
    if(points[site->index].y < rect->min.y){points[site->index].y = rect->min.y;}

    if(points[site->index].x > rect->max.x){points[site->index].x = rect->max.x;}
    if(points[site->index].y > rect->max.y){points[site->index].y = rect->max.y;}
  }
}

//void voronoi_draw_edges(const jcv_diagram &diagram, sf::RenderWindow &window)
//{
//  // If all you need are the edges
//  const jcv_edge* edge = jcv_diagram_get_edges( &diagram );
//  while( edge )
//  {
//    DrawLine(edge->pos[0].x ,edge->pos[0].y , edge->pos[1].x, edge->pos[1].y, window);
//    edge = edge->next;
//  }
//}
//
//
//void voronoi_draw_cells(const jcv_diagram &diagram, sf::RenderWindow &window)
//{
//  // If you want to draw triangles, or relax the diagram,
//  // you can iterate over the sites and get all edges easily
//  const jcv_site* sites = jcv_diagram_get_sites( &diagram );
//  for( int i = 0; i < diagram.numsites; ++i )
//  {
//    const jcv_site* site = &sites[i];
//    const jcv_graphedge* e = site->edges;
//    while( e )
//    {
//      sf::ConvexShape convex;
//      convex.setPointCount(3);
//      convex.setPoint(0, sf::Vector2f(site->p.x, site->p.y));
//      convex.setPoint(1, sf::Vector2f(e->pos[0].x, e->pos[0].y));
//      convex.setPoint(2, sf::Vector2f(e->pos[1].x, e->pos[1].y));
//
//      convex.setFillColor(sf::Color((i*7) % 256,(i*31) % 256,(i*71) % 256));
//      window.draw(convex, sf::BlendAdd);
//
//      e = e->next;
//    }
//  }
//}


void voronoi_free(jcv_diagram &diagram)
{
  jcv_diagram_free( &diagram );
}

// ---------------------------------------------------------- USEFUL OBJECTS

// Predefined structures / points:
// set points + exclusion zone for random points
// points / cells refer back to object, call its functions?
//
// allow generation of structured cell maps - no voronoi ?

struct Point {
   float x,y;
   Point(float x=0, float y=0) : x(x),y(y) {}
};

struct CellMapSubstructure {
   std::vector<Point> points;
   virtual bool isInside(float x, float y) {return false;}
};

struct CellMapSubstructureGrid : public CellMapSubstructure {
   float x0,y0,x1,y1;
   CellMapSubstructureGrid(int nx, int ny, float x0, float y0, float x1, float y1) 
   : x0(x0),y0(y0),x1(x1),y1(y1) {
      float dx = x1-x0; float dy = y1-y0;
      points.reserve(nx*ny);
      for(int xx=0; xx<nx; xx++) {
	 float x = x0 + dx*(xx+0.5)/nx;
         for(int yy=0; yy<ny; yy++) {
	    float y = y0 + dy*(yy+0.5)/ny;
	    points.push_back(Point(x,y));
	 }
      }
   }
   virtual bool isInside(float x, float y) {
      return x>=x0 and x<x1 and y>=y0 and y<y1;
   }
};

struct CellMapSubstructureAntiCircle : public CellMapSubstructure {
   float x0,y0,r0,r1;
   CellMapSubstructureAntiCircle(int n, float x0, float y0, float r0, float r1) 
   : x0(x0),y0(y0),r0(r0),r1(r1) {
      points.reserve(n);
      for(int i=0; i<n; i++) {
	 const float x = x0 + r1*std::cos(2*M_PI*i/((float) n));
	 const float y = y0 + r1*std::sin(2*M_PI*i/((float) n));
	 points.push_back(Point(x,y));
      }
   }
   virtual bool isInside(float x, float y) {
      const float dx = x0-x;
      const float dy = y0-y;
      const float d2 = dx*dx + dy*dy;
      return d2 > r0*r0;
   }
};



template <typename CInfo>
struct CellMapCell
{
  Point pos;
  sf::Color color;
  int sub = -1;
  int subI = -1;

  std::vector<Point> corners;
  std::vector<int> neighbors;

  CInfo info;
};

template <typename CInfo>
struct CellMap
{
  /*
   --- this is basically a graph
   V : the center of each polygon/cell (contains cell and edges)
   E : connections between cells
  */

  const float x0,y0,x1,y1;
  size_t numCells;
  std::vector<CellMapCell<CInfo>> cells;
  std::vector<CellMapSubstructure*> subs;

  std::vector<sf::Vertex> mesh;

  CellMap(float x0, float y0, float x1, float y1,
          const std::vector<CellMapSubstructure*> &subs,
	  int nAddPoints)
  : x0(x0),y0(y0),x1(x1),y1(y1), subs(subs)
  {
    // --------------------------- Prepare Points
    numCells = nAddPoints;
    for(CellMapSubstructure* s : subs) {
       numCells += s->points.size();
    }
    jcv_point* points = NULL;
    points = (jcv_point*)malloc( sizeof(jcv_point) * (size_t)numCells);
    std::vector<bool> pointsFix(numCells,false);
    // --------------------------- Fill in Points from subs
    int pIndex=0;
    for(CellMapSubstructure* s : subs) {
       for(const Point& p : s->points) {
          int i = pIndex++;
	  points[i].x = p.x;
          points[i].y = p.y;
	  pointsFix[i] = true;
       }
    }
    
    // --------------------------- Fill in Additional Points
    while(nAddPoints>0) {
       float x = x0 + (x1-x0)*((float) rand() / (RAND_MAX));
       float y = y0 + (y1-y0)*((float) rand() / (RAND_MAX));
       bool hit = false;
       for(CellMapSubstructure* s : subs) {
          if(s->isInside(x,y)) {hit=true; break;}
       }
       if(!hit) {
          nAddPoints--;
	  int i = pIndex++;
	  points[i].x = x;
          points[i].y = y;
       }
    }

    // --------------------------- Prepare Diagram
    jcv_diagram diagram;
    _jcv_rect rect;
    rect.min.x = x0;
    rect.min.y = y0;
    rect.max.x = x1;
    rect.max.y = y1;
    voronoi_generate(numCells, points, diagram, &rect);

    for(int i=0; i<10; i++) // relax some -> make more equispaced
    {
      voronoi_relax_points(diagram, points, &rect, pointsFix);
      voronoi_free(diagram);
      voronoi_generate(numCells, points, diagram, &rect);
    }

    // ----------------  extract graph
    cells.resize(numCells);
    
    int ppIndex=0;
    for(int si=0; si<subs.size(); si++) {
       int pi=0;
       for(const Point& p : subs[si]->points) {
          int i = ppIndex++;
	  cells[i].sub = si;
	  cells[i].subI = pi;
       }
    }
	

    for(size_t i = 0; i<numCells; i++)
    {
      cells[i].pos.x = points[i].x;
      cells[i].pos.y = points[i].y;
      cells[i].color = Color(
		      (11*i % 256) / 256.0,
		      (13*i % 256) / 256.0,
		      (17*i % 256) / 256.0 ).toSFML();
    }

    // get neighbours and edges
    const jcv_site* sites = jcv_diagram_get_sites( &diagram );
    for( int i = 0; i < diagram.numsites; ++i )
    {
      const jcv_site* site = &sites[i];

      int index = site->index;

      const jcv_graphedge* e = site->edges;
      while( e )
      {
        cells[index].corners.push_back({e->pos[0].x, e->pos[0].y});
        if(e->neighbor && e->neighbor->index >= 0 && e->neighbor->index < numCells){
          cells[index].neighbors.push_back(e->neighbor->index);
        }

        e = e->next;
      }
    }

    voronoi_free(diagram);

    create_mesh();
  }
  void create_mesh()
  {
    mesh.clear();

    for(size_t i = 0; i<numCells; i++)
    {
      int nc = cells[i].corners.size();

      for(int c = 2; c<nc; c++){
        mesh.push_back(sf::Vertex(
                          sf::Vector2f(cells[i].corners[0].x,cells[i].corners[0].y),
                          cells[i].color));

        mesh.push_back(sf::Vertex(
                          sf::Vector2f(cells[i].corners[c-1].x,cells[i].corners[c-1].y),
                          cells[i].color));

        mesh.push_back(sf::Vertex(
                          sf::Vector2f(cells[i].corners[c].x,cells[i].corners[c].y),
                          cells[i].color));
      }
    }
  }

  size_t getCell(float x, float y, size_t guess_cell)
  {
    size_t number_iterations = 0;
    // supposed to speed up, because could be close:
    size_t res = guess_cell;

    float dx = cells[guess_cell].pos.x - x;
    float dy = cells[guess_cell].pos.y - y;
    float squaredist = dx*dx + dy*dy; // from initial guess

    bool done = false;
    while(!done)
    {
      done = true;

      // check neighbors of current guess, find better fit
      size_t nc = cells[res].neighbors.size();

      for(int i=0; i<nc; i++)
      {
        size_t n = cells[res].neighbors[i];
        dx = cells[n].pos.x - x;
        dy = cells[n].pos.y - y;

        float d = dx*dx + dy*dy;

        if(d < squaredist)
        {
          res = n;
          squaredist = d;
          done = false;
          number_iterations++;
          break; // needed to recompute nc !
        }
      }
    }
    return res;
  }

  //size_t getCell(float x, float y)
  //{
  //  size_t res = 0;
  //  float squaredist = spread_x*spread_x + spread_y*spread_y; // max dist on map !

  //  for(size_t i = 0; i<num_cells; i++)
  //  {
  //    float dx = cells[i].pos.x - x;
  //    float dy = cells[i].pos.y - y;

  //    float d = dx*dx + dy*dy;

  //    if(d < squaredist)
  //    {
  //      res = i;
  //      squaredist = d;
  //    }
  //  }

  //  return res;
  //}

  //void bfs(size_t root, std::vector<size_t> &parents)
  //{
  //  if(! cells[root].isLand)
  //  {
  //    // root is not reachable! -> make that clear in parents!
  //    parents[root] = root;

  //    size_t nn = cells[root].neighbors.size();
  //    for(size_t nec = 0; nec<nn; nec++)
  //    {
  //      size_t ne = cells[root].neighbors[nec];
  //      parents[ne] = ne;
  //    }
  //    return; // early abort
  //  }

  //  // creates a rooted tree from a bfs traversal
  //  bool marked[num_cells] = {false};
  //  std::queue<size_t> q;

  //  parents[root] = root;
  //  marked[root] = true;
  //  q.push(root);

  //  while(!q.empty())
  //  {
  //    size_t c = q.front();
  //    q.pop();

  //    marked[c] = true;

  //    //push all children
  //    size_t nn = cells[c].neighbors.size();
  //    for(size_t nec = 0; nec<nn; nec++)
  //    {
  //      size_t ne = cells[c].neighbors[nec];
  //      if(!marked[ne] && cells[ne].isLand)
  //      {
  //        parents[ne] = c;
  //        q.push(ne);

  //        //if(((float) rand() / (RAND_MAX)) < 0.9){ // makes it inprecize, helps them find different paths
  //          marked[ne] = true;
  //        //}
  //      }
  //    }
  //  }
  //}

  void draw(float x, float y, float zoom, float w, sf::RenderTarget &target)
  {
    sf::Transform t;
    t.translate(x, y);
    t.scale(zoom, zoom);
    t.rotate(w*180.0f/M_PI);

    target.draw(&mesh[0], mesh.size(), sf::Triangles, t);
  }
};




template <typename CInfo>
struct VoronoiMapCell
{
  Point pos;
  sf::Color color;

  std::vector<Point> corners;
  std::vector<size_t> neighbors;

  CInfo info;
};

template <typename CInfo>
struct VoronoiMap
{
  /*
   --- this is basically a graph
   V : the center of each polygon/cell (contains cell and edges)
   E : connections between cells
  */

  float spread_x; // more or less ...
  float spread_y;
  size_t num_cells;
  std::vector<VoronoiMapCell<CInfo>> cells;

  std::vector<sf::Vertex> mesh;

  VoronoiMap(size_t n_cells, float s_x, float s_y)
  {
    num_cells = n_cells;
    spread_x = s_x;
    spread_y = s_y;

    // --------------------------- Prepare Points
    jcv_point* points = NULL;
    points = (jcv_point*)malloc( sizeof(jcv_point) * (size_t)num_cells);
    std::vector<bool> pointsFix(num_cells,false);

    for(size_t i = 0; i<num_cells; i++)
    {
      points[i].x = spread_x*((float) rand() / (RAND_MAX));
      points[i].y = spread_y*((float) rand() / (RAND_MAX));
    }

    // --------------------------- Prepare Diagram
    jcv_diagram diagram;
    _jcv_rect rect;
    rect.min.x = 0;
    rect.min.y = 0;
    rect.max.x = spread_x;
    rect.max.y = spread_y;
    voronoi_generate(num_cells, points, diagram, &rect);

    for(int i=0; i<10; i++) // relax some -> make more equispaced
    {
      voronoi_relax_points(diagram, points, &rect, pointsFix);
      voronoi_free(diagram);
      voronoi_generate(num_cells, points, diagram, &rect);
    }

    // ----------------  extract graph
    cells.resize(num_cells);

    for(size_t i = 0; i<num_cells; i++)
    {
      cells[i].pos.x = points[i].x;
      cells[i].pos.y = points[i].y;
      cells[i].color = Color(
		      (11*i % 256) / 256.0,
		      (13*i % 256) / 256.0,
		      (17*i % 256) / 256.0 ).toSFML();
    }

    // get neighbours and edges
    const jcv_site* sites = jcv_diagram_get_sites( &diagram );
    for( int i = 0; i < diagram.numsites; ++i )
    {
      const jcv_site* site = &sites[i];

      int index = site->index;

      const jcv_graphedge* e = site->edges;
      while( e )
      {
        cells[index].corners.push_back({e->pos[0].x, e->pos[0].y});
        if(e->neighbor && e->neighbor->index >= 0 && e->neighbor->index < num_cells){
          cells[index].neighbors.push_back(e->neighbor->index);
        }

        e = e->next;
      }
    }

    voronoi_free(diagram);

    create_mesh();
  }
  void create_mesh()
  {
    mesh.clear();

    for(size_t i = 0; i<num_cells; i++)
    {
      int nc = cells[i].corners.size();

      for(int c = 2; c<nc; c++){
        mesh.push_back(sf::Vertex(
                          sf::Vector2f(cells[i].corners[0].x,cells[i].corners[0].y),
                          cells[i].color));

        mesh.push_back(sf::Vertex(
                          sf::Vector2f(cells[i].corners[c-1].x,cells[i].corners[c-1].y),
                          cells[i].color));

        mesh.push_back(sf::Vertex(
                          sf::Vector2f(cells[i].corners[c].x,cells[i].corners[c].y),
                          cells[i].color));
      }
    }
  }

  size_t getCell(float x, float y, size_t guess_cell)
  {
    size_t number_iterations = 0;
    // supposed to speed up, because could be close:
    size_t res = guess_cell;

    float dx = cells[guess_cell].pos.x - x;
    float dy = cells[guess_cell].pos.y - y;
    float squaredist = dx*dx + dy*dy; // from initial guess

    bool done = false;
    while(!done)
    {
      done = true;

      // check neighbors of current guess, find better fit
      size_t nc = cells[res].neighbors.size();

      for(int i=0; i<nc; i++)
      {
        size_t n = cells[res].neighbors[i];
        dx = cells[n].pos.x - x;
        dy = cells[n].pos.y - y;

        float d = dx*dx + dy*dy;

        if(d < squaredist)
        {
          res = n;
          squaredist = d;
          done = false;
          number_iterations++;
          break; // needed to recompute nc !
        }
      }
    }
    return res;
  }

  //size_t getCell(float x, float y)
  //{
  //  size_t res = 0;
  //  float squaredist = spread_x*spread_x + spread_y*spread_y; // max dist on map !

  //  for(size_t i = 0; i<num_cells; i++)
  //  {
  //    float dx = cells[i].pos.x - x;
  //    float dy = cells[i].pos.y - y;

  //    float d = dx*dx + dy*dy;

  //    if(d < squaredist)
  //    {
  //      res = i;
  //      squaredist = d;
  //    }
  //  }

  //  return res;
  //}

  //void bfs(size_t root, std::vector<size_t> &parents)
  //{
  //  if(! cells[root].isLand)
  //  {
  //    // root is not reachable! -> make that clear in parents!
  //    parents[root] = root;

  //    size_t nn = cells[root].neighbors.size();
  //    for(size_t nec = 0; nec<nn; nec++)
  //    {
  //      size_t ne = cells[root].neighbors[nec];
  //      parents[ne] = ne;
  //    }
  //    return; // early abort
  //  }

  //  // creates a rooted tree from a bfs traversal
  //  bool marked[num_cells] = {false};
  //  std::queue<size_t> q;

  //  parents[root] = root;
  //  marked[root] = true;
  //  q.push(root);

  //  while(!q.empty())
  //  {
  //    size_t c = q.front();
  //    q.pop();

  //    marked[c] = true;

  //    //push all children
  //    size_t nn = cells[c].neighbors.size();
  //    for(size_t nec = 0; nec<nn; nec++)
  //    {
  //      size_t ne = cells[c].neighbors[nec];
  //      if(!marked[ne] && cells[ne].isLand)
  //      {
  //        parents[ne] = c;
  //        q.push(ne);

  //        //if(((float) rand() / (RAND_MAX)) < 0.9){ // makes it inprecize, helps them find different paths
  //          marked[ne] = true;
  //        //}
  //      }
  //    }
  //  }
  //}

  void draw(float x, float y, float zoom, sf::RenderTarget &target)
  {
    sf::Transform t;
    t.scale(zoom, zoom);
    t.translate(x, y);

    target.draw(&mesh[0], mesh.size(), sf::Triangles, t);
  }
};


} // namespace evp

#endif //EVP_VORONOI_HPP

