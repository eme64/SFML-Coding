#include "evp_networking.hpp"
#include <fstream>


#include "FastNoise/FastNoise.h"
//https://github.com/Auburns/FastNoise/wiki

//https://github.com/JCash/voronoi
#define JC_VORONOI_IMPLEMENTATION
// If you wish to use doubles
//#define JCV_REAL_TYPE double
//#define JCV_FABS fabs
//#define JCV_ATAN2 atan2
#include "jc_voronoi.h"

/* TODO:

account

game
-> lobby
-> player (account or ai) -> id, color
-> teams

*/

// ################### STATIC VARIABLES ############
// ################### STATIC VARIABLES ############
// ################### STATIC VARIABLES ############
// --------- MOUSE
bool MOUSE_LEFT_HIT = false;
bool MOUSE_LEFT_DOWN = false;

bool MOUSE_RIGHT_HIT = false;
bool MOUSE_RIGHT_DOWN = false;

float MOUSE_X;
float MOUSE_Y;

// ################### VORONOI ############
// ################### VORONOI ############
// ################### VORONOI ############
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

void voronoi_free(jcv_diagram &diagram)
{
  jcv_diagram_free( &diagram );
}

// ################### Poly Map ############
// ################### Poly Map ############
// ################### Poly Map ############
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
  std::vector<uint16_t> neighbors;

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
  uint16_t num_cells;
  std::vector<PolyMapCell> cells;

  std::vector<sf::Vertex> mesh;

  char* networkData = NULL;
  size_t networkData_size = 0;

  PolyMap(uint16_t n_cells, float s_x, float s_y)
  {
    num_cells = n_cells;
    spread_x = s_x;
    spread_y = s_y;

    // --------------------------- Prepare Points
    jcv_point* points = NULL;
    points = (jcv_point*)malloc( sizeof(jcv_point) * (size_t)num_cells);

    for(uint16_t i = 0; i<num_cells; i++)
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
    for(uint16_t i = 0; i<num_cells; i++)
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

    generateNetworkData();
  }
  PolyMap(char* netwData, size_t data_size)
  {
    // create stream:
    std::vector<char> v;
    v.resize(data_size);
    std::memcpy(v.data(), netwData, data_size);

    NUDPReadPacket p;
    p.setFromVector(v);

    // start reading:
    p >> spread_x >> spread_y;
    p >> num_cells;

    cells.resize(num_cells);

    for(uint16_t i = 0; i< num_cells; i++)
    {
      // per cell:
      p >> cells[i].pos.x >> cells[i].pos.y;

      char cr; char cg; char cb;
      p >> cr >> cg >> cb;
      cells[i].color.r = cr;
      cells[i].color.g = cg;
      cells[i].color.b = cb;

      char flags;
      p >> flags;
      cells[i].isLand = (flags & 0x1);
      cells[i].isWater = (flags & 0x2);

      uint16_t size_corners;
      p >> size_corners;
      cells[i].corners.resize(size_corners);
      for(uint16_t c=0; c<size_corners; c++)
      {
        p >> cells[i].corners[c].x >> cells[i].corners[c].y;
      }

      uint16_t size_neighbors;
      p >> size_neighbors;
      cells[i].neighbors.resize(size_neighbors);
      for(uint16_t c=0; c<size_neighbors; c++)
      {
        p >> cells[i].neighbors[c];
      }
    }
    create_mesh();
    std::cout << "done constructing" << std::endl;
  }

  ~PolyMap()
  {
    if (networkData != NULL) {
      std::free(networkData);
    }
  }
  void generateNetworkData()
  {
    NUDPWritePacket p;
    // general info
    p << spread_x << spread_y;
    p << num_cells;

    for(uint16_t i = 0; i< num_cells; i++)
    {
      // per cell:
      p << cells[i].pos.x << cells[i].pos.y;

      char cr = cells[i].color.r;
      char cg = cells[i].color.g;
      char cb = cells[i].color.b;
      p << cr << cg << cb;

      char flags = ((char)cells[i].isLand) || ((char)cells[i].isWater << 1);
      p << flags;

      uint16_t size_corners = cells[i].corners.size();
      p << size_corners;
      for(uint16_t c=0; c<size_corners; c++)
      {
        p << cells[i].corners[c].x << cells[i].corners[c].y;
      }

      uint16_t size_neighbors = cells[i].neighbors.size();
      p << size_neighbors;
      for(uint16_t c=0; c<size_neighbors; c++)
      {
        p << cells[i].neighbors[c];
      }

    }

    networkData = (char*)std::malloc(p.index);
    std::memcpy(networkData, p.data.data(), p.index);
    networkData_size = p.index;
  }

  void create_mesh()
  {
    mesh.clear();

    for(uint16_t i = 0; i<num_cells; i++)
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

  uint16_t getCell(float x, float y, uint16_t guess_cell)
  {
    size_t number_iterations = 0;
    // supposed to speed up, because could be close:
    uint16_t res = guess_cell;

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
        uint16_t n = cells[res].neighbors[i];
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

  uint16_t getCell(float x, float y)
  {
    uint16_t res = 0;
    float squaredist = spread_x*spread_x + spread_y*spread_y; // max dist on map !

    for(uint16_t i = 0; i<num_cells; i++)
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

  void bfs(uint16_t root, std::vector<uint16_t> &parents)
  {
    if(! cells[root].isLand)
    {
      // root is not reachable! -> make that clear in parents!
      parents[root] = root;

      size_t nn = cells[root].neighbors.size();
      for(size_t nec = 0; nec<nn; nec++)
      {
        uint16_t ne = cells[root].neighbors[nec];
        parents[ne] = ne;
      }
      return; // early abort
    }

    // creates a rooted tree from a bfs traversal
    bool marked[num_cells] = {false};
    std::queue<uint16_t> q;

    parents[root] = root;
    marked[root] = true;
    q.push(root);

    while(!q.empty())
    {
      uint16_t c = q.front();
      q.pop();

      marked[c] = true;

      //push all children
      size_t nn = cells[c].neighbors.size();
      for(size_t nec = 0; nec<nn; nec++)
      {
        uint16_t ne = cells[c].neighbors[nec];
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

  bool calculatePath(const uint16_t start,const uint16_t goal, std::vector<uint16_t> &path,const size_t STEP_BOUND = 1000)
  {
    path.clear();
    std::vector<uint16_t> parents(num_cells);
    bfs(goal, parents);

    size_t step_counter = 0;
    uint16_t current = start;
    while (current != goal && step_counter < STEP_BOUND) {
      uint16_t next = parents[current];

      path.push_back(next);
      current = next;
      step_counter++;
    }

    if(! (step_counter< STEP_BOUND))
    {
      path.clear();
      path.push_back(start);
    }

    return (step_counter< STEP_BOUND);
  }

  void draw(float x, float y, float zoom, sf::RenderWindow &window)
  {
    sf::Transform t;

    t.translate(SCREEN_SIZE_X*0.5, SCREEN_SIZE_Y*0.5);
    t.scale(zoom, zoom);
    t.translate(-x, -y);

    window.draw(&mesh[0], mesh.size(), sf::Triangles, t);

    return;

    for(uint16_t i = 0; i<num_cells; i++)
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

// ############################## ACCOUNTS ##############################
// ############################## ACCOUNTS ##############################
// ############################## ACCOUNTS ##############################
struct Account
{
  static uint16_t last_id;
  uint16_t id;
  std::string name;

  // if linked up:
  uint16_t NSC_id = 0; // 0 = logged out.

  Account(std::string name): name(name)
  {
    id = ++Account::last_id;
  }
};
uint16_t Account::last_id = 0;

struct AccountHandler_Server
{
  static uint16_t id_last;

  std::map<std::string, Account*> map_accName;
  std::map<uint16_t, Account*> map_accId; // account id
  std::map<uint16_t, Account*> map_nscId; // NServerClient id

  Account* get_name(std::string n)
  {
    std::map<std::string, Account*>::iterator it;
    it = map_accName.find(n);
    if(it != map_accName.end()){
      // account exists:
      return it->second;
    }else{
      return NULL;
    }
  }

  Account* get_id(uint16_t id)
  {
    std::map<uint16_t, Account*>::iterator it;
    it = map_accId.find(id);
    if(it != map_accId.end()){
      // account exists:
      return it->second;
    }else{
      return NULL;
    }
  }

  Account* get_nscId(uint16_t nscId)
  {
    std::map<uint16_t, Account*>::iterator it;
    it = map_nscId.find(nscId);
    if(it != map_nscId.end()){
      // account exists:
      return it->second;
    }else{
      return NULL;
    }
  }

  void updateServer()
  {
    // test if all NServerClient are still valid:
    std::vector<uint16_t> deletees;
    std::map<uint16_t, Account*>::iterator it;
    for (it=map_nscId.begin(); it!=map_nscId.end(); ++it)
  	{
  		Account* acc = it->second;
    	if(NET_SERVER->getClient(acc->NSC_id) != NULL){
    		// active still.
    	}else{
    		// inactive -> delete:
        deletees.push_back(acc->NSC_id);
        acc->NSC_id = 0;

        // say in chat:
        std::string line = "%255,0,0%[" + acc->name + "] timed out";

        NET_SERVER->chat->push(line);
        NET_SERVER->broadcastChat(line);
    	}
    }

    // delete some:
    for (size_t i = 0; i < deletees.size(); i++) {
      map_nscId.erase(deletees[i]);
    }
  }

  void logout(uint16_t nscId)
  {
    Account* acc = get_nscId(nscId);

    if(acc == NULL){
    	// not logged in:
      NServerClient* c = NET_SERVER->getClient(nscId);
      if (c != NULL) {

        std::string line = "%255,0,0%[logout] %% you were not logged in!";
        c->np_send.enqueueChat(line);
      }else{
        // who was that?
        std::cout << "logout: was not logged in and not available !" << std::endl;
      }
    }else{
      std::string line = "%255,0,0%[" + acc->name + "] logged out";

      NET_SERVER->chat->push(line);
      NET_SERVER->broadcastChat(line);

      map_nscId.erase(acc->NSC_id);
      acc->NSC_id = 0;
    }
  }

  int login(uint16_t nscId, std::string userName)
  /* output
  0 -> successful
  1 -> you already logged in
  2 -> other logged in with same name
  */
  {
    // test if you already logged in:
    if(get_nscId(nscId) != NULL){
    	// you already logged in
      return 1;
    }

    // test if name already has an account:
    Account* acc = get_name(userName);
    if (acc != NULL) {
      // account exists:

      // test if other already logged in with name:
      if (acc->NSC_id == 0) {
        // not yet -> log me in!

        acc->NSC_id = nscId;
        map_nscId.insert(
          std::pair<uint16_t, Account*>
          (acc->NSC_id,acc)
        );

        return 0;
      }else{
        // other logged in with same name
        return 2;
      }
    }

    // not logged in, account name not taken -> create:
    acc = new Account(userName);
    acc->NSC_id = nscId;

    // put in all maps:
    map_accName.insert(
      std::pair<std::string, Account*>
      (acc->name,acc)
    );

    map_accId.insert(
      std::pair<uint16_t, Account*>
      (acc->id,acc)
    );

    map_nscId.insert(
      std::pair<uint16_t, Account*>
      (acc->NSC_id,acc)
    );

    return 0;
  }
};

// ################### GAME OBJECTS ############
// ################### GAME OBJECTS ############
// ################### GAME OBJECTS ############
struct GameObjectContext
{
  // this can be defined by user:
  // GameObjects will get it
  // info about map, screen, ...
  // it will be passed by pointer -> as (void *)
  PolyMap *map;

  float screen_x;
  float screen_y;
  float screen_zoom;

  sf::Int32 time;
  float dt; // in secs

  AccountHandler_Server* accountHandler_s;
};

const uint16_t GameObject_TYPE_BASIC = 1;
struct GameObject_Basic:public GameObject
{
  float x; float y;

  // navigation:
  uint16_t cell = 0;
  std::vector<uint16_t> path;
  uint16_t path_it = 0;

  // for client:
  float server_last_x; float server_last_y;
  sf::Int32 server_last;
  float server_dx; float server_dy;

  GameObject_Basic(size_t my_id)
  {
    type = GameObject_TYPE_BASIC;
    id = my_id;
  }

  void toPacket(NUDPWritePacket& p)
  {
    p << id << type;
    p << x << y;
  }
  void fromPacket(NUDPReadPacket& p)
  {
    // id and type already read.
    //p >> x >> y;
    float slx = server_last_x;
    float sly = server_last_y;
    sf::Int32 sl = server_last;

    p >> server_last_x >> server_last_y;

    server_last = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

    float s_dt = ((float)(server_last - sl))/1000.0;
    server_dx = (server_last_x - slx)/s_dt;
    server_dy = (server_last_y - sly)/s_dt;

    if(std::abs(server_dx) > 100 || std::abs(server_dy) > 100)
    {
      server_dx = 0;
      server_dy = 0;
      x = server_last_x;
      y = server_last_y;
    }
  }

  void draw(void* context, sf::RenderWindow &window)
  {
    GameObjectContext* ctxt = (GameObjectContext*)context;

    DrawDot((x - ctxt->screen_x)*ctxt->screen_zoom + SCREEN_SIZE_X*0.5, (y - ctxt->screen_y)*ctxt->screen_zoom + SCREEN_SIZE_Y*0.5, window);
  }

  void updateServer(void* context)
  {
    // server only!
    GameObjectContext* ctxt = (GameObjectContext*)context;

    cell = ctxt->map->getCell(x,y, cell);

    if (! ctxt->map->cells[cell].isLand) {
      // something went bad, must relocate...
      cell = ctxt->map->num_cells * ((float) rand() / (RAND_MAX));
      x = ctxt->map->cells[cell].pos.x;
      y = ctxt->map->cells[cell].pos.y;

      killServer();
    }

    if (path.size() == 0 || path_it >= path.size()) {
      uint16_t goal = ctxt->map->num_cells * ((float) rand() / (RAND_MAX));
      ctxt->map->calculatePath(cell, goal, path);
      path_it = 0;
    }

    uint16_t next_cell = path[path_it];
    float dx = ctxt->map->cells[next_cell].pos.x - x;
    float dy = ctxt->map->cells[next_cell].pos.y - y;
    float d = std::sqrt(dx*dx + dy*dy);
    dx/= d;
    dy/= d;

    x+= dx*60.0*ctxt->dt;
    y+= dy*60.0*ctxt->dt;

    if (cell == next_cell) {
      path_it++;
    }
  }

  void updateClient(void* context)
  {
    // client only: nothing for now
    GameObjectContext* ctxt = (GameObjectContext*)context;

    float now_sl_dt = ((float)(ctxt->time - server_last))/1000.0;

    x = 0.9 * x + 0.1*(server_last_x + now_sl_dt*server_dx);
    y = 0.9 * y + 0.1*(server_last_y + now_sl_dt*server_dy);
  }
};

GameObject* new_GameObject_implementation(size_t id, uint16_t type)
{
  switch (type) {
    case 0: // test objects
    {
      GameObject_Basic* obj = new GameObject_Basic(id);
      obj->x = ((float) rand() / (RAND_MAX)) * 400 + 200;
      obj->y = ((float) rand() / (RAND_MAX)) * 400 + 100;
      return obj;
    }
    case GameObject_TYPE_BASIC:
    {
      return new GameObject_Basic(id);
    }
    default:
    {
      std::cout << "(will crash) Object type not found: " << type << std::endl;
      return NULL;
    }
  }
}

void NPacketCommandHandler_implementation(NServerClient* client, NPacket &np)
{
  std::cout << "just got a command!" << std::endl;
}



// ############################## MAIN ##############################
// ############################## MAIN ##############################
// ############################## MAIN ##############################
int main(int argc, char** argv)
{
    // necessary initialization for GameObjects
    new_GameObject = new_GameObject_implementation;
    NPacketCommandHandler = NPacketCommandHandler_implementation;

    // ----- network config:
    unsigned short net_conf_port = 54000;
    sf::IpAddress net_conf_ip = "localhost";

    std::ifstream net_conf_file("networking.conf");
    std::vector<std::string> lines;
    for(std::string l; std::getline(net_conf_file, l);)
    {
      lines.push_back(l);
    }

    if(lines.size() >1)
    {
      net_conf_ip = lines[0];
      net_conf_port = (uint16_t)str2int(lines[1]);
    }

    // ----------------------------- Handle Commandline Input
    NET_SERVER_MODE = false;
    if(argc > 1)
    {
      std::string serv = "server";
      std::string arg0 = argv[1];
      if(!arg0.compare(serv))
      {
        std::cout << "Server Mode!" << std::endl;
        NET_SERVER_MODE = true;
      }
    }

    if(!NET_SERVER_MODE)
    {
      std::cout << " [INPUT HELP] use 'server' as argument to start a server!" << std::endl;
    }

    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "first attempts", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);

    // ------------------------------------ FONT
    if (!FONT.loadFromFile("arial.ttf"))
    {
        std::cout << "font could not be loaded!" << std::endl;
        return 0;
    }

    // --------------------------- EChat
    EChat chat;

    // --------------------------- Map
    PolyMap* map = NULL;

    // --------------------------- Accounts
    AccountHandler_Server* accountHandler_s;
    if(NET_SERVER_MODE)
    {
      accountHandler_s = new AccountHandler_Server();
    }else{
      // nothing yet
    }
    // --------------------------- Setup Networking
    if(NET_SERVER_MODE)
    {
      // -------------- SERVER:
      // chat handler:
      auto f_chat = [accountHandler_s](NServerClient* client, const std::string &s)
      {
        std::string line;
        std::vector<std::string> input_parts = split(s, ' ');

        if(client != NULL)
        {
          // client input:

          if(input_parts[0] == "/help")
          {

            std::string txt= "%16%[help]%% use the percent symbol to get all the %0%R%16%G%32%B%rand% colors!";
            client->np_send.enqueueChat(txt);
            return;
          }else if (input_parts[0] == "/login") {

            if (input_parts.size() < 2) {
              // bad input.
              std::string txt= "%0%[login]%% /login <name>";
              client->np_send.enqueueChat(txt);
              return;
            }

            int answer = accountHandler_s->login(client->id, input_parts[1]);
            switch (answer) {
              case 0:
              {
                // success:
                line = "%0,255,0%[" + input_parts[1] + "] logged in";

                NET_SERVER->chat->push(line);
                NET_SERVER->broadcastChat(line);
                return;
                break;
              }
              case 1:
              {
                // you already logged in:
                std::string txt= "%0%[login]%% you are already logged in!";
                client->np_send.enqueueChat(txt);
                return;
                break;
              }
              case 2:
              {
                // other logged in with same name:
                std::string txt= "%0%[login]%% some one else is already logged in with that name!";
                client->np_send.enqueueChat(txt);
                return;
                break;
              }
            }
          }else if (input_parts[0] == "/logout") {
            accountHandler_s->logout(client->id);
            return;
          }

          Account* acc = accountHandler_s->get_nscId(client->id);

          if (acc != NULL) {
            // found:
            line = "%255,255,255%[" + acc->name + "]%% " + s;

            NET_SERVER->chat->push(line);
            NET_SERVER->broadcastChat(line);
            return;
          }else{
            // mot logged in yet:
            line = "%255,255,255%[chat]%% you need to %0,255,0%/login <name> %%before you can chat.";
            client->np_send.enqueueChat(line);
            return;
          }
        }else{
          // server input:
          line = "%8%[server]%% " + s;

          if(input_parts[0] == "/help")
          {
            NET_SERVER->chat->push("%16%[help]%% use the percent symbol to get all the %0%R%16%G%32%B%rand% colors!");
            return;
          }else if(input_parts[0] == "/data"){
            NET_SERVER->chat->push("%16%[data]%% request some data...");

            // test data request:
            auto f_success = [](char* data, size_t size)
            {
              std::cout << "[]data received! size: " << std::to_string(size) << std::endl;

              if(data == NULL){return;}

              for(size_t i = 0; i<size; i++)
              {
                char c = i;

                if(data[i] != c)
                {
                  std::cout << "[]off at " << i << std::endl;
                }
              }
              std::cout << "[]check complete" << std::endl;
              std::free(data);
              data = NULL;
            };

            auto f_failure = [](bool noTimeout, std::string errorMsg)
            {
              std::cout << "[].test file name. failed" << std::endl;
              std::cout << "[]ERROR: " << errorMsg << std::endl;
              if(noTimeout)
              {
                std::cout << "[]not even timeout" << std::endl;
              }else{
                std::cout << "[]timeout" << std::endl;
              }
            };

            std::map<int, NServerClient*>::iterator it;
          	for (it=NET_SERVER->map_id.begin(); it!=NET_SERVER->map_id.end(); ++it)
          	{
          		NServerClient* c = it->second;
              c->np_send.issueDataRequest("testdata", f_success, f_failure);
            }
            return;
          }else if(input_parts[0] == "/cmd"){
            NET_SERVER->chat->push("%16%[cmd]%% sent a cmd...");

            std::vector<char> v;
            v.push_back(0);
            v.push_back(1);
            v.push_back(2);

            std::map<int, NServerClient*>::iterator it;
          	for (it=NET_SERVER->map_id.begin(); it!=NET_SERVER->map_id.end(); ++it)
          	{
          		NServerClient* c = it->second;
              c->np_send.enqueue(NPacket_TAG_COMMAND, v);
            }
            return;
          }
        }

        NET_SERVER->chat->push(line);
        NET_SERVER->broadcastChat(line);
      };

      // data request handler:
      auto f_dataReq = [&map](std::string fileName, NServerClient* client, bool& success, char* &data, size_t &data_size, std::string& errorMsg)
      {
        std::cout << "[RH]DATA REQUEST:" << std::endl;
        if(client!=NULL)
        {
          std::cout << "[RH]client id: " << std::to_string(client->id) << std::endl;
        }else{
          std::cout << "[RH]by server" << std::endl;
        }

        if(fileName == "testdata")
        {
          // set dummy data here:
          data_size = 10000;
          data = (char*)std::malloc(data_size);

          for(size_t i = 0; i<data_size; i++)
          {
            data[i] = i;
          }

          success = true;
        }else if(fileName == "map"){
          // set dummy data here:
          data_size = map->networkData_size;
          data = (char*)std::malloc(map->networkData_size);
          std::memcpy(data, map->networkData, map->networkData_size);

          success = true;
        }else{
          // we will now just send error instead:
          success = false;
          errorMsg = "this is only the dummy dataRequestHandler !";
        }
      };

      // server
      NET_SERVER = new NServer(net_conf_port, &chat, f_chat, f_dataReq);


    }else{
      // ------------------ CLIENT:
      // data request handler:
      auto f_dataReq = [](std::string fileName, NServerClient* client, bool& success, char* &data, size_t &data_size, std::string& errorMsg)
      {
        std::cout << "[RH]DATA REQUEST:" << std::endl;

        success = false;
        errorMsg = "Client does not handle dataRequests !";
      };

      // client:
      NET_CLIENT = new NClient(net_conf_ip, net_conf_port, &chat, f_dataReq);

      /*
      // test data request:
      auto f_success = [](char* data, size_t size)
      {
        std::cout << "[]data received! size: " << std::to_string(size) << std::endl;

        if(data == NULL){return;}

        for(size_t i = 0; i<size; i++)
        {
          char c = i;

          if(data[i] != c)
          {
            std::cout << "[]off at " << i << std::endl;
          }
        }
        std::cout << "[]check complete" << std::endl;
        std::free(data);
        data = NULL;
      };

      auto f_failure = [](bool noTimeout, std::string errorMsg)
      {
        std::cout << "[].test file name. failed" << std::endl;
        std::cout << "[]ERROR: " << errorMsg << std::endl;
        if(noTimeout)
        {
          std::cout << "[]not even timeout" << std::endl;
        }else{
          std::cout << "[]timeout" << std::endl;
        }
      };

      for(int i=0; i<3; i++)
      {
        NET_CLIENT->np_send.issueDataRequest("testdata", f_success, f_failure);
      }
      */
    }
    std::string textInput;

    // ------------------------------------ MAP creation:
    if (NET_SERVER_MODE) {
      // server:
      map = new PolyMap(30000, 1000, 1000);
      //map = new PolyMap(300, 100, 100);
    }else{
      // request map:
      auto f_success = [&map](char* data, size_t size)
      {
        std::cout << "[]map received! size: " << std::to_string(size) << std::endl;

        if(data == NULL){return;}

        map = new PolyMap(data, size);

        std::free(data);
        data = NULL;
      };

      auto f_failure = [](bool noTimeout, std::string errorMsg)
      {
        std::cout << "[]map loading failed:" << std::endl;
        std::cout << "[]ERROR: " << errorMsg << std::endl;
        if(noTimeout)
        {
          std::cout << "[]not even timeout" << std::endl;
        }else{
          std::cout << "[]timeout" << std::endl;
        }
      };

      NET_CLIENT->np_send.issueDataRequest("map", f_success, f_failure);
    }

    // ------------------------------------ SCEEN
    float screen_x = 0;
    float screen_y = 0;
    float screen_zoom = 1.0;

    // ------------------------------------ TIME
    sf::Int32 current_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
    float dt = 0.01;

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
              {
                window.close();
                break;
              }
              case sf::Event::TextEntered:
              {
                //std::cout << uint16_t(event.text.unicode) << std::endl;
                if (event.text.unicode < 128 && chat.isActive  && window.hasFocus())
                {
                  InputHandler::push(static_cast<char>(event.text.unicode));
                  //textInput += static_cast<char>(event.text.unicode);
                  //std::cout << "ASCII character typed: " << static_cast<char>(event.text.unicode) << std::endl;
                }
                break;
              }
              case sf::Event::MouseWheelScrolled:
              {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel && window.hasFocus())
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
              }
              case sf::Event::MouseButtonPressed:
              {
                if(event.mouseButton.button == sf::Mouse::Left && window.hasFocus())
                {
                  MOUSE_LEFT_HIT = true;
                  MOUSE_LEFT_DOWN = true;
                }else if(event.mouseButton.button == sf::Mouse::Right && window.hasFocus())
                {
                  MOUSE_RIGHT_HIT = true;
                  MOUSE_RIGHT_DOWN = true;
                }
                break;
              }
              case sf::Event::MouseButtonReleased:
              {
                if(event.mouseButton.button == sf::Mouse::Left)
                {
                  MOUSE_LEFT_DOWN = false;
                }else if(event.mouseButton.button == sf::Mouse::Right)
                {
                  MOUSE_RIGHT_DOWN = false;
                }
                break;
              }
              // we don't process other types of events
              default:
                break;
          }
      }

      sf::Vector2i mousepos = sf::Mouse::getPosition(window);
      MOUSE_X = mousepos.x;
      MOUSE_Y = mousepos.y;

      if(! chat.isActive && window.hasFocus())
      {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::T)){
          chat.isActive = true;
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
      }

      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
        window.close();
      }

      if (sf::Keyboard::isKeyPressed(sf::Keyboard::F1) && window.hasFocus()){
        window.close();
      }

      // MAIN LOOP:
      window.clear();

      GameObjectContext context = GameObjectContext{
        map,
        screen_x, screen_y, screen_zoom,
        current_time, dt,
      };

      if(NET_SERVER_MODE)
      {
        // server
        accountHandler_s->updateServer();
        NET_SERVER->update(&context);
      }else{
        // client
        NET_CLIENT->update(&context);
      }

      // draw GAME
      if(map != NULL)
      {
        map->draw(screen_x, screen_y, screen_zoom, window);
      }

      DrawDot(MOUSE_X, MOUSE_Y, window, sf::Color(255*MOUSE_RIGHT_DOWN,255*MOUSE_LEFT_DOWN,255));

      // draw rest.
      if(NET_SERVER_MODE)
      {
        // server
        NET_SERVER->draw(&context, window);
      }else{
        // client
        NET_CLIENT->draw(&context, window);
      }

      chat.update();

      chat.draw(0,600, window);

      window.display();

      // TIME:
      sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
      float dt = (float)(now_time - current_time)/1000.0;
      current_time = now_time;
    }

    if(map != NULL)
    {
      delete(map);
      map = NULL;
    }

    if(NET_SERVER_MODE)
    {
      // server
      delete(NET_SERVER);
    }else{
      // client
      delete(NET_CLIENT);
    }

    return 0;
}
