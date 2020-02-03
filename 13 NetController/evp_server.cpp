#include "evp_server.hpp"

// --------- SPLIT
std::vector<std::string>
evp::split(const std::string &text, char sep) {
   std::vector<std::string> tokens;
   std::size_t start = 0, end = 0;
   while ((end = text.find(sep, start)) != std::string::npos) {
      tokens.push_back(text.substr(start, end - start));
      start = end + 1;
   }
   tokens.push_back(text.substr(start));
   return tokens;
}

// --------- SERVER
void
evp::Server::handleError(std::string &ret, const std::string &error) {
   ret = HTTP_response(std::string("Error: ") + error);
}

void
evp::Server::handleRequest(std::string &ret, const std::string &url) {
   ret = HTTP_response(std::string("404: ") + url);
}


// ---------- FILE SERVER
void
evp::FileServer::handleRequest(std::string &ret, const std::string &url) {
   URL u(url);
   const auto &it = files_.find(u.path);
   if(it!=files_.end()) {
      ret = it->second->get(u);

      if(u.pathExt == "") {
         ret = evp::Server::HTTP_response(ret);
      } else if(u.pathExt == "html") {
         ret = evp::Server::HTTP_response(ret);
      } else if(u.pathExt == "js") {
         ret = std::string("HTTP/1.0 200 Ok\n")
	         +"Connection: Keep-Alive\n"
                 + "Content-Type: text/javascript;charset=UTF-8\n"
        	 + "Content-Length: " + std::to_string(ret.size()) + "\n"
        	 + "\n"
        	 +ret;
      } else if(u.pathExt == "png") {
         ret = std::string("HTTP/1.0 200 Ok\n")
	         +"Connection: Keep-Alive\n"
                 + "Content-Type: image/png\n"
                 + "Content-Length: " + std::to_string(ret.size()) + "\n"
                 + "\n"
                 +ret;
      } else if(u.pathExt == "ico") {
         ret = std::string("HTTP/1.0 200 Ok\n")
	         +"Connection: Keep-Alive\n"
                 + "Content-Type: image/x-icon\n"
                 + "Content-Length: " + std::to_string(ret.size()) + "\n"
                 + "\n"
                 +ret;
      } else {
         std::cout << "unknown ext: " << u.pathExt << " " << u.path << std::endl;
      }
   } else {
      ret = HTTP_response(std::string("404: ") + url);
   }
}

void
evp::FileServer::registerFile(const std::string &url, const std::string &filename) {
   files_[url] = new evp::FileServer::FileItem(filename);
}

void
evp::FileServer::registerString(const std::string &url, const std::string &data) {
   files_[url] = new evp::FileServer::StringItem(data);
}
void
evp::FileServer::registerFunction(const std::string &url, const evp::FileServer::FunctionItem::F f) {
   files_[url] = new evp::FileServer::FunctionItem(f);
}


// ------------------ Room
evp::Room::Room(const std::string &name, RoomServer* server) 
: name_(name), server_(server) {
   server_->addRoom(this);
}
// ------------------ RoomServer
evp::RoomServer::RoomServer() {
   // set up basic handlers:
   
   // redirect to login:
   registerString("",std::string("")
           +"<html> <head> <title> REDIRECT </title>\n"
           +"<meta http-equiv='Refresh' content='0; url=login.html'/>\n"
           +"</head>\n"
           +"<body>\n"
           +"<p> redirecting...\n"
           +"</body> </html>"
   );
   registerFile("login.html","RoomServer/login.html");


   evp::FileServer::FunctionItem::F loginFunc = [&](const evp::URL &url) -> std::string {
      std::string name = url.paramString("name","");
      // validate name:
      bool success = false;
      std::string id;
      login(name, id, success);
      if(!success) {
         return std::string("")
           +"<html> <head> <title>LOGIN FAILED</title>\n"
           +"<meta http-equiv='Refresh' content='10; url=login.html'/>\n"
           +"</head>\n"
           +"<body>\n"
           +"<p>Login failed, name "+name+" already in use!</br>\n"
           +"<button onclick='window.location=\"login.html\"'>Try Again</button>\n"
           +"</body> </html>";
      } else {
         return std::string("")
           +"<html> <head> <title>LOGIN SUCESS - Redirect</title>\n"
           +"<meta http-equiv='Refresh' content='1; url=control.html'/>\n"
           +"</head>\n"
           +"<body>\n"
           +"<script>\n"
           +"document.cookie = 'user="+name+"'\n"
           +"document.cookie = 'userid="+id+"'\n"
           +"</script>\n"
           +"<p> Welcome, "+name+"!\n"
           +"</body> </html>";
      }
   };
   registerFunction("login2.html", loginFunc);
   
   registerFile("control.html","RoomServer/control.html");
   registerFile("test.html","RoomServer/test.html");
   registerFile("script.js","RoomServer/script.js");
   registerFile("util.js","RoomServer/util.js");
   registerFile("img.png","RoomServer/img.png");
   registerFile("favicon.ico","RoomServer/favicon.ico");

   registerString("hello.html",std::string("")
           +"<html> <head> <title> TITLE </title> </head>\n"
           +"<body>\n"
           +"<p> hello world\n"
           +"<img src='img.png' alt='subtitle'>\n"
           +"</body> </html>"
   );

   evp::FileServer::FunctionItem::F func = [&](const evp::URL &url) -> std::string {
      std::string id = url.paramString("uid","");
      User* u = id2user(id);
      if(u) {
         std::string s = url.paramString("0","");
         if(s=="f") {
            //u->set(0,0);
         } else {
            const auto& pp = evp::split(s,',');
            float dx = std::stod(pp[0]);
            float dy = std::stod(pp[1]);
            //u->set(dx,dy);
            std::cout << dx << " " << dy << std::endl;
	 }
   
         std::string s2 = url.paramString("5","");
         //u->setDown(s2=="t");
	 if(s2=="t") {std::cout << "t" << std::endl;}
   
         return std::string("ok");
      } else {
         return std::string("error: user");
      }
   };
   registerFunction("data", func);
 
}

