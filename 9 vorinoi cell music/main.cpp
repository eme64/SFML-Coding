#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <SFML/Graphics.hpp>
#include <list>
#include <queue>
#include <algorithm>
#include <thread>

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

bool HUD_ENABLED = true;
bool SHOW_BACKGROUND = false;
int BACKGROUND_COLORGENERATOR = 0;
float BACKGROUND_COLORGENERATOR_SCALE = 1.0;
float BACKGROUND_COLORGENERATOR_TIME = 1.0;
std::vector<std::string> BACKGROUND_COLORGENERATORS = {
  "random RGB",
  "random BW",
  "saturation spots",
  "rgb field",
};

size_t NUM_THREADS = 8;

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
    float r, g, b;
    float scale = BACKGROUND_COLORGENERATOR_SCALE;
    float tscale = BACKGROUND_COLORGENERATOR_TIME;
    if (BACKGROUND_COLORGENERATOR==0) {
      r = 255.0*(1.0+noiseGen[0].GetNoise(pos.x*scale,pos.y*scale,time*tscale))*0.5;
      g = 255.0*(1.0+noiseGen[1].GetNoise(pos.x*scale,pos.y*scale,time*tscale))*0.5;
      b = 255.0*(1.0+noiseGen[2].GetNoise(pos.x*scale,pos.y*scale,time*tscale))*0.5;
    } else if (BACKGROUND_COLORGENERATOR==1) {
      r = 255.0*(1.0+noiseGen[0].GetNoise(pos.x*scale,pos.y*scale,time*tscale))*0.5;
      g = r;
      b = r;
    } else if (BACKGROUND_COLORGENERATOR==2) {
      float hue0 = (1.0+noiseGen[0].GetNoise(pos.x*scale,pos.y*scale,time*tscale))*0.3;
      float hue1 = (1.0+noiseGen[1].GetNoise(pos.x*scale*0.1,pos.y*scale*0.1,time*tscale*0.1))*1.0;
      sf::Color rgb = HueToRGB(hue0+hue1);
      r=rgb.r, g=rgb.g, b=rgb.b;
      float white = (1.0+noiseGen[2].GetNoise(pos.x*scale,pos.y*scale,time*tscale))*0.5;
      float fraction = noiseGen[3].GetNoise(pos.x*scale*0.1,pos.y*scale*0.1,time*tscale*0.1)*10.0;
      fraction = std::max(0.0f,std::min(1.0f,fraction));
      r = fraction*r + (1.0-fraction)*255.0*white;
      g = fraction*g + (1.0-fraction)*255.0*white;
      b = fraction*b + (1.0-fraction)*255.0*white;
    } else {
      const float w = 1000.0;
      r = 255.0*fmod(pos.x*scale+time*tscale,w)/w;
      g = 255.0*fmod(pos.y*scale+time*tscale,w)/w;
      b = 255.0*fmod(std::abs(pos.x*scale+pos.y*scale-time*tscale),w)/w;
    }


    PolyMapCellState& s = state(epoch);

    float dimFactor = 1.0;

    if (s.type == PolyMapCellState::Type::Pulsator) {
      // full blast always
    }else if (s.type == PolyMapCellState::Type::Pulse) {
      //float factor = 1.0 - 1.0*(float)s.counter/s.value;
      dimFactor = std::pow(0.8,s.counter);
    } else {
      if (!SHOW_BACKGROUND) {
        dimFactor = 0.1;
      }
    }
    color = sf::Color(r*dimFactor,g*dimFactor,b*dimFactor);
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

  enum MapGeneratorType {Random,RandomNonUniform,RegularHex,PerturbedHex};

  PolyMap(size_t n_cells, float s_x, float s_y, MapGeneratorType genType)
  {
    num_cells = n_cells;

    // for grid placements:
    // x/y = sx/sy;
    // x*y = num_cells; -> x = num_cells/y;
    // num_cells/y/y = sx/sy
    // y*y = sy/sx*num_cells;

    spread_x = s_x;
    spread_y = s_y;
    size_t num_cells_y = std::sqrt(num_cells*spread_y/spread_x);
    size_t num_cells_x = (num_cells+num_cells_y-1)/num_cells_y;
    float dx = spread_x/(num_cells_x+1);
    float dy = spread_y/(num_cells_y+1);
    std::cout << num_cells << " " << num_cells_x*num_cells_y << " " << num_cells_x << " " << num_cells_y << std::endl;
    std::cout << dx << " " << dy << std::endl;


    // --------------------------- Prepare Points
    jcv_point* points = NULL;
    points = (jcv_point*)malloc( sizeof(jcv_point) * (size_t)num_cells);

    for(size_t i = 0; i<num_cells; i++)
    {
      if (genType == MapGeneratorType::RegularHex or genType == MapGeneratorType::PerturbedHex) {
        size_t xx = i % num_cells_x;
        size_t yy = i / num_cells_x;
        points[i].x = dx*((float)xx+0.5*(yy % 2));
        points[i].y = dy*(yy+0.5);
        if (genType == MapGeneratorType::PerturbedHex) {
          points[i].x+=dx*(((float) rand() / (RAND_MAX))*0.3-0.15);
          points[i].y+=dy*(((float) rand() / (RAND_MAX))*0.3-0.15);
        }
      } else { // Random,RandomNonUniform
        points[i].x = spread_x*((float) rand() / (RAND_MAX));
        points[i].y = spread_y*((float) rand() / (RAND_MAX));
      }
    }

    // --------------------------- Prepare Diagram
    jcv_diagram diagram;
    _jcv_rect rect;
    rect.min.x = 0;
    rect.min.y = 0;
    rect.max.x = spread_x;
    rect.max.y = spread_y;
    voronoi_generate(num_cells, points, diagram, &rect);

    size_t numRelax = 10;
    if (genType == MapGeneratorType::RegularHex or genType == MapGeneratorType::PerturbedHex) {
      numRelax = 0;
    }
    if (genType == MapGeneratorType::RandomNonUniform) {
      numRelax = 1;
    }

    for(size_t i=0; i<numRelax; i++) // relax some -> make more equispaced
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
    bool isNewEpoch = false;
    if(not (epoch==newEpoch)){
      epoch = newEpoch;
      isNewEpoch = true;
    }
    //clock_t t1 = clock(); // CLOCK

    std::vector<std::thread> threads;
    for (size_t tid = 0; tid < NUM_THREADS; tid++) {
      threads.push_back(std::thread([this,tid,isNewEpoch]{
        size_t num_cells_local = (num_cells+NUM_THREADS-1)/NUM_THREADS;
        size_t start = tid*num_cells_local;
        size_t end = std::min(start+num_cells_local,num_cells);
        for (size_t i = start; i < end; i++) {
          cells[i].calculateStep(epoch,noiseGen);
          cells[i].computeColor(time,epoch,noiseGen);
        }
      }));
    }
    for (std::thread &t : threads) {
      t.join();
    }

    //clock_t t2 = clock(); // CLOCK
    create_mesh();
    //clock_t t3 = clock(); // CLOCK
    //std::cout << "time: " << (t2-t1) << " " << (t3-t2) << std::endl;
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
    std::vector<size_t> map_size_num = {4800,10000,40000,160000};
    std::vector<size_t> map_size_x = {400,2000,4000,8000};
    std::vector<size_t> map_size_y = {400,1000,2000,4000};
    std::vector<std::string> map_size_text = {"small","medium","large","extra large"};
    std::vector<PolyMap::MapGeneratorType> map_genTypes = {
      PolyMap::MapGeneratorType::Random,
      PolyMap::MapGeneratorType::RandomNonUniform,
      PolyMap::MapGeneratorType::RegularHex,
      PolyMap::MapGeneratorType::PerturbedHex,
    };
    std::vector<std::string> map_genTypes_text = {
      "random",
      "random non-uniform",
      "hex",
      "hex noisy",
    };

    size_t map_num = map_size_num[2];
    size_t map_x = map_size_x[2];
    size_t map_y = map_size_y[2];
    std::string map_text = map_size_text[2];
    PolyMap::MapGeneratorType map_genType = map_genTypes[0];
    std::string map_genType_text = map_genTypes_text[0];
    PolyMap map = PolyMap(map_num, map_x, map_y, map_genType);

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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)){
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
            HUD_ENABLED = false;
          } else {
            HUD_ENABLED = true;
          }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::L)){
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
            SHOW_BACKGROUND = false;
          } else {
            SHOW_BACKGROUND = true;
          }
        }

        bool regenerateMap = false;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)){
          int param = -1;
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
            param = 0;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
            param = 1;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) {
            param = 2;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) {
            param = 3;
          }
          if (param>=0) {
            regenerateMap = true;
            map_num = map_size_num[param];
            map_x = map_size_x[param];
            map_y = map_size_y[param];
            map_text = map_size_text[param];
          }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::B)){
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
            BACKGROUND_COLORGENERATOR = 0;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
            BACKGROUND_COLORGENERATOR = 1;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) {
            BACKGROUND_COLORGENERATOR = 2;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) {
            BACKGROUND_COLORGENERATOR = 3;
          }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)){
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
            BACKGROUND_COLORGENERATOR_SCALE = 10.0;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
            BACKGROUND_COLORGENERATOR_SCALE = 3.0;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) {
            BACKGROUND_COLORGENERATOR_SCALE = 1.0;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) {
            BACKGROUND_COLORGENERATOR_SCALE = 0.5;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)) {
            BACKGROUND_COLORGENERATOR_SCALE = 0.25;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)) {
            BACKGROUND_COLORGENERATOR_SCALE = 0.1;
          }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::T)){
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
            BACKGROUND_COLORGENERATOR_TIME = 10.0;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
            BACKGROUND_COLORGENERATOR_TIME = 3.0;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) {
            BACKGROUND_COLORGENERATOR_TIME = 1.0;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) {
            BACKGROUND_COLORGENERATOR_TIME = 0.5;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)) {
            BACKGROUND_COLORGENERATOR_TIME = 0.25;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)) {
            BACKGROUND_COLORGENERATOR_TIME = 0.1;
          }
        }


        if (sf::Keyboard::isKeyPressed(sf::Keyboard::G)){
          int param = -1;
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
            param = 0;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
            param = 1;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) {
            param = 2;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) {
            param = 3;
          }
          if (param>=0) {
            regenerateMap = true;
            map_genType = map_genTypes[param];
            map_genType_text = map_genTypes_text[param];
          }
        }

        if (regenerateMap) {
          map = PolyMap(map_num, map_x, map_y, map_genType);
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
        if (HUD_ENABLED) {
          DrawText(5,5, "FPS: " + std::to_string(framesPerSec), 10, window, sf::Color(255,255,255));
          DrawText(5,20, "avg: " + std::to_string(framesPerSec_avg), 10, window, sf::Color(255,255,255));
          DrawText(5,35, "ticker: " + std::to_string(ticker), 10, window, sf::Color(255,255,255));

          DrawText(5,50, "[WASDQE] navigate/scale", 20, window, sf::Color(255,255,255));
          DrawText(5,75, "[L] lighten background", 20, window, sf::Color(255,255,255));
          DrawText(5,100, "[H] enable HUD", 20, window, sf::Color(255,255,255));
          DrawText(5,125, "[SPACE] clear map", 20, window, sf::Color(255,255,255));
          DrawText(5,150, "[R+1234] resize map: "+map_text, 20, window, sf::Color(255,255,255));
          DrawText(5,175, "[G+1234] map generator: "+map_genType_text, 20, window, sf::Color(255,255,255));
          DrawText(5,200, "[B+1234] background color generator: "+BACKGROUND_COLORGENERATORS[BACKGROUND_COLORGENERATOR], 20, window, sf::Color(255,255,255));
          DrawText(5,225, "[Z+123456] background color zoom: "+std::to_string(BACKGROUND_COLORGENERATOR_SCALE), 20, window, sf::Color(255,255,255));
          DrawText(5,250, "[T+123456] background color time: "+std::to_string(BACKGROUND_COLORGENERATOR_TIME), 20, window, sf::Color(255,255,255));
        }

        window.display();
    }

    return 0;
}
