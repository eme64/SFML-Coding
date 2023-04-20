#include <functional>
#include <cassert>

#ifndef EVP_ARENA_HPP
#define EVP_ARENA_HPP

void nothing () {}

struct Control {
  typedef std::function<void()> Callback;
  Callback _up, _down, _left, _right;
  Control() { clear(); }
  void clear() {
    _up    = nothing;
    _down  = nothing;
    _left  = nothing;
    _right = nothing;
  }
  void up()    {_up();}
  void down()  {_down();}
  void left()  {_left();}
  void right() {_right();}
  void set_up(Callback f)    {_up = f;}
  void set_down(Callback f)  {_down = f;}
  void set_left(Callback f)  {_left = f;}
  void set_right(Callback f) {_right = f;}
};

struct Camera {
  float _target_dx = 0, _target_dy = 0;
  float _x = 0, _y = 0, _scale = 0;
  float _min_x, _min_y, _max_x, _max_y;
  Camera(float target_dx, float target_dy)
  : _target_dx(target_dx), _target_dy(target_dy) {
  }
  Camera() {}
  float x(float x_) {return _x + _scale*x_;}
  float y(float y_) {return _y + _scale*y_;}
  float dx(float dx_) {return _scale*dx_;}
  float dy(float dy_) {return _scale*dy_;}

  void rect(float x_, float y_, float dx_, float dy_, sf::RenderWindow &window, sf::Color color) {
    DrawRect(x(x_), y(y_), dx(dx_), dy(dy_), window, color);
  }
  void dot(float x_, float y_, float r_, sf::RenderWindow &window, sf::Color color) {
    DrawDot(x(x_), y(y_), dx(r_), window, color);
  }
  void capture_start() {
    _min_x = 1e10;
    _min_y = 1e10;
    _max_x = -1e10;
    _max_y = -1e10;
  }
  void capture_add(float x_, float y_) {
    _min_x = std::min(_min_x, x_);
    _min_y = std::min(_min_y, y_);
    _max_x = std::max(_max_x, x_);
    _max_y = std::max(_max_y, y_);
  }
  void capture_end(float min_dx, float min_dy, float margin_factor) {
    float ddx = _max_x - _min_x;
    float ddy = _max_y - _min_y;
    assert(ddx >= 0 && ddy >=0 && "had at least one capture_add");
    float ccx = (_max_x + _min_x) * 0.5;
    float ccy = (_max_y + _min_y) * 0.5;
    // If captured points are too close, then take bigger view
    ddx = std::max(ddx, min_dx);
    ddy = std::max(ddy, min_dy);
    // Figure out scale
    float r1 = _target_dx / _target_dy;
    float r2 = ddx / ddy;
    if (r1 < r2) {
      // x is bottleneck
      _scale = _target_dx / ddx;
    } else {
      // y is bottleneck
      _scale = _target_dy / ddy;
    }
    _scale /= margin_factor;
    // Figure out offset, given scale
    // _target_dx * 0.5 = x(ccx) = _x + _scale*ccx
    _x = _target_dx*0.5 - _scale*ccx;
    _y = _target_dy*0.5 - _scale*ccy;
  }
  void transition_to(Camera& c, float factor) {
    float f1 = 1.0-factor;
    float f2 = factor;
    if (_scale == 0) {
      f1 = 0; f2 = 1;
    }
    //_x = c._x;
    //_y = c._y;
    //_scale = c._scale;
    _x = f1*_x + f2*c._x;
    _y = f1*_y + f2*c._y;
    _scale = f1*_scale + f2*c._scale;
  }
};

struct User {
  int _uid;
  Control* _ctrl;
  User(int uid, Control* c) : _uid(uid), _ctrl(c) {}
  int uid() const { return _uid; }
  Control* ctrl() const { return _ctrl; }
};

struct UserData {
  virtual ~UserData() {};
};

struct Room {
  std::string _name;
  std::map<User*, UserData*> _user_data;

  float _size_x, _size_y;

  Room(std::string name_) : _name(name_) {}

  std::string name() const { return _name; }
  virtual UserData* new_user_data(User* u) = 0;
  virtual void set_ctrl(User* u) = 0;
  void add_user(User* u) {
    UserData* data = new_user_data(u);
    _user_data[u] = data;
  }

  typedef std::function<void(User*)> UserVisitF;
  void visit_users(UserVisitF f) {
    for(const auto it : _user_data) {
      f(it.first);
    }
  }

  virtual void update() {}

  virtual void draw(sf::RenderWindow &window) {
    sf::Vector2u size = window.getSize();
    DrawText(size.x/2 - 50, size.y/2-10, "Room: " + name(), 20, window, sf::Color(255,255,255));
  }

  template<typename T>
  T user_data(User* u) {
   T data = dynamic_cast<T>(_user_data[u]);
   assert(data != nullptr && "user data non null");
   return data;
  }
};

struct Arena {
  int size_x, size_y, scale;
  int _uid = 0;

  std::map<std::string, Room*> _rooms; // name to room
  Room* _active = nullptr;
  std::map<int, User*> _users; // uid to user
  std::vector<Control*> _unused_controls;

  Arena(int x, int y, int scale) : size_x(x), size_y(y), scale(scale) {
  }

  void add_room(Room* r) {
    _rooms[r->name()] = r;
    if (_active == nullptr) {
      _active = r;
    }
  }
  User* new_user() {
    if (_unused_controls.size() == 0) { return nullptr; }
    Control* ctrl = _unused_controls.back();
    _unused_controls.pop_back();
    assert(ctrl != nullptr && "ctrl not null");
    int uid = ++_uid;
    User* u = new User(uid, ctrl);
    _users[u->uid()] = u;
    for (const auto it : _rooms) {it.second->add_user(u);}
    assert(_active != nullptr && "active room not null");
    _active->set_ctrl(u);
    return u;
  }
  Control* new_ctrl() {
    Control* c = new Control();
    _unused_controls.push_back(c);
    return c;
  }

  void update() {
    _active->update();
  }

  void draw(sf::RenderWindow &window) {
    _active->draw(window);
  }
};

#endif //EVP_ARENA_HPP
