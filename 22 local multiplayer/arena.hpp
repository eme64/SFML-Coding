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

  virtual void update() {
  }

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
