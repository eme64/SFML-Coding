#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <SFML/Graphics.hpp>
#include <list>
#include <queue>
#include <algorithm>

#include "FastNoise/FastNoise.h"
//https://github.com/Auburns/FastNoise/wiki

//https://github.com/JCash/voronoi
#define JC_VORONOI_IMPLEMENTATION
// If you wish to use doubles
//#define JCV_REAL_TYPE double
//#define JCV_FABS fabs
//#define JCV_ATAN2 atan2
#include "jc_voronoi.h"

//----------------------------------- STATIC VARIABLES
float SCREEN_SIZE_X = 1600;
float SCREEN_SIZE_Y = 1000;

float MOUSE_X;
float MOUSE_Y;
float MOUSE_X_MAP;
float MOUSE_Y_MAP;

sf::Font FONT;

// --------- MOUSE
bool MOUSE_LEFT_HIT = false;
bool MOUSE_LEFT_DOWN = false;

bool MOUSE_RIGHT_HIT = false;
bool MOUSE_RIGHT_DOWN = false;


// --------------------------------------------------------- DRAWING
sf::Color HueToRGB(float hue) {
  hue = fmod(hue,1.0);
  hue*=6.0;
  if (hue<1.0) {
    return sf::Color(255.0,255.0*hue,0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(255.0-255.0*hue,255.0,0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(0,255.0,255.0*hue);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(0,255.0-255.0*hue,255.0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(255.0*hue,0,255.0);
  }
  hue-=1.0;
  if (hue<1.0) {
    return sf::Color(255.0,0,255.0-255.0*hue);
  }
  return sf::Color(255,255,255);
}

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

struct PolyMapCellState {
  enum Type {Pulsator, Pulse, Void};
  Type type = Type::Void;
  size_t value = 0;
  size_t counter = 0;
  bool isSelect = false;

  void setPulsator(){
    type = Type::Pulsator; value = 32; counter = 0;
  }

  void setVoid(){
    type = Type::Void; value = 0; counter = 0;
  }

  void setPulse(){
    type = Type::Pulse; value = 12; counter = 0;
  }
};

struct PolyMapCell
{
  _EPPos pos;
  sf::Color color;

  std::vector<_EPPos> corners;
  std::vector<PolyMapCell*> neighbors;

  void computeColor(size_t time, size_t epoch, std::vector<FastNoise> noiseGen) {
    float hue0 = (1.0+noiseGen[0].GetNoise(pos.x,pos.y,time*1))*0.3;
    float hue1 = (1.0+noiseGen[1].GetNoise(pos.x*0.1,pos.y*0.1,time*0.1))*1.0;
    sf::Color rgb = HueToRGB(hue0+hue1);
    float r=rgb.r, g=rgb.g, b=rgb.b;
    float white = (1.0+noiseGen[2].GetNoise(pos.x,pos.y,time*1))*0.5;
    float fraction = noiseGen[3].GetNoise(pos.x*0.1,pos.y*0.1,time*0.1)*10.0;
    fraction = std::max(0.0f,std::min(1.0f,fraction));
    r = fraction*r + (1.0-fraction)*255.0*white;
    g = fraction*g + (1.0-fraction)*255.0*white;
    b = fraction*b + (1.0-fraction)*255.0*white;
    //float r = 255.0*(1.0+noiseGen[0].GetNoise(pos.x,pos.y,time*1))*0.5;
    //float g = 255.0*(1.0+noiseGen[1].GetNoise(pos.x,pos.y,time*1))*0.5;
    //float b = 255.0*(1.0+noiseGen[2].GetNoise(pos.x,pos.y,time*1))*0.5;
    PolyMapCellState& s = state(epoch);
    if (s.type == PolyMapCellState::Type::Pulsator) {
      // full blast always
    }else if (s.type == PolyMapCellState::Type::Pulse) {
      //float factor = 1.0 - 1.0*(float)s.counter/s.value;
      float factor = std::pow(0.8,s.counter);
      r*=factor;g*=factor;b*=factor;
    } else {
      r*=0.1;g*=0.1;b*=0.1;
    }
    color = sf::Color(r,g,b);
  }

  void calculateStep(size_t epoch, std::vector<FastNoise> noiseGen) {
    PolyMapCellState& oldState = state(epoch+1);
    PolyMapCellState newState = oldState;

    if (oldState.type == PolyMapCellState::Type::Pulsator) {
      newState.counter+=1;
      if (newState.counter >= oldState.value) {
        newState.counter = 0;
      }
    } else if (oldState.type == PolyMapCellState::Type::Pulse) {
      newState.counter+=1;
      if (newState.counter >= oldState.value) {
        newState.setVoid();
      }
    } else if (oldState.type == PolyMapCellState::Type::Void) {
      bool makePulse = false;
      bool denyPulse = false;
      for(auto n : neighbors) {
        PolyMapCellState& nOld = n->state(epoch+1);
        if (nOld.type == PolyMapCellState::Type::Pulsator) {
          if (epoch % nOld.value == 0) {
            makePulse = true;
          }
        } else if (nOld.type == PolyMapCellState::Type::Pulse) {
          if (nOld.counter==0) {
            makePulse = true;
          } else {
            denyPulse = true;
          }
        }
      }
      if (makePulse and !denyPulse) {
        newState.setPulse();
      }
    }

    state(epoch) = newState;
  }

  void clear() {
    state0.setVoid();
    state1.setVoid();
  }

  PolyMapCellState state0,state1;
  PolyMapCellState& state(size_t epoch) {
    if (epoch % 2 == 0) {return state0;}else{return state1;}
  }
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

  std::vector<FastNoise> noiseGen;
  size_t time = 0;
  size_t epoch = 0;

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

    // -------------------- noise functions
    noiseGen.resize(6);
    for (size_t i = 0; i < noiseGen.size(); i++) {
      noiseGen[i].SetNoiseType(FastNoise::SimplexFractal);
      noiseGen[i].SetSeed(generation_seed + i);
      noiseGen[i].SetFrequency(0.01);
    }

    // get positions
    for(size_t i = 0; i<num_cells; i++)
    {
      cells[i].pos.x = points[i].x;
      cells[i].pos.y = points[i].y;
      cells[i].computeColor(0,0,noiseGen);
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
          cells[index].neighbors.push_back(&(cells[e->neighbor->index]));
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

  PolyMapCell* getCell(float x, float y, PolyMapCell* guess_cell)
  {
    size_t number_iterations = 0;
    // supposed to speed up, because could be close:
    PolyMapCell* res = guess_cell;

    float dx = res->pos.x - x;
    float dy = res->pos.y - y;
    float squaredist = dx*dx + dy*dy; // from initial guess

    bool done = false;
    while(!done)
    {
      done = true;

      // check neighbors of current guess, find better fit
      size_t nc = res->neighbors.size();

      for(int i=0; i<nc; i++)
      {
        PolyMapCell* cell = res->neighbors[i];
        dx = cell->pos.x - x;
        dy = cell->pos.y - y;

        float d = dx*dx + dy*dy;

        if(d < squaredist)
        {
          res = cell;
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
  void calculateStep() {
    std::cout << "step " << epoch << std::endl;
    for(size_t i = 0; i<num_cells; i++)
    {
      cells[i].calculateStep(epoch,noiseGen);
    }
  }
  void recolor() {
    time+=1;
    size_t newEpoch = time/1;
    if(not (epoch==newEpoch)){
      epoch = newEpoch;
      calculateStep();
    }
    for(size_t i = 0; i<num_cells; i++)
    {
      cells[i].computeColor(time,epoch,noiseGen);
    }
    create_mesh();
  }

  void clear() {
    for(size_t i = 0; i<num_cells; i++)
    {
      cells[i].clear();
    }
  }

  void draw(float x, float y, float zoom, sf::RenderWindow &window)
  {
    recolor();
    sf::Transform t;

    t.translate(SCREEN_SIZE_X*0.5, SCREEN_SIZE_Y*0.5);
    t.scale(zoom, zoom);
    t.translate(-x, -y);

    window.draw(&mesh[0], mesh.size(), sf::Triangles, t);

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
    }
  }
};

//-------------------------------------------------------- Main
int main()
{
    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    //sf::Style style = sf::Style::Fullscreen;//Default
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "first attempts", sf::Style::Fullscreen, settings);
    window.setVerticalSyncEnabled(true);
    window.setMouseCursorVisible(false);

    // ------------------------------------ FONT
    if (!FONT.loadFromFile("arial.ttf"))
    {
        std::cout << "font could not be loaded!" << std::endl;
        return 0;
    }

    // ------------------------------------ MAP
    //PolyMap map = PolyMap(30000, 1000, 1000);
    PolyMap map = PolyMap(40000, 4000, 2000);
    //PolyMap map = PolyMap(4800, 400, 400);

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
        MOUSE_X_MAP = (MOUSE_X - SCREEN_SIZE_X*0.5) / screen_zoom + screen_x;
        MOUSE_Y_MAP = (MOUSE_Y - SCREEN_SIZE_Y*0.5) / screen_zoom + screen_y;

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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)){
          map.clear();
        }

        window.clear();

        map.draw(screen_x, screen_y, screen_zoom, window);

        DrawDot(MOUSE_X, MOUSE_Y, window, sf::Color(255*MOUSE_RIGHT_DOWN,255*MOUSE_LEFT_DOWN,255));

        if (MOUSE_LEFT_DOWN) {
          PolyMapCell* select = map.getCell(MOUSE_X_MAP, MOUSE_Y_MAP, &(map.cells[0]));
          select->state(map.epoch).setPulsator();
        }

        if (MOUSE_RIGHT_DOWN) {
          PolyMapCell* select = map.getCell(MOUSE_X_MAP, MOUSE_Y_MAP, &(map.cells[0]));
          select->state(map.epoch).setVoid();
        }

        FPS_tick();
        DrawText(5,5, "FPS: " + std::to_string(framesPerSec), 10, window, sf::Color(255,255,255));
        DrawText(5,20, "avg: " + std::to_string(framesPerSec_avg), 10, window, sf::Color(255,255,255));
        DrawText(5,35, "ticker: " + std::to_string(ticker), 10, window, sf::Color(255,255,255));

        window.display();
    }

    return 0;
}
