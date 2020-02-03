#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <cassert>
#include <string>

#include <fstream>
#include <streambuf>

#include <functional>

#include <chrono>

#ifndef EVP_SERVER_HPP
#define EVP_SERVER_HPP

namespace evp {

std::vector<std::string> split(const std::string &text, char sep);

class Connection {
public:
   Connection(sf::TcpSocket* socket) : socket_(socket) {
      lastAccess_ = std::chrono::system_clock::now();
      // assume socket was accepted.
      //std::cout << "Connection: " << socket_->getRemoteAddress() << std::endl;
   }
   ~Connection() {
      //std::cout << "~Connection" << std::endl;
      delete(socket_);
   }
   void run() {
      std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
      long t = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAccess_).count();
      if(t>5000) {
	 std::cout << "timeout: cnt " << rcvdCnt << std::endl;
	 socket_->disconnect();
         connected_ = false;
         return;
      }
      char in[2048];
      std::size_t received;
      while(true) {
         auto status = socket_->receive(in, sizeof(in), received);

         switch(status) {
            case sf::Socket::Done: {
               rcvd.push(in);
               rcvdCnt++;
               lastAccess_ = std::chrono::system_clock::now();
               //std::cout << "rcvd: " << rcvdCnt << std::endl;
               break;
            }
            case sf::Socket::NotReady: {
               return;
            }
            case sf::Socket::Partial: {
               std::cout << "Partial" << std::endl;
               return;
            }
            case sf::Socket::Disconnected: {
               connected_ = false;
               return;
            }
            case sf::Socket::Error: {
               std::cout << "Error connected " << connected_ << std::endl;
               return;
            }
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
      //socket_->disconnect();
      //connected_ = false;
   }
private:
   sf::TcpSocket *socket_;
   std::queue<std::string> rcvd;
   bool connected_ = true;
   int rcvdCnt = 0;
   std::chrono::system_clock::time_point lastAccess_;
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
   static std::string HTTP_response(const std::string &txt) {
      return std::string("")
	      +"HTTP/ 1.1 200 OK\n"
	      +"Connection: Keep-Alive\n"
	      +"Content-Type: text/html\n"
              +"Content-Length: " + std::to_string(txt.size()) + "\n"
	      +"\n"
	      +txt;
   }
   
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
   std::string paramString(const std::string &p, const std::string &def) const {
      const auto &it = param.find(p);
      if(it!=param.end()) {
	 return it->second;
      } else {
         return def;
      }
   }
   std::string path;
   std::string pathExt;
   std::map<std::string,std::string> param;
private:
};

class FileServer : public Server {
public:
   class Item{ public: virtual const std::string get(const URL &url) {return "XXX";}; };
   class FileItem : public Item {
   public:
      FileItem(const std::string &filename) : filename_(filename) {}
      virtual const std::string get(const URL &url) {
         std::ifstream t(filename_);
         std::string ret((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
	 return ret;
      }
   private:
      std::string filename_;
   };
   class StringItem : public Item {
   public:
      StringItem(const std::string &s) : str_(s) {}
      virtual const std::string get(const URL &url) { return str_; }
   private:
      std::string str_;
   };

   class FunctionItem : public Item {
   public:
      typedef std::function<std::string(const evp::URL&)> F;
      FunctionItem(const F &f) : f_(f) {}
      virtual const std::string get(const URL &url) {
	 return f_(url);
      }
   private:
      F f_;
   };

   virtual void handleRequest(std::string &ret, const std::string &url);
   void registerFile(const std::string &url, const std::string &filename);
   void registerString(const std::string &url, const std::string &data);
   void registerFunction(const std::string &url, const FunctionItem::F f);
private:
   std::map<std::string, Item*> files_;
};

class RoomServer;

class User {
public:
   User(const std::string &name) : name_(name) {}
   void setId(const std::string &id) {id_=id;}
   std::string name() const {return name_;}
   std::string id() const {return id_;}
private:
   std::string name_;
   std::string id_;
};

class UserData {
public:
   virtual void dummy() {} // required for polymorphism
private:
};

class Room {
public:
   Room(const std::string &name, RoomServer* server);
   const std::string &name() {return name_;}
   virtual void draw(sf::RenderTarget &target) {}
   virtual UserData* newUserData(const User* u) { return new UserData();}
   void onUserNew(User* const u) {
      user2data_[u] = newUserData(u);
   }
   void onUserLeave(User* const u) {
      user2data_.erase(u);
   }
   typedef std::function<void(evp::User*,evp::UserData*)> UserVisitF;
   void visitUsers(UserVisitF f) {
      for(const auto it : user2data_) {
         f(it.first, it.second);
      }
   }
private:
   const std::string name_;
   RoomServer* const server_;

   std::map<User*,UserData*> user2data_;
};

class RoomServer : public FileServer {
public:
   RoomServer();
   void addRoom(Room* r) { rooms_[r->name()] = r; }
   Room* room(const std::string &name) {
      const auto it = rooms_.find(name);
      if(it!=rooms_.end()) {return it->second;} else {return NULL;}
   };
   void setActive(const std::string &name) {
      Room* r = room(name);
      if(r) {
         active_ = r;
      } else {
         std::cout << "Error: could not find room " << r->name() << std::endl;
      }
   }
   
   void draw(sf::RenderTarget &target) {
      if(active_) {
         active_->draw(target);
      } else {
	 std::cout << "No active room to draw!" << std::endl;
      }
   }
private:
   // Room Part:
   std::map<std::string,Room*> rooms_;
   Room* active_;

   void onUserNew(User* const u) {
      for(const auto it : rooms_) {it.second->onUserNew(u);}
   }
   void onUserLeave(User* const u) {
      for(const auto it : rooms_) {it.second->onUserLeave(u);}
   }

   // User Part:
   void login(const std::string &inName, std::string &outId, bool &outSuccess) {
      const auto &it = name2user_.find(inName);
      if(it == name2user_.end()) {
         // user does not exist yet
	 outSuccess = true;
         User* u = new User(inName);
	 outId = newId();
	 u->setId(outId);
	 name2user_[inName] = u;
	 id2user_[outId] = u;

	 onUserNew(u);
      } else {
         // user exists
	 outSuccess = false;
      }
   }
   std::string newId() {
      return std::to_string(++userIdCnt);
   }
   User* id2user(const std::string &id) {
      const auto &it = id2user_.find(id);
      if(it==id2user_.end()) {
         return NULL;
      } else {
         return it->second;
      }
   }
   
   int userIdCnt = 0;
   std::map<std::string, User*> name2user_;
   std::map<std::string, User*> id2user_;
};

} // namespace evp
#endif //EVP_SERVER_HPP
