
#include "arena.hpp"

#ifndef EVP_LOBBY_HPP
#define EVP_LOBBY_HPP

struct LobbyButton {
  float _x, _y, _dx, _dy;
  LobbyButton(float x, float y, float dx, float dy)
  : _x(x), _y(y), _dx(dx), _dy(dy) {}
};

struct UserDataLobby : public UserData {
  float _x, _y;
  UserDataLobby() : _x(50), _y(50) {};
};

struct RoomLobby : public Room {
  std::vector<LobbyButton*> _buttons;
  Camera _camera;
  RoomLobby(std::string name_) : Room(name_) {
    _size_x = 100;
    _size_y = 100;
    _buttons.push_back(new LobbyButton(45,45,10,10));
    _buttons.push_back(new LobbyButton(70,70,10,10));
    _buttons.push_back(new LobbyButton(20,30,10,10));
  }
  virtual UserData* new_user_data(User* u) {
    return new UserDataLobby();
  };
  virtual void set_ctrl(User* u) {
    UserDataLobby* data = user_data<UserDataLobby*>(u);
    Control* c = u->ctrl();
    c->clear();
    c->set_up([data](){
      data->_y -= 0.1;
    });
    c->set_down([data](){
      data->_y += 0.1;
    });
    c->set_left([data](){
      data->_x -= 0.1;
    });
    c->set_right([data](){
      data->_x += 0.1;
    });
  };

  virtual void update() {}

  virtual void draw(sf::RenderWindow &window) {
    // compute camera
    sf::Vector2u size = window.getSize();
    Camera c(size.x, size.y);
    c.capture_start();
    visit_users([&](User* u){
      UserDataLobby* data = user_data<UserDataLobby*>(u);
      c.capture_add(data->_x, data->_y);
    });
    c.capture_end(10,10,1.5);
    _camera.transition_to(c, 0.1);

    // draw area
    _camera.rect(0,0,_size_x,_size_y, window, sf::Color(0,100,100));
    _camera.rect(1,1,_size_x-2,_size_y-2, window, sf::Color(100,0,0));

    // draw buttons
    for (LobbyButton* b : _buttons) {
      _camera.rect(b->_x, b->_y, b->_dx, b->_dy, window, sf::Color(50,100,150));
    }

    // draw users
    visit_users([&](User* u){
      UserDataLobby* data = user_data<UserDataLobby*>(u);
      _camera.dot(data->_x, data->_y, 1, window, sf::Color(255,0,0));
    });
  }
};

#endif //EVP_LOBBY_HPP
