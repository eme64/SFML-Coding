#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <list>
#include <queue>
#include <algorithm>
#include <thread>
#include <valarray>
#include <complex>
#include <mutex>
#include <forward_list>
#include <functional>

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
  "plain white",
  "single color",
  "land",
  "shapes hue",
  "shapes dicrete",
  "distorted squares",
  "music board",
};
size_t PULSATOR_COLOR = 0;
bool PULSATOR_NOT_PULSE = false;

size_t EPOCH_STEPS = 1;

size_t NUM_THREADS = 8;

std::function<size_t(float,float)> MUSIC_BOARD_FUNCTION = [](float x, float y) { return 1*(fmod(x+y,200.0)>100.0); };

// --------------------------------------------------------- PONG:
float PONG_Y1 = SCREEN_SIZE_Y/2;
float PONG_Y2 = SCREEN_SIZE_Y/2;
float PONG_YGoal1 = PONG_Y1;
float PONG_YGoal2 = PONG_Y2;
float PONG_YV1 = 0;
float PONG_YV2 = 0;
float PONG_RADIUS = 100;

float PONG_BALL_X = SCREEN_SIZE_X/2;
float PONG_BALL_Y = SCREEN_SIZE_Y/2;
float PONG_BALL_XV = 10;
float PONG_BALL_YV = 0;

bool PONG_ENABLED = false;

// --------------------------------------------------------- END PONG

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

void DrawRect(float x, float y, float dx, float dy, sf::RenderWindow &window, sf::Color color)
{
  sf::RectangleShape rectangle;
  rectangle.setSize(sf::Vector2f(dx, dy));
  rectangle.setFillColor(color);
  rectangle.setPosition(x, y);
  window.draw(rectangle, sf::BlendAdd);
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
  size_t color = 0; // 0: bg, 1+: custom
  bool isSelect = false;

  void setPulsator(size_t color){
    type = Type::Pulsator; value = 32; counter = 0; this->color = color;
  }

  void setVoid(){
    type = Type::Void; value = 0; counter = 0;
  }

  void setPulse(size_t color){
    type = Type::Pulse; value = 12; counter = 0; this->color = color;
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
    } else if (BACKGROUND_COLORGENERATOR==3) {
      const float w = 1000.0;
      r = 255.0*fmod(pos.x*scale+time*tscale,w)/w;
      g = 255.0*fmod(pos.y*scale+time*tscale,w)/w;
      b = 255.0*fmod(std::abs(pos.x*scale+pos.y*scale-time*tscale),w)/w;
    } else if (BACKGROUND_COLORGENERATOR==4) {
      r = 255; g=255; b = 255;
    } else if (BACKGROUND_COLORGENERATOR==5) {
      r = 255.0*(1.0+noiseGen[0].GetNoise(0,time*tscale))*0.5;
      g = 255.0*(1.0+noiseGen[1].GetNoise(0,time*tscale))*0.5;
      b = 255.0*(1.0+noiseGen[2].GetNoise(0,time*tscale))*0.5;
    } else if (BACKGROUND_COLORGENERATOR==6) {
      float lscale = 0.1;
      float water_frac = 0.5; // 0 - 1
      float hight_noise = 1.0; // 0 - 1
      auto height = [&lscale,&noiseGen](float x, float y, float t)
      {
        return (1.0+noiseGen[0].GetNoise(x*lscale*2.0,y*lscale*2.0,t))*0.5;
      };
      auto waterColor = [&lscale,&noiseGen, &water_frac](float x, float y, float height, float t)
      {
        float water_depth_ratio = height / water_frac;
        float light = water_depth_ratio + 0.4*(1.0+noiseGen[1].GetNoise(x*lscale,y*lscale,t))*0.5;
        float color = (1.0+noiseGen[2].GetNoise(x*lscale,y*lscale,t))*0.5;
        float c_inv = 1.0 - color;
        float r = light * 40;
        float g = light * (80 * color + 120 * c_inv);
        float b = light * (200 * color + 160 * c_inv);
        return sf::Color(r,g,b);
      };
      auto sandColor = [&lscale,&noiseGen](float x, float y, float t)
      {
        float light = 1 + 0.2*noiseGen[3].GetNoise(x,y,t);
        float color = (1.0+noiseGen[4].GetNoise(x*lscale,y*lscale,t))*0.5;
        float c_inv = 1.0 - color;
        float r = light * (76 * color + 200 * c_inv);
        float g = light * (70 * color + 100 * c_inv);
        float b = light * (50 * color + 30 * c_inv);

        return sf::Color(r,g,b);
      };
      auto grassColor = [&lscale,&noiseGen, &water_frac](float x, float y, float height, float t)
      {
        // fraction over water
        float fow = (height - water_frac) / (1.0 - water_frac);
        float fow_inv = 1.0 - fow;

        float light = 1 + 0.2*noiseGen[5].GetNoise(x*lscale*3.0,y*lscale*3.0,t);
        float color = (1.0+noiseGen[6].GetNoise(x*lscale*3.0,y*lscale*3.0,t))*0.5;
        float c_inv = 1.0 - color;
        float r_grass = light * (0 * color + 20 * c_inv);
        float g_grass = light * (100 * color + 80 * c_inv);
        float b_grass = light * (0 * color + 30 * c_inv);

        float r_hill = light * (50 * color + 70 * c_inv);
        float g_hill = light * (50 * color + 50 * c_inv);
        float b_hill = light * (50 * color + 30 * c_inv);


        return sf::Color(r_grass * fow_inv + r_hill * fow, g_grass * fow_inv + g_hill * fow, b_grass * fow_inv + b_hill * fow);
      };

      float h = height(pos.x*scale,pos.y*scale,time*tscale);

      sf::Color rgb = sf::Color(255 * h,255 * h,255 * h);

      if(h<water_frac)
      {
        rgb = waterColor(pos.x*scale,pos.y*scale, h,time*tscale);
      } else if(h < (water_frac + 0.05)) {
        rgb = sandColor(pos.x*scale,pos.y*scale,time*tscale);
      } else {
        rgb = grassColor(pos.x*scale,pos.y*scale, h,time*tscale);
      }

      r = rgb.r; g = rgb.g; b = rgb.b;
    } else if (BACKGROUND_COLORGENERATOR==7) {
      float lscale = 0.1;
      float w = 0.05;
      float h1 = (1.0+noiseGen[0].GetNoise(pos.x*scale*lscale,pos.y*scale*lscale,time*tscale))*0.5;
      float hue = (1.0+noiseGen[1].GetNoise(pos.x*scale*lscale,pos.y*scale*lscale,time*tscale))*0.5;
      sf::Color rgb(255,0,0);
      if (fmod(h1,w)/w<0.3) {
        rgb = HueToRGB(hue*5.0);
      }else{
        rgb = HueToRGB(hue*5.0+0.5);
      }
      r=rgb.r, g=rgb.g, b=rgb.b;
    } else if (BACKGROUND_COLORGENERATOR==8) {
      float w = 0.05;
      size_t dim = 3;
      size_t num = 0;
      size_t power = 1;
      for (size_t i = 0; i < dim; i++) {
        float h = (1.0+noiseGen[i].GetNoise(pos.x*scale,pos.y*scale,time*tscale))*0.5;
        if (h>0.5) {
          num+=power;
        }
        power*=2;
      }
      r = 255.0*(1.0+noiseGen[dim+0].GetNoise(num*100.0,time*tscale))*0.5;
      g = 255.0*(1.0+noiseGen[dim+1].GetNoise(num*100.0,time*tscale))*0.5;
      b = 255.0*(1.0+noiseGen[dim+2].GetNoise(num*100.0,time*tscale))*0.5;
    } else if (BACKGROUND_COLORGENERATOR==9) {
      float w = 0.4;
      float lscale = 0.1;
      float a = noiseGen[0].GetNoise(pos.x*scale*lscale,pos.y*scale*lscale,time*tscale)*4.0;
      float dx = std::sin(a)*30.0/scale/w;
      float dy = std::cos(a)*30.0/scale/w;
      float x = std::floor((pos.x+dx)*w*scale*lscale);
      float y = std::floor((pos.y+dy)*w*scale*lscale);
      r = 255.0*(1.0+noiseGen[2].GetNoise(x*100.0,y*100.0,time*tscale))*0.5;
      g = 255.0*(1.0+noiseGen[3].GetNoise(x*100.0,y*100.0,time*tscale))*0.5;
      b = 255.0*(1.0+noiseGen[4].GetNoise(x*100.0,y*100.0,time*tscale))*0.5;
    } else if (BACKGROUND_COLORGENERATOR==10) {
      size_t state = MUSIC_BOARD_FUNCTION(pos.x,pos.y);
      bool isSet = (state % 2)==1;
      state/=2;
      bool isActive = (state % 2)==1;
      state/=2;
      bool isOddX = (state % 2)==1;
      state/=2;
      bool isOddY = (state % 2)==1;

      float lscale = 0.1;
      float hue = (1.0+noiseGen[0].GetNoise(pos.x*scale*lscale,pos.y*scale*lscale,time*tscale))*3.0;
      float brightness = 1.0;
      sf::Color rgb(0,0,0);
      if (isSet) {
        if (isActive) {
          rgb = HueToRGB(hue);
        }else{
          rgb = HueToRGB(hue+0.1);
          brightness = 0.5;
        }
      }else{
        if (isActive) {
          rgb = HueToRGB(hue+0.3);
        }else{
          rgb = HueToRGB(hue+0.4);
          brightness = 0.5;
        }
      }
      if (isOddX) {
        brightness*=0.9;
      }
      if (isOddY) {
        brightness*=0.9;
      }
      r=rgb.r*brightness, g=rgb.g*brightness, b=rgb.b*brightness;
    }


    PolyMapCellState& s = state(epoch);

    float dimFactor = 1.0;

    if (s.type == PolyMapCellState::Type::Pulsator) {
      if (s.color>0) {
        float hue = (float)s.color/9.0;
        sf::Color rgb = HueToRGB(hue);
        r=rgb.r, g=rgb.g, b=rgb.b;
      }
    }else if (s.type == PolyMapCellState::Type::Pulse) {
      //float factor = 1.0 - 1.0*(float)s.counter/s.value;
      if (s.color>0) {
        float intensity = (r+g+b)/3.0/255.0;
        float hue = (float)s.color/9.0+(1.0+noiseGen[0].GetNoise(pos.x*scale*0.1,pos.y*scale*0.1,time*tscale))*2.0;
        sf::Color rgb = HueToRGB(hue);
        r=rgb.r*intensity, g=rgb.g*intensity, b=rgb.b*intensity;
      }
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
      bool makePulse = false;
      size_t makePulseColor = oldState.color;
      for(auto n : neighbors) {
        PolyMapCellState& nOld = n->state(epoch+1);
        if (nOld.type == PolyMapCellState::Type::Pulse) {
          int diff = (nOld.color - makePulseColor+9) % 9;
          if (diff > 5) {
            makePulseColor = nOld.color;
            makePulse = true;
          }
        }
      }
      if (makePulse) {
        newState.setPulse(makePulseColor);
      }
    } else if (oldState.type == PolyMapCellState::Type::Void) {
      bool makePulse = false;
      size_t makePulseColor = 0;
      bool denyPulse = false;
      for(auto n : neighbors) {
        PolyMapCellState& nOld = n->state(epoch+1);
        if (nOld.type == PolyMapCellState::Type::Pulsator) {
          if (epoch % nOld.value == 0) {
            makePulse = true;
            makePulseColor = nOld.color;
          }
        } else if (nOld.type == PolyMapCellState::Type::Pulse) {
          if (nOld.counter==0) {
            makePulse = true;
            makePulseColor = nOld.color;
          } else {
            denyPulse = true;
          }
        }
      }
      if (makePulse and !denyPulse) {
        newState.setPulse(makePulseColor);
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
    size_t newEpoch = time/EPOCH_STEPS;
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

//-------------------------------------------------------- SOUND recorder

const double PI  = 3.141592653589793238463;
typedef std::complex<double> ComplexVal;
typedef std::valarray<ComplexVal> SampleArray;
void cooleyTukeyFFT(SampleArray& values) {
	const size_t N = values.size();
	if (N <= 1) return; //base-case

	//separate the array of values into even and odd indices
	SampleArray evens = values[std::slice(0, N / 2, 2)]; //slice starting at 0, N/2 elements, increment by 2
	SampleArray odds = values[std::slice(1, N / 2, 2)]; //slice starting at 1

	//now call recursively
	cooleyTukeyFFT(evens);
	cooleyTukeyFFT(odds);

	//recombine
	for(size_t i=0; i<N/2; i++) {
		ComplexVal index = std::polar(1.0, -2 * PI*i / N) * odds[i];
		values[i] = evens[i] + index;
		values[i + N / 2] = evens[i] - index;
	}
}

class MyRecorder : public sf::SoundRecorder {
    virtual bool onStart() {
        std::cout << "Start" << std::endl;
        setProcessingInterval(sf::milliseconds(0.1));
        return true;
    }

    virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount) {
        const size_t newSize = std::pow(2.0, (size_t) std::log2(sampleCount));
        std::cout << "process " << samples[0] << " " << sampleCount << " " << newSize << std::endl;
        std::lock_guard<std::mutex> lock(mutex_);
        array_.resize(newSize);
        for (size_t i = 0; i < newSize; i++) {
          array_[i] = samples[i];
        }
        cooleyTukeyFFT(array_);
        std::cout << array_[0] << std::endl;
        return true;
    }

    virtual void onStop() {
        std::cout << "Stop" << std::endl;
    }
public:
		float getValue() {
			// return value between 0..1, if out of bounds: consider off
			std::lock_guard<std::mutex> lock(mutex_);
			float res = transformInv((double)getMaxIndex()*2 / array_.size());
			return (res-0.07)*13.0;
		}
    void draw(sf::RenderWindow &window)
    {
        std::cout << "draw" << std::endl;
        if (array_.size()==0) {
          return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        const size_t N = 1000, dx=1, dy=1;
        const double A = 0.0001;
        for (size_t i = 0; i < N; i++) {
          const size_t index = array_.size()*0.5*transform((double)i/(double)N);
          const double value = std::abs(array_[index]);
          sf::RectangleShape rectangle;
          rectangle.setSize(sf::Vector2f(dx, dy));
          rectangle.setPosition(i*dx, value*A*dy+100);
          rectangle.setFillColor(sf::Color(200,0,0));
          window.draw(rectangle);
        }
        {
          int x = N*transformInv((double)getMaxIndex()*2 / array_.size());
          sf::RectangleShape rectangle;
          rectangle.setSize(sf::Vector2f(5,5));
          rectangle.setPosition(x*dx, 100);
          rectangle.setFillColor(sf::Color(0,200,0));
          window.draw(rectangle);
        }
    }
    double transform(const double in) const {
      //return in;
      return std::pow(2.0,in)-1.0;
    }
    double transformInv(const double out) const {
      //return out;
      return std::log2(out+1.0);
    }
    size_t getMaxIndex() const {
      size_t m = 0;
      double maxVal = 0;
      for (size_t i = 0; i < array_.size()/2; i++) {
        const double loc = std::abs(array_[i]);
        if (maxVal < loc) {
          maxVal = loc;
          m = i;
        }
      }
      std::cout << m << std::endl;
      return m;
    }
private:
  std::mutex mutex_;
  SampleArray array_;
};

//-------------------------------------------------------- Synth
class Instrument
{
public:
  bool isAlive = true;

  Instrument(double frequency, std::function<double(double, double)> oscillator)
  :frequency(frequency), oscillator(oscillator)
  {
    // empty
  }

  static double oscillator_sin(double t, double freq)
  {
    return std::sin(t * freq*2.0*M_PI + 0.1*std::sin(t * 5.0 * 2.0 * M_PI));
  }

  static double oscillator_mod(double t, double freq)
  {
    return 2.0*std::fmod(t * freq + 0.1*std::sin(t * 5.0 * 2.0 * M_PI), 1.0)-1.0;
  }

  static double oscillator_tria(double t, double freq)
  {
    double saw = std::fmod(t * freq + 0.1*std::sin(t * 5.0 * 2.0 * M_PI), 1.0);
    return std::abs(saw-0.5)*2.0;
  }

  double sample(double t)
  {
    if (attacked == false) { // first time playing:
      attacked = true;
      when_attacked = t;
    }

    double res = oscillator(t, frequency);

    if(attacked)
    {
      res *= attack_envelope(t - when_attacked);
    }

    if(released)
    {
      res *= release_envelope(t - when_released);
    }

    t_last = t;
    return res;
  }

  void release()
  {
    released = true;
    when_released = t_last;
  }

private:
  std::function<double(double, double)> oscillator; // t, freq

  double t_last;
  bool attacked = false;
  bool released = false;
  double when_attacked;
  double when_released;
  double frequency;

  double attack_envelope(double t)// t since attack:
  {
    const double attack_time = 0.005;
    const double decay_time = 0.01;
    const double sustain_level = 0.3;

    if (t < attack_time) {
      return t/attack_time;
    }
    t-=attack_time;
    if (t < decay_time) {
      return (1.0-t/decay_time) *(1.0-sustain_level) + sustain_level;
    }

    return sustain_level;
  }

  double release_envelope(double t) // t since release
  {
    double res = std::max(0.0, 1.0-t/0.2);
    if (res < 0.001) {
      isAlive = false;
    }
    return res;
  }
};

// custom audio stream that plays a loaded buffer
class MyStream : public sf::SoundStream
{
public:
  MyStream()
  :mySampleRate(44100), mySampleSize(2000), myAmplitude(30000)
  {
    ticks = 0;
    initialize(1, mySampleRate);
  }

  void playInstrument(Instrument *instrument)
  {
    std::lock_guard<std::mutex> lock(instruments_mutex);
    instruments.push_front(instrument);
  }

  void stopInstrument(Instrument *instrument)
  {
    instrument->release();
  }

  void setTicker(std::function<void()> ticker)
  {
    this->ticker = ticker;
  }

private:
  std::function<void()> ticker = [](){};

  const unsigned mySampleRate;
  const unsigned mySampleSize;
  const unsigned myAmplitude;

  std::vector<sf::Int16> samples;

  size_t ticks;

  std::mutex instruments_mutex;
  std::forward_list<Instrument*> instruments;

  virtual bool onGetData(Chunk& data)
  {
    samples.clear();

    for (size_t i = 0; i < mySampleSize; i++) {
      ticks++;
      ticker();
      double t = (double)(ticks) / (double)mySampleRate;

      double res = 0.0;

      std::vector<Instrument*> delete_vec;

      std::lock_guard<std::mutex> lock(instruments_mutex);
      for (Instrument *i : instruments) {
        res += i->sample(t)*0.05;

        if(!i->isAlive)
        {
          delete_vec.push_back(i);
        }
      }

      for (Instrument *i : delete_vec) {
        instruments.remove(i);
        delete(i);
      }

      samples.push_back(myAmplitude * res);
    }

    data.samples = &samples[0];
    data.sampleCount = mySampleSize;
    return true;
  }

  virtual void onSeek(sf::Time timeOffset)
  {
    // not supported.
  }
};


class MusicBoard
{
public:
  MusicBoard(MyStream &stream, double size_x, double size_y, int n_x, int n_y)
  :size_x(size_x), size_y(size_y), n_x(n_x), n_y(n_y),
  dx(size_x/n_x), dy(size_y/n_y),
  stream(stream),
  oscillator(Instrument::oscillator_sin)
  {
    grid.resize(n_x);
    instruments.resize(n_x);
    for (size_t i = 0; i < n_x; i++) {
      grid[i].resize(n_y);
      instruments[i].resize(n_y);
    }

    resetFrequencies();
  }

  void resize(int n_x, int n_y)
  {
    clear();

    this->n_x = n_x;
    this->n_y = n_y;


    dx = size_x/n_x;
    dy = size_y/n_y;

    grid.resize(n_x);
    instruments.resize(n_x);
    for (size_t i = 0; i < n_x; i++) {
      grid[i].resize(n_y);
      instruments[i].resize(n_y);
    }

    clear();

    resetFrequencies();

    active_column = 0;
    count_ticks = 0;
  }

  void setFrequencies(std::vector<double> freq)
  {
    for (size_t j = 0; j < n_y; j++) {
      int octave = j/freq.size() + 1;
      size_t offset = j % freq.size();
      frequencies[n_y-j-1] = freq[offset]*std::pow(2.0,octave);
    }
  }

  void resetFrequencies()
  {
    frequencies.resize(n_y);
    for (size_t j = 0; j < n_y; j++) {
      frequencies[j] = 110*std::pow(2.0,(double)(n_y-j)/12.0);
    }
  }

  void setOscillator(std::function<double(double, double)> o)
  {
    oscillator = o;
  }

  void draw(sf::RenderWindow &window)
  {
    for (size_t i = 0; i < n_x; i++) {
      for (size_t j = 0; j < n_y; j++) {
        sf::RectangleShape rectangle;
        rectangle.setSize(sf::Vector2f(dx-1, dy-1));
        rectangle.setPosition(i*dx, j*dy);
        if(grid[i][j])
        {
          if (i == active_column) {
            rectangle.setFillColor(sf::Color(200,0,0));
          }else{
            rectangle.setFillColor(sf::Color(100,0,0));
          }
        }else{
          if (i == active_column) {
            rectangle.setFillColor(sf::Color(0,150,200));
          }else{
            rectangle.setFillColor(sf::Color(0,100,150));
          }
        }
        window.draw(rectangle);
      }
    }
  }

  void set(bool value, double x, double y)
  {
    int i = std::min(n_x-1.0, std::max(0.0, x / dx));
    int j = std::min(n_y-1.0, std::max(0.0, y / dy));

    grid[i][j] = value;
  }

  size_t get(double x, double y)
  {
    int i = std::min(n_x-1.0, std::max(0.0, x / dx));
    int j = std::min(n_y-1.0, std::max(0.0, y / dy));

    return grid[i][j] + (active_column==i)*2 + (i%2)*4 + (j%2)*8;
  }

  void setCurrentColumn(bool value, double y,bool deleteUpDown=false)
  {
    int j = std::min(n_y-1.0, std::max(0.0, y / dy));

    grid[active_column][j] = value;
    if (deleteUpDown) {
      if (j>0) {
        grid[active_column][j-1] = false;
      }
      if (j<n_y-1) {
        grid[active_column][j+1] = false;
      }
    }
  }

  void tick()
  {
    count_ticks++;
    if (count_ticks>=ticks_per_column) {
      count_ticks = 0;

      size_t old_col = active_column;
      active_column++;
      if (active_column>=n_x) {
        active_column = 0;
      }

      for (size_t j = 0; j < n_y; j++) {
        if (instruments[old_col][j] == NULL) {
          // start new if needed:
          if (grid[active_column][j]) {
            instruments[active_column][j] = new Instrument(frequencies[j], oscillator);
            stream.playInstrument(instruments[active_column][j]);
          }
        }else{
          if (grid[active_column][j]) {
            // move instrument:
            instruments[active_column][j] = instruments[old_col][j];
            instruments[old_col][j] = NULL;
          }else{
            // stop old.
            stream.stopInstrument(instruments[old_col][j]);
            instruments[old_col][j] = NULL;
          }
        }
      }
    }
  }

  void clear()
  {
    for (size_t i = 0; i < n_x; i++) {
      for (size_t j = 0; j < n_y; j++) {
        grid[i][j] = false;
        if (instruments[i][j] != NULL) {
          stream.stopInstrument(instruments[i][j]);
          instruments[i][j] = NULL;
        }
      }
    }
  }
private:
  MyStream &stream;

  double size_x; // screen
  double size_y;

  double dx;
  double dy;

  int n_x; // grid units
  int n_y;

  std::vector<std::vector<bool>> grid;
  std::vector<std::vector<Instrument*>> instruments;
  size_t active_column = 0;
  size_t ticks_per_column = 5000;
  size_t count_ticks = 0;

  std::vector<double> frequencies;

  std::function<double(double, double)> oscillator;
};

//-------------------------------------------------------- Main
int main()
{
    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    //sf::Style style = sf::Style::Fullscreen;//Default
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "first attempts", sf::Style::Fullscreen, settings);
    sf::Vector2u size = window.getSize();
    SCREEN_SIZE_X = size.x;
    SCREEN_SIZE_Y = size.y;
    window.setVerticalSyncEnabled(true);
    window.setMouseCursorVisible(false);

    // ------------------------------------ FONT
    if (!FONT.loadFromFile("arial.ttf"))
    {
        std::cout << "font could not be loaded!" << std::endl;
        return 0;
    }

    // ------------------------------------ Recorder:
    if (!MyRecorder::isAvailable()) {
        std::cout << "not available" << std::endl;
    }
    MyRecorder recorder;
    recorder.start();

    // ------------------------------------ Stream:
    MyStream stream;
    stream.play();

    MusicBoard board(stream, SCREEN_SIZE_X, SCREEN_SIZE_Y, 32, 32);
    stream.setTicker([&board](){board.tick();});

    // ------------------------------------ MAP
    std::vector<size_t> map_size_num = {4800,10000,24000,40000,80000,160000};
    std::vector<size_t> map_size_x = {700,2000,2700,4000,5300,8000};
    std::vector<size_t> map_size_y = {400,1000,1400,2000,2700,4000};
    std::vector<std::string> map_size_text = {"very small","small","medium","big","large","extra large"};
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
    float screen_x = map_x/2;
    float screen_y = map_y/2;
    float screen_zoom = SCREEN_SIZE_Y/map_y;

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


    // --------------------------- MUSIC_BOARD_FUNCTION
    MUSIC_BOARD_FUNCTION = [&board,&map,&map_x,&map_y](float x, float y) {
      return board.get(x*SCREEN_SIZE_X/map_x,y*SCREEN_SIZE_Y/map_y);
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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::M)){
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
            PONG_ENABLED = false;
          } else {
            PONG_ENABLED = true;
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
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)) {
            param = 4;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)) {
            param = 5;
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
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)) {
            BACKGROUND_COLORGENERATOR = 4;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)) {
            BACKGROUND_COLORGENERATOR = 5;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num7)) {
            BACKGROUND_COLORGENERATOR = 6;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)) {
            BACKGROUND_COLORGENERATOR = 7;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num9)) {
            BACKGROUND_COLORGENERATOR = 8;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num0)) {
            BACKGROUND_COLORGENERATOR = 9;
          }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F1)) {
          BACKGROUND_COLORGENERATOR = 10;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)){
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
            BACKGROUND_COLORGENERATOR_SCALE = 3.0;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
            BACKGROUND_COLORGENERATOR_SCALE = 2.0;
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
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num7)) {
            BACKGROUND_COLORGENERATOR_SCALE = 0.05;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)) {
            BACKGROUND_COLORGENERATOR_SCALE = 0.025;
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
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num7)) {
            BACKGROUND_COLORGENERATOR_TIME = 0.05;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)) {
            BACKGROUND_COLORGENERATOR_TIME = 0.02;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num9)) {
            BACKGROUND_COLORGENERATOR_TIME = 0.01;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num0)) {
            BACKGROUND_COLORGENERATOR_TIME = 0.005;
          }
        }


        if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)){
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
            EPOCH_STEPS = 1;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
            EPOCH_STEPS = 2;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) {
            EPOCH_STEPS = 3;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) {
            EPOCH_STEPS = 4;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)) {
            EPOCH_STEPS = 5;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)) {
            EPOCH_STEPS = 6;
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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::P) || sf::Keyboard::isKeyPressed(sf::Keyboard::O)){
          PULSATOR_NOT_PULSE = sf::Keyboard::isKeyPressed(sf::Keyboard::O);
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
            PULSATOR_COLOR = 1;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
            PULSATOR_COLOR = 2;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) {
            PULSATOR_COLOR = 3;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) {
            PULSATOR_COLOR = 4;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)) {
            PULSATOR_COLOR = 5;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)) {
            PULSATOR_COLOR = 6;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num7)) {
            PULSATOR_COLOR = 7;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)) {
            PULSATOR_COLOR = 8;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num9)) {
            PULSATOR_COLOR = 9;
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num0)) {
            PULSATOR_COLOR = 0;
          }
        }

        if (regenerateMap) {
          map = PolyMap(map_num, map_x, map_y, map_genType);
          screen_x = map_x/2;
          screen_y = map_y/2;
          screen_zoom = SCREEN_SIZE_Y/map_y;
        }

        window.clear();
        map.draw(screen_x, screen_y, screen_zoom, window);
        // ----------------------------------- pong:

        if (PONG_ENABLED) {
          float res = recorder.getValue();
  				if (res>=0 and res<=1) {
            float resY = (1.0-res)*SCREEN_SIZE_Y;
            if (PONG_BALL_X<SCREEN_SIZE_X/2) {
  					  PONG_YGoal1 = resY;
  					} else {
              PONG_YGoal2 = resY;
            }
  				}

          float pLasty1 = PONG_Y1;
          float pLasty2 = PONG_Y2;
          PONG_Y1 = PONG_Y1*0.5+PONG_YGoal1*0.5;
          PONG_Y2 = PONG_Y2*0.5+PONG_YGoal2*0.5;

          PONG_BALL_X+=PONG_BALL_XV;
          PONG_BALL_Y+=PONG_BALL_YV;

          if (PONG_BALL_Y<0) {
            PONG_BALL_YV = std::abs(PONG_BALL_YV);
          }
          if (PONG_BALL_Y>SCREEN_SIZE_Y) {
            PONG_BALL_YV = -std::abs(PONG_BALL_YV);
          }

          if (PONG_BALL_X<20) {
            if (std::abs(PONG_BALL_Y-PONG_Y1)<PONG_RADIUS) {
              PONG_BALL_XV = std::abs(PONG_BALL_XV);
              PONG_BALL_YV = std::max(-20.0f,std::min(20.0f,(PONG_Y1-pLasty1+PONG_BALL_YV)));
            }else{
              PONG_BALL_X = SCREEN_SIZE_X/2;
              PONG_BALL_Y = SCREEN_SIZE_Y/2;
              PONG_BALL_YV*=0.5;
            }
          }
          if (PONG_BALL_X>SCREEN_SIZE_X-20) {
            if (std::abs(PONG_BALL_Y-PONG_Y2)<PONG_RADIUS) {
              PONG_BALL_XV = -std::abs(PONG_BALL_XV);
              PONG_BALL_YV = std::max(-20.0f,std::min(20.0f,(PONG_Y2-pLasty2+PONG_BALL_YV)));
            }else{
              PONG_BALL_X = SCREEN_SIZE_X/2;
              PONG_BALL_Y = SCREEN_SIZE_Y/2;
              PONG_BALL_YV*=0.5;
            }
          }

          DrawDot(PONG_BALL_X, PONG_BALL_Y, window, sf::Color(255,255,255));
          {
            float PONG_BALL_X_MAP = (PONG_BALL_X - SCREEN_SIZE_X*0.5) / screen_zoom + screen_x;
            float PONG_BALL_Y_MAP = (PONG_BALL_Y - SCREEN_SIZE_Y*0.5) / screen_zoom + screen_y;
            PolyMapCell* select = map.getCell(PONG_BALL_X_MAP, PONG_BALL_Y_MAP, &(map.cells[0]));
            select->state(map.epoch).setPulse(PULSATOR_COLOR);
          }
          DrawRect(0, PONG_Y1-PONG_RADIUS, 20, 2*PONG_RADIUS, window, sf::Color(255,255,255));
          DrawRect(SCREEN_SIZE_X-20, PONG_Y2-PONG_RADIUS, 20, 2*PONG_RADIUS, window, sf::Color(255,255,255));
        }


        {//-------- music board
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Delete)){
            board.clear();
          }

          if (sf::Keyboard::isKeyPressed(sf::Keyboard::Insert)){
            if (MOUSE_LEFT_DOWN) {
              board.set(true, MOUSE_X_MAP*SCREEN_SIZE_X/map_x,MOUSE_Y_MAP*SCREEN_SIZE_Y/map_y);
            }
            if (MOUSE_RIGHT_DOWN) {
              board.set(false, MOUSE_X_MAP*SCREEN_SIZE_X/map_x,MOUSE_Y_MAP*SCREEN_SIZE_Y/map_y);
            }
          }

          if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
            float res = (recorder.getValue()+0.4)*0.6;
            if (res>=0 and res<=1) {
              board.setCurrentColumn(true,(1.0-res)*SCREEN_SIZE_Y,true);
            }
          }
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::F2)) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)){
              board.resetFrequencies();
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)){
              board.setFrequencies(std::vector<double>{30,36,40,45,54});
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)){
              board.setFrequencies(std::vector<double>{24,27,30,36,40});
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)){
              board.setFrequencies(std::vector<double>{24,27,32,36,42});
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)){
              board.setFrequencies(std::vector<double>{15,18,20,24,27});
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)){
              board.setFrequencies(std::vector<double>{24,27,32,36,40});
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num7)){
              board.setFrequencies(std::vector<double>{32.70,36.71,41.20,43.65,49.00,55.00,61.74});
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)){
              board.setFrequencies(std::vector<double>{32.70,36.71,38.89,43.65,49.00,51.91,58.27});
            }
          }

          if (sf::Keyboard::isKeyPressed(sf::Keyboard::F3)) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)){
              board.setOscillator(Instrument::oscillator_sin);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)){
              board.setOscillator(Instrument::oscillator_tria);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)){
              board.setOscillator(Instrument::oscillator_mod);
            }
          }

          if (sf::Keyboard::isKeyPressed(sf::Keyboard::F4)) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)){
              board.resize(16, 32);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)){
              board.resize(32, 32);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)){
              board.resize(64, 32);
            }
          }
        }


        DrawDot(MOUSE_X, MOUSE_Y, window, sf::Color(255*MOUSE_RIGHT_DOWN,255*MOUSE_LEFT_DOWN,255));

        if (MOUSE_LEFT_DOWN) {
          PolyMapCell* select = map.getCell(MOUSE_X_MAP, MOUSE_Y_MAP, &(map.cells[0]));
          if (PULSATOR_NOT_PULSE) {
            select->state(map.epoch).setPulsator(PULSATOR_COLOR);
          } else {
            select->state(map.epoch).setPulse(PULSATOR_COLOR);
          }
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
          DrawText(5,150, "[R+1..4] resize map: "+map_text, 20, window, sf::Color(255,255,255));
          DrawText(5,175, "[G+1..4] map generator: "+map_genType_text, 20, window, sf::Color(255,255,255));
          DrawText(5,200, "[B+1..6] background color generator: "+BACKGROUND_COLORGENERATORS[BACKGROUND_COLORGENERATOR], 20, window, sf::Color(255,255,255));
          DrawText(5,225, "[Z+1..6] background color zoom: "+std::to_string(BACKGROUND_COLORGENERATOR_SCALE), 20, window, sf::Color(255,255,255));
          DrawText(5,250, "[T+1..6] background color time: "+std::to_string(BACKGROUND_COLORGENERATOR_TIME), 20, window, sf::Color(255,255,255));
          DrawText(5,275, "[P+1..0] pulsator color: "+std::to_string(PULSATOR_COLOR), 20, window, sf::Color(255,255,255));
          DrawText(5,300, "[X+1..4] epoch steps: "+std::to_string(EPOCH_STEPS), 20, window, sf::Color(255,255,255));
          DrawText(5,325, "[M] enable pong", 20, window, sf::Color(255,255,255));
          DrawText(5,350, "[F1] music board", 20, window, sf::Color(255,255,255));
          if (BACKGROUND_COLORGENERATOR==10) {
            DrawText(5,375, "[F2+1..8] harmonies", 20, window, sf::Color(255,255,255));
            DrawText(5,400, "[F3+1..3] waves", 20, window, sf::Color(255,255,255));
            DrawText(5,425, "[F4+1..3] music board size", 20, window, sf::Color(255,255,255));
            DrawText(5,450, "[Insert] draw on music board", 20, window, sf::Color(255,255,255));
            DrawText(5,475, "[Delete] clear music board", 20, window, sf::Color(255,255,255));
            DrawText(5,500, "[RShift] record for music board", 20, window, sf::Color(255,255,255));
          }
        }

        window.display();
    }
    recorder.stop();

    return 0;
}
