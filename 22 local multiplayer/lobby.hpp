
#include "arena.hpp"

#ifndef EVP_LOBBY_HPP
#define EVP_LOBBY_HPP

struct UserDataLobby : public UserData {
  float _x, _y;
  UserDataLobby() : _x(50), _y(50) {};
};

struct RoomLobby : public Room {
  RoomLobby(std::string name_) : Room(name_) {
    _size_x = 100;
    _size_y = 100;
  }
  virtual UserData* new_user_data(User* u) {
    return new UserDataLobby();
  };
  virtual void set_ctrl(User* u) {
    UserDataLobby* data = user_data<UserDataLobby*>(u);
    Control* c = u->ctrl();
    c->clear();
    c->set_up([data](){
      data->_y -= 1;
    });
    c->set_down([data](){
      data->_y += 1;
    });
    c->set_left([data](){
      data->_x -= 1;
    });
    c->set_right([data](){
      data->_x += 1;
    });
  };

  virtual void draw(sf::RenderWindow &window) {
    sf::Vector2u size = window.getSize();
    visit_users([&](User* u){
      UserDataLobby* data = user_data<UserDataLobby*>(u);
      DrawDot(data->_x, data->_y, 3, window);
    });
  }
};

#endif //EVP_LOBBY_HPP
