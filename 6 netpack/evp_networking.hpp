/* TODO

*/

// ############### INCLUDE #############
// ############### INCLUDE #############
// ############### INCLUDE #############
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <climits>

#include <SFML/Network.hpp>
#include <list>
#include <queue>
#include <map>
#include <time.h>


namespace evp
{
  // ################## EXTRA FUNCTIONS ############
  // ################## EXTRA FUNCTIONS ############
  // ################## EXTRA FUNCTIONS ############
  int str2int (std::string &s, int base = 10)
  {
    int i;
    char *end;
    long  l;
    errno = 0;
    l = strtol(s.data(), &end, base);
    if ((errno == ERANGE && l == LONG_MAX) || l > INT_MAX) {
        return 0;
    }
    if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) {
        return 0;
    }
    if (s[0] == '\0' || *end != '\0') {
        return 0;
    }
    i = l;
    return i;
  }

  std::vector<std::string> split(const std::string &text, char sep) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos) {
      tokens.push_back(text.substr(start, end - start));
      start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
  }

  // ################## NETWORKING ############
  // ################## NETWORKING ############
  // ################## NETWORKING ############

  class Connection
  {
  private:
    const bool isServer;
    const unsigned short serverPort;
    sf::UdpSocket socket;

    // --- Server

    // --- Client
    const sf::IpAddress serverIP;
    unsigned short localPort;

  public:
    Connection(unsigned short serverPort) // create Server
    : isServer(true),
      serverPort(serverPort)
    {
      // bind socket:
      if(socket.bind(serverPort) != sf::Socket::Done)
      {
        std::cout << "could not bind socket!" << std::endl;
        return;
      }
      socket.setBlocking(false);
      std::cout << "Server Created." << std::endl;
    }

    Connection(unsigned short serverPort, sf::IpAddress serverIP) // connect client to server
    : isServer(false),
      serverPort(serverPort),
      serverIP(serverIP)
    {
      if(socket.bind(sf::Socket::AnyPort) != sf::Socket::Done)
      {
        std::cout << "could not bind socket!" << std::endl;
        return;
      }
      socket.setBlocking(false);
      localPort = socket.getLocalPort();

      send();
    }

    void send(){
      sf::Packet p;
      int i = 101;
      p << i;

      if (socket.send(p, serverIP, serverPort) != sf::Socket::Done)
      {
          std::cout << "Error on send !" << std::endl;
      }
    }

    void update(){
      // update connection

      // listen:
      bool repeat = false;
      do
      {
        sf::Packet p;
        sf::IpAddress remoteIp;
        unsigned short remotePort;
        auto status = socket.receive(p, remoteIp, remotePort);

        repeat = false;
        if(status == sf::Socket::Done)
        {
          repeat = true;
          int i;
          p >> i;
          std::cout << remoteIp << ":" << remotePort << "->" << i << std::endl;
        }
      }
      while(repeat);
    }
  };
}
