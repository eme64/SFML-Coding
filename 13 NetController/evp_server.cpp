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

