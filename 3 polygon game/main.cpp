#include <iostream>
#include <stdlib.h>
#include <cmath>
//#include <list>
#include <SFML/Graphics.hpp>


//https://github.com/JCash/voronoi
#define JC_VORONOI_IMPLEMENTATION
// If you wish to use doubles
//#define JCV_REAL_TYPE double
//#define JCV_FABS fabs
//#define JCV_ATAN2 atan2
#include "jc_voronoi.h"


float SCREEN_SIZE_X = 1600;
float SCREEN_SIZE_Y = 1000;


// --------------------------------------------------------- DRAWING
void DrawDot(float x, float y, sf::RenderWindow &window)
{
  float r = 3;
  sf::CircleShape shape(r);
  shape.setFillColor(sf::Color(255,0,0));
  shape.setPosition(x-r, y-r);
  window.draw(shape, sf::BlendAdd);
}

void DrawLine(float x1, float y1, float x2, float y2, sf::RenderWindow &window)
{
  sf::Vertex line[] =
  {
    sf::Vertex(sf::Vector2f(x1,y1)),
    sf::Vertex(sf::Vector2f(x2,y2))
  };
  window.draw(line, 2, sf::Lines, sf::BlendAdd);
}


// --------------------------------------------------------- VORONOI
void voronoi_generate(int numpoints, const jcv_point* points, jcv_diagram &diagram)
{
  memset(&diagram, 0, sizeof(jcv_diagram));
  jcv_diagram_generate(numpoints, points, 0, &diagram);
}

void voronoi_relax_points(const jcv_diagram &diagram, jcv_point* points)
{
  const jcv_site* sites = jcv_diagram_get_sites(&diagram);
  for( int i = 0; i < diagram.numsites; ++i )
  {
    const jcv_site* site = &sites[i];
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
  }
}

void voronoi_draw_edges(const jcv_diagram &diagram, sf::RenderWindow &window)
{
  // If all you need are the edges
  const jcv_edge* edge = jcv_diagram_get_edges( &diagram );
  while( edge )
  {
    DrawLine(edge->pos[0].x ,edge->pos[0].y , edge->pos[1].x, edge->pos[1].y, window);
    edge = edge->next;
  }
}


void voronoi_draw_cells(const jcv_diagram &diagram, sf::RenderWindow &window)
{
  // If you want to draw triangles, or relax the diagram,
  // you can iterate over the sites and get all edges easily
  const jcv_site* sites = jcv_diagram_get_sites( &diagram );
  for( int i = 0; i < diagram.numsites; ++i )
  {
    const jcv_site* site = &sites[i];
    const jcv_graphedge* e = site->edges;
    while( e )
    {
      sf::ConvexShape convex;
      convex.setPointCount(3);
      convex.setPoint(0, sf::Vector2f(site->p.x, site->p.y));
      convex.setPoint(1, sf::Vector2f(e->pos[0].x, e->pos[0].y));
      convex.setPoint(2, sf::Vector2f(e->pos[1].x, e->pos[1].y));

      convex.setFillColor(sf::Color((i*7) % 256,(i*31) % 256,(i*71) % 256));
      window.draw(convex, sf::BlendAdd);

      e = e->next;
    }
  }
}


void voronoi_free(jcv_diagram &diagram)
{
  jcv_diagram_free( &diagram );
}

//-------------------------------------------------------- Poly Map
struct _EPPos
{
  float x; float y;
  _EPPos(){
    x = 0;
    y = 0;
  }
  _EPPos(float x, float y) : x(x), y(y){}
};

struct PolyMapCell
{
  _EPPos pos;
  sf::Color color;

  std::vector<_EPPos> corners;
  std::vector<size_t> neighbors;
};

struct PolyMap
{
  /*
   --- this is basically a graph
   V : the center of each polygon/cell (contains cell and edges)
   E : connections between cells

  */

  float spread_x; // more or less ...
  float spread_y;
  size_t num_cells;
  std::vector<PolyMapCell> cells;

  PolyMap(size_t n_cells, float s_x, float s_y)
  {
    num_cells = n_cells;
    spread_x = s_x;
    spread_y = s_y;

    // --------------------------- Prepare Points
    jcv_point* points = NULL;
    points = (jcv_point*)malloc( sizeof(jcv_point) * (size_t)num_cells);

    for(size_t i = 0; i<num_cells; i++)
    {
      points[i].x = spread_x*((float) rand() / (RAND_MAX));
      points[i].y = spread_y*((float) rand() / (RAND_MAX));
    }

    // --------------------------- Prepare Diagram
    jcv_diagram diagram;
    voronoi_generate(num_cells, points, diagram);

    for(int i=0; i<100; i++) // relax some -> make more equispaced
    {
      voronoi_relax_points(diagram, points);
      voronoi_free(diagram);
      voronoi_generate(num_cells, points, diagram);
    }

    // ----------------  extract graph
    cells.resize(num_cells);

    // get positions
    for(size_t i = 0; i<num_cells; i++)
    {
      cells[i].pos.x = points[i].x;
      cells[i].pos.y = points[i].y;

      cells[i].color = sf::Color(255*cells[i].pos.x / spread_x,(i*31) % 128,255*cells[i].pos.y / spread_y);
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
  }

  void draw(float x, float y, float zoom, sf::RenderWindow &window)
  {
    for(size_t i = 0; i<num_cells; i++)
    {
      // draw cells
      sf::ConvexShape convex;
      int nc = cells[i].corners.size();

      convex.setPointCount(nc);

      for(int c = 0; c<nc; c++){
        convex.setPoint(c, sf::Vector2f(
                                (cells[i].corners[c].x - x)*zoom+ SCREEN_SIZE_X*0.5,
                                (cells[i].corners[c].y - y)*zoom+ SCREEN_SIZE_Y*0.5));
      }

      convex.setFillColor(cells[i].color);
      window.draw(convex, sf::BlendAdd);


      // draw connections
      int nn = cells[i].neighbors.size();
      for(int c = 0; c<nn; c++){
        int j = cells[i].neighbors[c];
        DrawLine(
                  (cells[i].pos.x - x)*zoom+ SCREEN_SIZE_X*0.5,
                  (cells[i].pos.y - y)*zoom+ SCREEN_SIZE_Y*0.5,
                  (cells[j].pos.x - x)*zoom+ SCREEN_SIZE_X*0.5,
                  (cells[j].pos.y - y)*zoom+ SCREEN_SIZE_Y*0.5,
                  window);
      }

      // draw dots
      DrawDot((cells[i].pos.x - x)*zoom + SCREEN_SIZE_X*0.5, (cells[i].pos.y - y)*zoom + SCREEN_SIZE_Y*0.5, window);
    }
  }
};



//-------------------------------------------------------- Main
int main()
{
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "first attempts");
    window.setVerticalSyncEnabled(true);

    PolyMap map = PolyMap(1000, 800, 600);

    float screen_x = 0;
    float screen_y = 0;
    float screen_zoom = 1.0;

    // --------------------------- Render Loop
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
                // window closed
                case sf::Event::Closed:
                    window.close();
                    break;
                // we don't process other types of events
                default:
                    break;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Add)){
          screen_zoom*= 1.01;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract)){
          screen_zoom*= 0.99;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
          screen_x-=5/screen_zoom;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
          screen_x+=5/screen_zoom;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
          screen_y-=5/screen_zoom;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
          screen_y+=5/screen_zoom;
        }

        window.clear();

        map.draw(screen_x, screen_y, screen_zoom, window);

        window.display();
    }

    return 0;
}
