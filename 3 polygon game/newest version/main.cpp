#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <SFML/Graphics.hpp>
#include <list>
#include <queue>

#include "FastNoise/FastNoise.h"
//https://github.com/Auburns/FastNoise/wiki

//https://github.com/JCash/voronoi
#define JC_VORONOI_IMPLEMENTATION
// If you wish to use doubles
//#define JCV_REAL_TYPE double
//#define JCV_FABS fabs
//#define JCV_ATAN2 atan2
#include "jc_voronoi.h"

/*
------------------------------- TODO

- make objects collide
- give objects a player, player belongs to a group
- allow objects to navigate to a coordinate within a cell
- object selection / commands

- networking
*/

//----------------------------------- STATIC VARIABLES
float SCREEN_SIZE_X = 1600;
float SCREEN_SIZE_Y = 1000;

float MOUSE_X;
float MOUSE_Y;

sf::Font FONT;

// --------- MOUSE
bool MOUSE_LEFT_HIT = false;
bool MOUSE_LEFT_DOWN = false;

bool MOUSE_RIGHT_HIT = false;
bool MOUSE_RIGHT_DOWN = false;


// --------------------------------------------------------- DRAWING
void DrawDot(float x, float y, sf::RenderWindow &window, sf::Color color = sf::Color(255,0,0))
{
  float r = 3;
  sf::CircleShape shape(r);
  shape.setFillColor(color);
  shape.setPosition(x-r, y-r);
  window.draw(shape, sf::BlendAdd);
}

void DrawLine(float x1, float y1, float x2, float y2, sf::RenderWindow &window)
{
  sf::Vertex line[] =
  {
    sf::Vertex(sf::Vector2f(x1,y1), sf::Color(0,0,0)),
    sf::Vertex(sf::Vector2f(x2,y2), sf::Color(255,255,255))
  };
  window.draw(line, 2, sf::Lines, sf::BlendAdd);
}

void DrawText(float x, float y, std::string text, float size, sf::RenderWindow &window, sf::Color color = sf::Color(255,255,255))
{
  sf::Text shape(text, FONT);//(text, FONT);
  //shape.setFont(FONT);
  //shape.setString(text); //std::to_string(input[i])
  shape.setCharacterSize(size);
  shape.setColor(color);
  shape.setPosition(x,y);
  //shape.setStyle(sf::Text::Bold | sf::Text::Underlined);
  window.draw(shape, sf::BlendAdd);
}

// --------------------------------------------------------- VORONOI
void voronoi_generate(int numpoints, const jcv_point* points, jcv_diagram &diagram, _jcv_rect* rect)
{
  memset(&diagram, 0, sizeof(jcv_diagram));
  jcv_diagram_generate(numpoints, points, rect, &diagram);
}

void voronoi_relax_points(const jcv_diagram &diagram, jcv_point* points, _jcv_rect* rect)
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

    if(points[site->index].x < rect->min.x){points[site->index].x = rect->min.x;}
    if(points[site->index].y < rect->min.y){points[site->index].y = rect->min.y;}

    if(points[site->index].x > rect->max.x){points[site->index].x = rect->max.x;}
    if(points[site->index].y > rect->max.y){points[site->index].y = rect->max.y;}
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

  /* --- game properies:
  land -> land, air
  water -> water, air
  solid -> air
  */

  bool isLand;
  bool isWater;
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

  std::vector<sf::Vertex> mesh;

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
    _jcv_rect rect;
    rect.min.x = 0;
    rect.min.y = 0;
    rect.max.x = spread_x;
    rect.max.y = spread_y;
    voronoi_generate(num_cells, points, diagram, &rect);

    for(int i=0; i<10; i++) // relax some -> make more equispaced
    {
      voronoi_relax_points(diagram, points, &rect);
      voronoi_free(diagram);
      voronoi_generate(num_cells, points, diagram, &rect);
    }

    // ----------------  extract graph
    cells.resize(num_cells);

    // ---------------------   generation settings:
    size_t generation_seed = 1000;
    float water_frac = 0.4; // 0 - 1
    float hight_noise = 0.5; // 0 - 1

    // -------------------- hight function
    FastNoise noiseHight;
    noiseHight.SetNoiseType(FastNoise::SimplexFractal);
		noiseHight.SetSeed(generation_seed + 1);
		noiseHight.SetFrequency(0.01*hight_noise + 0.0005);

    auto height = [&noiseHight, this](float x, float y)
    {
      float border_proximity_x = std::min(x, spread_x-x);
      float border_proximity_y = std::min(y, spread_y-y);
      float border_proximity = std::min(border_proximity_x, border_proximity_y);

      float res = (1.0+noiseHight.GetNoise(x,y))*0.5;

      float border = 100;
      if(border > border_proximity)
      {
        res *= border_proximity/border;
      }

      return res;
    };

    // --------------------- water color function
    FastNoise noiseWaterColor1;
    noiseWaterColor1.SetNoiseType(FastNoise::SimplexFractal);
		noiseWaterColor1.SetSeed(generation_seed + 2);
		noiseWaterColor1.SetFrequency(0.01);

    FastNoise noiseWaterColor2;
    noiseWaterColor2.SetNoiseType(FastNoise::SimplexFractal);
		noiseWaterColor2.SetSeed(generation_seed + 3);
		noiseWaterColor2.SetFrequency(0.01);

    auto waterColor = [&noiseWaterColor1, &noiseWaterColor2, &water_frac](float x, float y, float height)
    {
      float water_depth_ratio = height / water_frac;
      float light = water_depth_ratio + 0.4*(1.0+noiseWaterColor1.GetNoise(x,y))*0.5;
      float color = (1.0+noiseWaterColor2.GetNoise(x,y))*0.5;
      float c_inv = 1.0 - color;
      float r = light * 40;
      float g = light * (80 * color + 120 * c_inv);
      float b = light * (200 * color + 160 * c_inv);

      return sf::Color(r,g,b);
    };

    // --------------------- sand color function
    FastNoise noiseSandColor1;
    noiseSandColor1.SetNoiseType(FastNoise::SimplexFractal);
		noiseSandColor1.SetSeed(generation_seed + 3);
		noiseSandColor1.SetFrequency(0.01);

    FastNoise noiseSandColor2;
    noiseSandColor2.SetNoiseType(FastNoise::SimplexFractal);
		noiseSandColor2.SetSeed(generation_seed + 4);
		noiseSandColor2.SetFrequency(0.01);

    auto sandColor = [&noiseSandColor1, &noiseSandColor2](float x, float y)
    {
      float light = 1 + 0.2*noiseSandColor1.GetNoise(x,y);
      float color = (1.0+noiseSandColor2.GetNoise(x,y))*0.5;
      float c_inv = 1.0 - color;
      float r = light * (76 * color + 200 * c_inv);
      float g = light * (70 * color + 100 * c_inv);
      float b = light * (50 * color + 30 * c_inv);

      return sf::Color(r,g,b);
    };

    // --------------------- grass color function
    FastNoise noiseGrassColor1;
    noiseGrassColor1.SetNoiseType(FastNoise::SimplexFractal);
		noiseGrassColor1.SetSeed(generation_seed + 5);
		noiseGrassColor1.SetFrequency(0.03);

    FastNoise noiseGrassColor2;
    noiseGrassColor2.SetNoiseType(FastNoise::SimplexFractal);
		noiseGrassColor2.SetSeed(generation_seed + 6);
		noiseGrassColor2.SetFrequency(0.03);

    auto grassColor = [&noiseGrassColor1, &noiseGrassColor2, &water_frac](float x, float y, float height)
    {
      // fraction over water
      float fow = (height - water_frac) / (1.0 - water_frac);
      float fow_inv = 1.0 - fow;

      float light = 1 + 0.2*noiseGrassColor1.GetNoise(x,y);
      float color = (1.0+noiseGrassColor2.GetNoise(x,y))*0.5;
      float c_inv = 1.0 - color;
      float r_grass = light * (0 * color + 20 * c_inv);
      float g_grass = light * (100 * color + 80 * c_inv);
      float b_grass = light * (0 * color + 30 * c_inv);

      float r_hill = light * (50 * color + 70 * c_inv);
      float g_hill = light * (50 * color + 50 * c_inv);
      float b_hill = light * (50 * color + 30 * c_inv);


      return sf::Color(r_grass * fow_inv + r_hill * fow, g_grass * fow_inv + g_hill * fow, b_grass * fow_inv + b_hill * fow);
    };


    // get positions
    for(size_t i = 0; i<num_cells; i++)
    {
      cells[i].pos.x = points[i].x;
      cells[i].pos.y = points[i].y;

      float h = height(cells[i].pos.x, cells[i].pos.y);

      cells[i].color = sf::Color(255 * h,255 * h,255 * h);

      if(h<water_frac)
      {
        cells[i].isWater = true;
        cells[i].isLand = false;
        cells[i].color = waterColor(cells[i].pos.x, cells[i].pos.y, h);
      }
      else if(h < (water_frac + 0.05))
      {
        cells[i].isWater = false;
        cells[i].isLand = true;
        cells[i].color = sandColor(cells[i].pos.x, cells[i].pos.y);
      }
      else
      {
        cells[i].isWater = false;
        cells[i].isLand = true;
        cells[i].color = grassColor(cells[i].pos.x, cells[i].pos.y, h);
      }


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

  size_t getCell(float x, float y)
  {
    size_t res = 0;
    float squaredist = spread_x*spread_x + spread_y*spread_y; // max dist on map !

    for(size_t i = 0; i<num_cells; i++)
    {
      float dx = cells[i].pos.x - x;
      float dy = cells[i].pos.y - y;

      float d = dx*dx + dy*dy;

      if(d < squaredist)
      {
        res = i;
        squaredist = d;
      }
    }

    return res;
  }

  void bfs(size_t root, std::vector<size_t> &parents)
  {
    if(! cells[root].isLand)
    {
      // root is not reachable! -> make that clear in parents!
      parents[root] = root;

      size_t nn = cells[root].neighbors.size();
      for(size_t nec = 0; nec<nn; nec++)
      {
        size_t ne = cells[root].neighbors[nec];
        parents[ne] = ne;
      }
      return; // early abort
    }

    // creates a rooted tree from a bfs traversal
    bool marked[num_cells] = {false};
    std::queue<size_t> q;

    parents[root] = root;
    marked[root] = true;
    q.push(root);

    while(!q.empty())
    {
      size_t c = q.front();
      q.pop();

      marked[c] = true;

      //push all children
      size_t nn = cells[c].neighbors.size();
      for(size_t nec = 0; nec<nn; nec++)
      {
        size_t ne = cells[c].neighbors[nec];
        if(!marked[ne] && cells[ne].isLand)
        {
          parents[ne] = c;
          q.push(ne);

          //if(((float) rand() / (RAND_MAX)) < 0.9){ // makes it inprecize, helps them find different paths
            marked[ne] = true;
          //}
        }
      }
    }
  }

  void draw(float x, float y, float zoom, sf::RenderWindow &window)
  {
    sf::Transform t;

    t.translate(SCREEN_SIZE_X*0.5, SCREEN_SIZE_Y*0.5);
    t.scale(zoom, zoom);
    t.translate(-x, -y);

    window.draw(&mesh[0], mesh.size(), sf::Triangles, t);

    /*
    std::vector<size_t> parents(num_cells);
    bfs(0, parents);

    // draw connections
    for(size_t i = 0; i<num_cells; i++)
    {
      DrawLine(
                (cells[i].pos.x - x)*zoom+ SCREEN_SIZE_X*0.5,
                (cells[i].pos.y - y)*zoom+ SCREEN_SIZE_Y*0.5,
                (cells[parents[i]].pos.x - x)*zoom+ SCREEN_SIZE_X*0.5,
                (cells[parents[i]].pos.y - y)*zoom+ SCREEN_SIZE_Y*0.5,
                window);
    }
    */

    return;

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

      /*
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
      */

      /*
      // draw dots
      DrawDot((cells[i].pos.x - x)*zoom + SCREEN_SIZE_X*0.5, (cells[i].pos.y - y)*zoom + SCREEN_SIZE_Y*0.5, window);
      */
    }
  }
};

//-------------------------------------------------------- Obj
struct GObject
{
  float x;
  float y;

  float speed = 2.0;

  PolyMap *map;
  size_t cell;
  size_t cell_goal;
  std::vector<size_t> path;
  size_t path_it; // points to the pos in path where the cell is that needs to be reached next

  GObject(float x, float y, PolyMap *map): x(x), y(y), map(map)
  {
    // let's find the closesd cell:

    cell = map->getCell(x,y, 0);

    calculatePath(map->num_cells * ((float) rand() / (RAND_MAX)));
  }

  bool calculatePath(size_t goal, size_t STEP_BOUND = 1000)
  {
    cell_goal = goal;

    path.clear();
    path_it = 0;
    std::vector<size_t> parents(map->num_cells);
    map->bfs(cell_goal, parents);

    size_t step_counter = 0;

    size_t current = cell;
    while(current != cell_goal && step_counter<STEP_BOUND)
    {
      size_t next = parents[current];

      path.push_back(next);
      current = next;

      step_counter++;
    }

    if(! (step_counter< STEP_BOUND))
    {
      path.clear();
      path_it = 0;
      path.push_back(cell);

      cell_goal = cell;
    }

    return (step_counter< STEP_BOUND);
    /*
    std::vector<size_t> parents(num_cells);
    bfs(0, parents);

    // draw connections
    for(size_t i = 0; i<num_cells; i++)
    {
      DrawLine(
                (cells[i].pos.x - x)*zoom+ SCREEN_SIZE_X*0.5,
                (cells[i].pos.y - y)*zoom+ SCREEN_SIZE_Y*0.5,
                (cells[parents[i]].pos.x - x)*zoom+ SCREEN_SIZE_X*0.5,
                (cells[parents[i]].pos.y - y)*zoom+ SCREEN_SIZE_Y*0.5,
                window);
    }
    */
  }

  void draw(float s_x, float s_y, float zoom, sf::RenderWindow &window)
  {
    sf::Transform t;

    t.translate(SCREEN_SIZE_X*0.5, SCREEN_SIZE_Y*0.5);
    t.scale(zoom, zoom);
    t.translate(-s_x, -s_y);

    DrawDot((x - s_x)*zoom + SCREEN_SIZE_X*0.5, (y - s_y)*zoom + SCREEN_SIZE_Y*0.5, window);

    /*
    DrawLine(   (x - s_x)*zoom + SCREEN_SIZE_X*0.5, (y - s_y)*zoom + SCREEN_SIZE_Y*0.5,
                (map->cells[cell].pos.x - s_x)*zoom + SCREEN_SIZE_X*0.5, (map->cells[cell].pos.y - s_y)*zoom + SCREEN_SIZE_Y*0.5,
                window);
                */

    /*
    std::vector<sf::Vertex> line;
    // local position
    line.push_back(sf::Vertex(sf::Vector2f((x - s_x)*zoom + SCREEN_SIZE_X*0.5, (y - s_y)*zoom + SCREEN_SIZE_Y*0.5), sf::Color(255,0,0)));
    // my cell
    line.push_back(sf::Vertex(sf::Vector2f((map->cells[cell].pos.x - s_x)*zoom + SCREEN_SIZE_X*0.5, (map->cells[cell].pos.y - s_y)*zoom + SCREEN_SIZE_Y*0.5), sf::Color(255,255,0)));
    line.push_back(sf::Vertex(sf::Vector2f((map->cells[cell].pos.x - s_x)*zoom + SCREEN_SIZE_X*0.5, (map->cells[cell].pos.y - s_y)*zoom + SCREEN_SIZE_Y*0.5), sf::Color(255,255,0)));

    size_t path_size = path.size();
    for(size_t i=path_it; i<path_size; i++)
    {
      size_t c = path[i];
      line.push_back(sf::Vertex(sf::Vector2f((map->cells[c].pos.x - s_x)*zoom + SCREEN_SIZE_X*0.5, (map->cells[c].pos.y - s_y)*zoom + SCREEN_SIZE_Y*0.5), sf::Color(0,255-255*i/path_size,255*i/path_size)));
      line.push_back(sf::Vertex(sf::Vector2f((map->cells[c].pos.x - s_x)*zoom + SCREEN_SIZE_X*0.5, (map->cells[c].pos.y - s_y)*zoom + SCREEN_SIZE_Y*0.5), sf::Color(0,255-255*i/path_size,255*i/path_size)));
    }
    window.draw(&line[0], line.size(), sf::Lines, sf::BlendAlpha);
    */
  }

  void update()
  {
    // check my cell:
    size_t new_cell = map->getCell(x,y, cell);
    cell = new_cell;

    // check path:
    size_t next_cell = path[path_it];
    float dx = map->cells[next_cell].pos.x - x;
    float dy = map->cells[next_cell].pos.y - y;

    float d = std::sqrt(dx*dx + dy*dy);
    dx/= d;
    dy/= d;

    x+= dx*speed;
    y+= dy*speed;

    if(new_cell == next_cell)
    {
      path_it++;
      if(path_it >= path.size())
      {
        bool done = calculatePath(map->num_cells * ((float) rand() / (RAND_MAX)));

        if(!done)
        {
          calculatePath(new_cell);
        }
      }
    }
  }
};

//-------------------------------------------------------- Main
int main()
{
    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "first attempts", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setMouseCursorVisible(false);

    // ------------------------------------ FONT
    if (!FONT.loadFromFile("arial.ttf"))
    {
        std::cout << "font could not be loaded!" << std::endl;
        return 0;
    }

    // ------------------------------------ MAP
    PolyMap map = PolyMap(30000, 1000, 1000);
    std::list<GObject*> obj_list;

    for(int i=0; i<100; i++)
    {
      float xx = map.spread_x * ((float) rand() / (RAND_MAX));
      float yy = map.spread_y * ((float) rand() / (RAND_MAX));
      GObject* n = new GObject(xx,yy, &map);
      obj_list.push_back(n);
    }

    // ------------------------------------ SCEEN
    float screen_x = 0;
    float screen_y = 0;
    float screen_zoom = 1.0;

    // ------------------------------------ FPS
    clock_t last_stamp = clock();
    clock_t deltaTime = 1000;
    float framesPerSec = 10;
    float framesPerSec_avg = 10;
    float averageDelta = 1000;
    size_t ticker = 0;

    auto FPS_tick = [&last_stamp, &deltaTime, &framesPerSec, &framesPerSec_avg, &averageDelta, &ticker]()
    {
      clock_t newTime = clock();
      deltaTime = newTime - last_stamp;

      averageDelta = 0.01 * deltaTime + 0.99 * averageDelta;

      framesPerSec = CLOCKS_PER_SEC / (float)deltaTime;
      framesPerSec_avg = CLOCKS_PER_SEC / (float)averageDelta;

      last_stamp = newTime;
      ticker++;
    };

    // --------------------------- Render Loop
    while (window.isOpen())
    {
        MOUSE_LEFT_HIT = false;
        MOUSE_RIGHT_HIT = false;

        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
                // window closed
                case sf::Event::Closed:
                  window.close();
                  break;
                case sf::Event::MouseWheelScrolled:
                  if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
                  {
                    int delta = event.mouseWheelScroll.delta;
                    if(delta > 0)
                    {
                      screen_zoom*= 1.1;
                    }else{
                      screen_zoom*= 0.9;
                    }
                  }
                  break;
                case sf::Event::MouseButtonPressed:
                  if(event.mouseButton.button == sf::Mouse::Left)
                  {
                    MOUSE_LEFT_HIT = true;
                    MOUSE_LEFT_DOWN = true;
                  }else if(event.mouseButton.button == sf::Mouse::Right)
                  {
                    MOUSE_RIGHT_HIT = true;
                    MOUSE_RIGHT_DOWN = true;
                  }
                  break;
                case sf::Event::MouseButtonReleased:
                  if(event.mouseButton.button == sf::Mouse::Left)
                  {
                    MOUSE_LEFT_DOWN = false;
                  }else if(event.mouseButton.button == sf::Mouse::Right)
                  {
                    MOUSE_RIGHT_DOWN = false;
                  }
                  break;
                // we don't process other types of events
                default:
                  break;
            }
        }

        sf::Vector2i mousepos = sf::Mouse::getPosition(window);
        MOUSE_X = mousepos.x;
        MOUSE_Y = mousepos.y;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)){
          screen_zoom*= 1.01;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)){
          screen_zoom*= 0.99;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)){
          screen_x-=5/screen_zoom;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)){
          screen_x+=5/screen_zoom;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)){
          screen_y-=5/screen_zoom;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)){
          screen_y+=5/screen_zoom;
        }

        window.clear();

        map.draw(screen_x, screen_y, screen_zoom, window);

        for (std::list<GObject*>::iterator it = obj_list.begin(); it != obj_list.end(); it++)
        {
            (**it).update();
        }

        for (std::list<GObject*>::iterator it = obj_list.begin(); it != obj_list.end(); it++)
        {
            (**it).draw(screen_x, screen_y, screen_zoom, window);
        }

        DrawDot(MOUSE_X, MOUSE_Y, window, sf::Color(255*MOUSE_RIGHT_DOWN,255*MOUSE_LEFT_DOWN,255));

        FPS_tick();
        DrawText(5,5, "FPS: " + std::to_string(framesPerSec), 10, window, sf::Color(255,255,255));
        DrawText(5,20, "avg: " + std::to_string(framesPerSec_avg), 10, window, sf::Color(255,255,255));
        DrawText(5,35, "ticker: " + std::to_string(ticker), 10, window, sf::Color(255,255,255));

        window.display();
    }

    return 0;
}
