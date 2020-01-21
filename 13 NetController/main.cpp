#include <iostream>
#include <map>
#include <vector>
#include <SFML/Network.hpp>
#include <cassert>
#include <string>

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



int main(int argc, char** argv) {
    std::cout << "Starting..." << std::endl;
    
    sf::TcpListener listener;
    if (listener.listen(sf::Socket::AnyPort) != sf::Socket::Done) {
       std::cout << "ERROR port" << std::endl;
       assert(false);
    }
    unsigned short port = listener.getLocalPort();
    std::cout << "Port: " << port << std::endl;
    
    while(true) {
       sf::TcpSocket socket;
       if (listener.accept(socket) != sf::Socket::Done) {
          std::cout << "ERROR socket" << std::endl;
          assert(false);
       }
       std::cout << "Client connected: " << socket.getRemoteAddress() << std::endl;
       
       char in[2048];
       std::size_t received;
       if (socket.receive(in, sizeof(in), received) != sf::Socket::Done) {
          std::cout << "ERROR rcv" << std::endl;
          assert(false);
       }
       std::cout << "Answer received from the client: \"" << in << "\"" << std::endl;
       
       std::string s(in);
       std::vector<std::string> lines(split(s,'\n'));
       std::vector<std::string> first(split(lines[0],' '));

       std::cout << "Request: " << first[0] << " - " << first[1] << std::endl;
       
       std::string resp = "HTTP/ 1.1 200 OK\nContent-Type: text/html\n\n";

       resp+= "Requested:\n";
       resp+= first[1];
       
       if (socket.send(resp.c_str(), resp.size()) != sf::Socket::Done) {
          std::cout << "ERROR snd" << std::endl;
          assert(false);
       }
    }

    //std::cout << "Press enter to exit..." << std::endl;
    //std::cin.ignore(10000, '\n');

    return 0;
}
