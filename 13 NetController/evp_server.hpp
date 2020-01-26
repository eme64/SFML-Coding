#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <SFML/Network.hpp>
#include <cassert>
#include <string>

#include <fstream>
#include <streambuf>

#ifndef EVP_SERVER_HPP
#define EVP_SERVER_HPP

namespace evp {

std::vector<std::string> split(const std::string &text, char sep);

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
	    std::cout << "Error connected " << connected_ << std::endl;
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
      socket_->disconnect();
      connected_ = false;
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

	    std::string ret = "";
	    if(first.size() < 2) {
	       // call handle error: give it first line?
	       // log error
	       handleError(ret, "bad request.\nFirst line:\n" + lines[0]);
	    } else if(first[0] != "GET") {
	       // call handle error: not GET
	       // log error
	       handleError(ret, "request not GET.\nFirst line:\n" + lines[0]);
	    } else {
	       std::string url = first[1].substr(1,-1);

	       // handle GET
	       handleRequest(ret, url);
	    }
	    c->send(ret);
	 }

	 if(!c->isConnected()) {
	    connections.erase(current);
	    delete(c);
	    std::cout << "connections " << connections.size() << std::endl;
	 }
      }
   }

   const static std::string HTTP_text;
   
   // override these below for impl
   virtual void handleError(std::string &ret, const std::string &error);
   virtual void handleRequest(std::string &ret, const std::string &url);
private:
   sf::TcpListener listener;
   unsigned short port;

   sf::TcpSocket* socket_; // used to accept sockets from listener
   std::set<Connection*> connections;
};

class URL {
public:
   URL(const std::string &url) {
      std::vector<std::string> urlParts = evp::split(url,'?');
      path = urlParts[0];
      std::vector<std::string> pathParts = evp::split(path,'.');
      if(pathParts.size()>1) {
         pathExt = pathParts[pathParts.size()-1];
      } else {
         pathExt = "";
      }

      if(urlParts.size()>1) {
         std::vector<std::string> parts = evp::split(urlParts[1],'&');
         for(std::string &s : parts) {
            std::vector<std::string> paramX = evp::split(s,'=');
            if(paramX.size()>1) {
	       param[paramX[0]] = paramX[1];
            } else {
	       param[paramX[0]] = "";
	    }
         }
      }
   }
   std::string path;
   std::string pathExt;
   std::map<std::string,std::string> param;
private:
};

class FileServer : public Server {
public:
   class Item{ public: virtual const std::string get() {return "XXX";}; };
   class File : public Item {
   public:
      File(const std::string &filename) : filename_(filename) {}
      virtual const std::string get() {
         std::ifstream t(filename_);
         std::string ret((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
	 return ret;
      }
   private:
      std::string filename_;
   };
   virtual void handleRequest(std::string &ret, const std::string &url);
   void registerFile(const std::string &url, const std::string &filename);
   void registerString(const std::string &url, const std::string &data);
private:
   std::map<std::string, Item*> files_;
};

} // namespace evp
#endif //EVP_SERVER_HPP
