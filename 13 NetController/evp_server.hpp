#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <SFML/Network.hpp>
#include <cassert>
#include <string>

namespace evp {

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

class Connection {
public:
   Connection(sf::TcpSocket* socket) : socket_(socket) {
      // assume socket was accepted.
      std::cout << "Connection: " << socket_->getRemoteAddress() << std::endl;
   }
   ~Connection() {
      std::cout << "~Connection" << std::endl;
      delete(socket_);
   }
   void run() {
      char in[2048];
      std::size_t received;
      auto status = socket_->receive(in, sizeof(in), received);

      switch(status) {
         case sf::Socket::Done: {
	    std::cout << "Done" << std::endl;
	    rcvd.push(in);
	    break;
         }
         case sf::Socket::NotReady: {
	    // ignore
	    break;
         }
         case sf::Socket::Partial: {
	    std::cout << "Partial" << std::endl;
	    break;
         }
         case sf::Socket::Disconnected: {
	    connected_ = false;
	    break;
         }
         case sf::Socket::Error: {
	    std::cout << "Error" << std::endl;
	    break;
         }
      }
   }

   bool isConnected() {return connected_;}
   int rcvdSize() {return rcvd.size();}
   std::string rcvdPop() {
      if(rcvd.size()==0) {return "";}

      std::string res = rcvd.front();
      rcvd.pop();
      return res;
   }

   void send(std::string &s) {
      std::size_t sent;
      if (socket_->send(s.c_str(), s.size(), sent) != sf::Socket::Done) {
         std::cout << "ERROR snd" << std::endl;
	 std::cout << std::endl;
	 std::cout << s << std::endl;
         assert(false);
      }
   }
private:
   sf::TcpSocket *socket_;
   std::queue<std::string> rcvd;
   bool connected_ = true;
};

class Server {
public:
   Server() {
      listener.setBlocking(false);
      if (listener.listen(sf::Socket::AnyPort) != sf::Socket::Done) {
         std::cout << "ERROR port" << std::endl;
         assert(false);
      }
      port = listener.getLocalPort();
      std::cout << "Server: http://localhost:" << port << std::endl;
      
      socket_ = new sf::TcpSocket();
      socket_->setBlocking(false);
   }
   ~Server() {
      delete(socket_);
   }

   void run() {
      while(listener.accept(*socket_) == sf::Socket::Done) {
         Connection* c = new Connection(socket_);
	 socket_ = new sf::TcpSocket(); // handed off socket to connection
         socket_->setBlocking(false);
	 
	 connections.insert(c);

	 std::cout << "connections " << connections.size() << std::endl;
      }

      for(std::set<Connection*>::iterator it = connections.begin(); it!=connections.end(); it++) {
	 std::set<Connection*>::iterator current = it;
         Connection* c = *current;
	 
	 c->run();

	 while(c->rcvdSize() > 0) {
	    std::string s = c->rcvdPop();
            std::vector<std::string> lines(split(s,'\n'));
            std::vector<std::string> first(split(lines[0],' '));
            std::cout << "Request: " << first[0] << " - " << first[1] << std::endl;
            
            // TODO: continue handling requests here.

            std::string resp = "HTTP/ 1.1 200 OK\nContent-Type: text/html\n\n";
            resp+= "Requested:\n";
            resp+= first[1];
	    c->send(resp);
	 }

	 if(!c->isConnected()) {
	    connections.erase(current);
	    delete(c);
	    std::cout << "connections " << connections.size() << std::endl;
	 }
      }
  }
private:
   sf::TcpListener listener;
   unsigned short port;

   sf::TcpSocket* socket_; // used to accept sockets from listener
   std::set<Connection*> connections;
};
} // namespace evp
