#include "evp_networking.hpp"
#include <fstream>

const uint16_t GameObject_TYPE_BASIC = 1;
struct GameObject_Basic:public GameObject
{
  float x; float y;

  GameObject_Basic(size_t my_id)
  {
    type = GameObject_TYPE_BASIC;
    id = my_id;
  }

  void toPacket(NUDPWritePacket& p)
  {
    p << id << type;
    p << x << y;
  }
  void fromPacket(NUDPReadPacket& p)
  {
    // id and type already read.
    p >> x >> y;
  }

  void draw(sf::RenderWindow &window)
  {
    DrawDot(x,y, window, sf::Color(10*id % 256,255,30*id % 256));
  }

  void updateServer()
  {
    // server only!
    x+= ((float) rand() / (RAND_MAX))-0.5;
    y+= ((float) rand() / (RAND_MAX))-0.5;
  }
};

GameObject* new_GameObject_implementation(size_t id, uint16_t type)
{
  switch (type) {
    case 0: // test objects
    {
      GameObject_Basic* obj = new GameObject_Basic(id);
      obj->x = ((float) rand() / (RAND_MAX)) * 400 + 200;
      obj->y = ((float) rand() / (RAND_MAX)) * 400 + 100;
      return obj;
    }
    case GameObject_TYPE_BASIC:
    {
      return new GameObject_Basic(id);
    }
    default:
    {
      std::cout << "(will crash) Object type not found: " << type << std::endl;
      return NULL;
    }
  }
}

void NPacketCommandHandler_implementation(NServerClient* client, NPacket &np)
{
  std::cout << "just got a command!" << std::endl;
}

// ############################## MAIN ##############################
// ############################## MAIN ##############################
// ############################## MAIN ##############################
int main(int argc, char** argv)
{
    // necessary initialization for GameObjects
    new_GameObject = new_GameObject_implementation;
    NPacketCommandHandler = NPacketCommandHandler_implementation;
    // ----- network config:
    unsigned short net_conf_port = 54000;
    sf::IpAddress net_conf_ip = "localhost";

    std::ifstream net_conf_file("networking.conf");
    std::vector<std::string> lines;
    for(std::string l; std::getline(net_conf_file, l);)
    {
      lines.push_back(l);
    }

    if(lines.size() >1)
    {
      net_conf_ip = lines[0];
      net_conf_port = (uint16_t)str2int(lines[1]);
    }

    // ----------------------------- Handle Commandline Input
    NET_SERVER_MODE = false;
    if(argc > 1)
    {
      std::string serv = "server";
      std::string arg0 = argv[1];
      if(!arg0.compare(serv))
      {
        std::cout << "Server Mode!" << std::endl;
        NET_SERVER_MODE = true;
      }
    }

    if(!NET_SERVER_MODE)
    {
      std::cout << " [INPUT HELP] use 'server' as argument to start a server!" << std::endl;
    }

    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "first attempts", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);

    // ------------------------------------ FONT
    if (!FONT.loadFromFile("arial.ttf"))
    {
        std::cout << "font could not be loaded!" << std::endl;
        return 0;
    }

    // --------------------------- EChat
    EChat chat;

    // --------------------------- Setup Networking
    if(NET_SERVER_MODE)
    {
      // -------------- SERVER:
      // chat handler:
      auto f_chat = [](NServerClient* client, const std::string &s)
      {
        std::string line;

        if(client != NULL)
        {
          // client input:

          if(s == "/help")
          {

            std::string txt= "%16%[help]%% use the percent symbol to get all the %0%R%16%G%32%B%rand% colors!";
            client->np_send.enqueueChat(txt);
            return;
          }

          line = "%255,255,255%[" + std::to_string(client->id) + "]%% " + s;
        }else{
          // server input:
          line = "%8%[server]%% " + s;

          if(s == "/help")
          {
            NET_SERVER->chat->push("%16%[help]%% use the percent symbol to get all the %0%R%16%G%32%B%rand% colors!");
            return;
          }else if(s == "/data"){
            NET_SERVER->chat->push("%16%[data]%% request some data...");

            // test data request:
            auto f_success = [](char* data, size_t size)
            {
              std::cout << "[]data received! size: " << std::to_string(size) << std::endl;

              if(data == NULL){return;}

              for(size_t i = 0; i<size; i++)
              {
                char c = i;

                if(data[i] != c)
                {
                  std::cout << "[]off at " << i << std::endl;
                }
              }
              std::cout << "[]check complete" << std::endl;
              std::free(data);
              data = NULL;
            };

            auto f_failure = [](bool noTimeout, std::string errorMsg)
            {
              std::cout << "[].test file name. failed" << std::endl;
              std::cout << "[]ERROR: " << errorMsg << std::endl;
              if(noTimeout)
              {
                std::cout << "[]not even timeout" << std::endl;
              }else{
                std::cout << "[]timeout" << std::endl;
              }
            };

            std::map<int, NServerClient*>::iterator it;
          	for (it=NET_SERVER->map_id.begin(); it!=NET_SERVER->map_id.end(); ++it)
          	{
          		NServerClient* c = it->second;
              c->np_send.issueDataRequest("testdata", f_success, f_failure);
            }
            return;
          }else if(s == "/cmd"){
            NET_SERVER->chat->push("%16%[cmd]%% sent a cmd...");

            std::vector<char> v;
            v.push_back(0);
            v.push_back(1);
            v.push_back(2);

            std::map<int, NServerClient*>::iterator it;
          	for (it=NET_SERVER->map_id.begin(); it!=NET_SERVER->map_id.end(); ++it)
          	{
          		NServerClient* c = it->second;
              c->np_send.enqueue(NPacket_TAG_COMMAND, v);
            }
            return;
          }
        }

        NET_SERVER->chat->push(line);
        NET_SERVER->broadcastChat(line);
      };

      // data request handler:
      auto f_dataReq = [](std::string fileName, NServerClient* client, bool& success, char* &data, size_t &data_size, std::string& errorMsg)
      {
        std::cout << "[RH]DATA REQUEST:" << std::endl;
        if(client!=NULL)
        {
          std::cout << "[RH]client id: " << std::to_string(client->id) << std::endl;
        }else{
          std::cout << "[RH]by server" << std::endl;
        }

        if(fileName == "testdata")
        {
          // set dummy data here:
          data_size = 10000;
          data = (char*)std::malloc(data_size);

          for(size_t i = 0; i<data_size; i++)
          {
            data[i] = i;
          }

          success = true;
        }else{
          // we will now just send error instead:
          success = false;
          errorMsg = "this is only the dummy dataRequestHandler !";
        }
      };

      // server
      NET_SERVER = new NServer(net_conf_port, &chat, f_chat, f_dataReq);


    }else{
      // ------------------ CLIENT:
      // data request handler:
      auto f_dataReq = [](std::string fileName, NServerClient* client, bool& success, char* &data, size_t &data_size, std::string& errorMsg)
      {
        std::cout << "[RH]DATA REQUEST:" << std::endl;

        success = false;
        errorMsg = "Client does not handle dataRequests !";
      };

      // client:
      NET_CLIENT = new NClient(net_conf_ip, net_conf_port, &chat, f_dataReq);

      // test data request:
      auto f_success = [](char* data, size_t size)
      {
        std::cout << "[]data received! size: " << std::to_string(size) << std::endl;

        if(data == NULL){return;}

        for(size_t i = 0; i<size; i++)
        {
          char c = i;

          if(data[i] != c)
          {
            std::cout << "[]off at " << i << std::endl;
          }
        }
        std::cout << "[]check complete" << std::endl;
        std::free(data);
        data = NULL;
      };

      auto f_failure = [](bool noTimeout, std::string errorMsg)
      {
        std::cout << "[].test file name. failed" << std::endl;
        std::cout << "[]ERROR: " << errorMsg << std::endl;
        if(noTimeout)
        {
          std::cout << "[]not even timeout" << std::endl;
        }else{
          std::cout << "[]timeout" << std::endl;
        }
      };

      for(int i=0; i<3; i++)
      {
        NET_CLIENT->np_send.issueDataRequest("testdata", f_success, f_failure);
      }
    }
    std::string textInput;

    // --------------------------- Render Loop
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
                // window closed
                case sf::Event::Closed:
                  window.close();
                  break;
                case sf::Event::TextEntered:
                  //std::cout << uint16_t(event.text.unicode) << std::endl;
                  if (event.text.unicode < 128)
                  {
                    InputHandler::push(static_cast<char>(event.text.unicode));
                    //textInput += static_cast<char>(event.text.unicode);
                    //std::cout << "ASCII character typed: " << static_cast<char>(event.text.unicode) << std::endl;
                  }
                  break;
                // we don't process other types of events
                default:
                  break;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F1) && window.hasFocus()){
          window.close();
        }

        // MAIN LOOP:
        window.clear();

        if(NET_SERVER_MODE)
        {
          // server
          NET_SERVER->update();
        }else{
          // client
          NET_CLIENT->update();
        }

        if(NET_SERVER_MODE)
        {
          // server
          NET_SERVER->draw(window);
        }else{
          // client
          NET_CLIENT->draw(window);
        }

        chat.update();

        chat.draw(0,600, window);

        window.display();
    }

    if(NET_SERVER_MODE)
    {
      // server
      delete(NET_SERVER);
    }else{
      // client
      delete(NET_CLIENT);
    }

    return 0;
}
