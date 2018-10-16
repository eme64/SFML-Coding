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
#include <cassert>

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
    sf::Int32 lastRcvTime; // ms
    sf::Int32 lastSendTime; // ms

  public:
    const unsigned short remotePort;
    const sf::IpAddress remoteIP;

    Connection(unsigned short remotePort, sf::IpAddress remoteIP)
    : remotePort(remotePort),
      remoteIP(remoteIP)
    {
      lastRcvTime = 0;
      lastSendTime = 0;
    }

    void setLastRcvTime(sf::Int32 timeNow)
    {
      lastRcvTime = timeNow;
    }

    void setLastSendTime(sf::Int32 timeNow)
    {
      lastSendTime = timeNow;
    }

    bool timeoutLastRcvTime(sf::Int32 timeNow, sf::Int32 timeout = 10000)
    {
      return lastRcvTime + timeout < timeNow;
    }

    bool timeoutLastSendTime(sf::Int32 timeNow, sf::Int32 timeout = 2000)
    {
      return lastSendTime + timeout < timeNow;
    }
  };

  class ConnectionIdentifyer
  {
  public:
    const sf::IpAddress remoteIp;
    const unsigned short remotePort;

    ConnectionIdentifyer(unsigned short remotePort, sf::IpAddress remoteIP)
    : remotePort(remotePort),
      remoteIp(remoteIP)
    {
    }

    ConnectionIdentifyer(evp::Connection *con)
    : remotePort(con->remotePort),
      remoteIp(con->remoteIP)
    {
    }
  };

  class cmpConnectionIdentifyer
  {
  public:
    bool operator() (const ConnectionIdentifyer& lhs, const ConnectionIdentifyer& rhs) const
    {
      if(lhs.remotePort == rhs.remotePort)
      {
        return lhs.remoteIp < rhs.remoteIp;
      }else{
        return lhs.remotePort < rhs.remotePort;
      }
    }
  };

  class Connector
  {
  private:
    const bool isServer;
    unsigned short localPort;
    sf::UdpSocket socket;

    std::map<ConnectionIdentifyer, Connection*, cmpConnectionIdentifyer> connections;

    sf::Clock clock;

    sf::Int32 getTime()
    {
      clock.getElapsedTime().asMilliseconds();
    }

  public:
    Connector(unsigned short serverPort) // create Server
    : isServer(true),
      localPort(serverPort)
    {
      // bind socket:
      if(socket.bind(localPort) != sf::Socket::Done)
      {
        std::cout << "could not bind socket!" << std::endl;
        return;
      }
      socket.setBlocking(false);
      std::cout << "Server Created." << std::endl;
    }

    Connector(unsigned short serverPort, sf::IpAddress serverIP) // connect client to server
    : isServer(false)
    {
      if(socket.bind(sf::Socket::AnyPort) != sf::Socket::Done)
      {
        std::cout << "could not bind socket!" << std::endl;
        return;
      }
      socket.setBlocking(false);
      localPort = socket.getLocalPort();

      addConnection(serverPort, serverIP);
      send(getServer());
    }

    evp::Connection* addConnection(unsigned short remotePort, sf::IpAddress remoteIP)
    {
      evp::Connection* con = NULL;
      // try find.
      con = getConnection(remotePort, remoteIP);
      if (con != NULL) {
        return con;
      }

      // create.
      con = new evp::Connection(remotePort, remoteIP);

      // add.
      connections.insert(
        std::pair<ConnectionIdentifyer, Connection*>
        (ConnectionIdentifyer(con), con)
      );

      return con;
    }

    evp::Connection* getConnection(unsigned short remotePort, sf::IpAddress remoteIP){
  		std::map<ConnectionIdentifyer, Connection*>::iterator it;

  		it = connections.find(ConnectionIdentifyer(remotePort, remoteIP));

  		if(it != connections.end()){
  			return it->second;
  		}else{
  			return NULL;
  		}
  	}

    evp::Connection* getServer()
    {
      assert(not isServer);

      evp::Connection* ret = NULL;
      for (auto const& x : connections)
      {
          ret = x.second;
      }
      return ret;
    }

    void send(evp::Connection* con){
      con->setLastSendTime(getTime());

      sf::Packet p;
      sf::Uint8 i = 'A';
      p << i;

      if (socket.send(p, con->remoteIP, con->remotePort) != sf::Socket::Done)
      {
          std::cout << "Error on send !" << std::endl;
      }
    }

    void rcvPacket(sf::Packet &p, evp::Connection* con)
    {
      con->setLastRcvTime(getTime());

      sf::Uint8 i;
      p >> i;
      std::cout << con->remoteIP << ":" << con->remotePort << "->" << (int)i << std::endl;
    }

    void update(){
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
          evp::Connection* con = addConnection(remotePort, remoteIp);
          rcvPacket(p, con);
        }
      }
      while(repeat);

      // check all Connections:
      std::vector<evp::Connection*> delVec;
      for (auto const& x : connections)
      {
          evp::Connection* con = x.second;

          if (con->timeoutLastRcvTime(getTime())) {
            delVec.push_back(con);
            std::cout << "timeout " << con->remoteIP << ":" << con->remotePort << std::endl;
          }
          if (con->timeoutLastSendTime(getTime())) {
            send(con);
            std::cout << "hartbeat to " << con->remoteIP << ":" << con->remotePort << std::endl;
          }
      }
      for (evp::Connection* con : delVec) {
        connections.erase(evp::ConnectionIdentifyer(con));
        delete(con);
      }

    }
  };
}
