#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <climits>

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <list>
#include <queue>
#include <map>
#include <time.h>


struct NServerClient_identifier;
struct NServerClient;
struct NServer;
struct NClient;
struct cmpByNSC_ident;
struct NPacketRcvHandler;
struct NPacketSendHandler;
struct NPacket;
typedef std::function<void(NPacket &, NPacketSendHandler &)> NPacket_tag_function;
typedef std::function<void(std::string &)> EChat_input_function;
struct EChat;

/* TODO

- full code cleanup
- data request -> eg. map
- user names
- chat commands
- game state transmission (unreliable, newest sequence num only)
- correct deconstructions !

- benchmark number packages sent, delay, packet loss, ...

 --- completed:

 - package sequence wrap around -> infinite messages!
 - replaced sfml packages with my own packaging
 - simple chat + colors

--- beware:
currently there is an extra part which "forgets" to send packages
or sends them twice. This will have to be eventually turned off
but it is very helpful to test if my code can actually handle it.

---- data request:
all over semi-reliable transmission
1. request: req id, file-name by string. can be interpreted as one wishes.
2. answer: req id, denied + reason / ack + num packages to be sent.
3. request: req id, ack
4. answer: injection of all packages, will possibly jam the queue!

also: need timeouts, data splitting and unification at either end.

generic interface:
make request to a np_send, give a function to answer to with data/error
request handler at np_rcv -> takes file name, id of requester -> gets data / error
rest taken care of by system!

*/

typedef uint16_t NPSeq;
const NPSeq NPSeq_HALF = 32768;
const NPSeq NPSeq_START_AT = 32768 + 32750;
/*
uint16_t - 32768
*/

typedef std::map<NServerClient_identifier, NServerClient*, cmpByNSC_ident> NSCMapIdentifyer;
typedef std::map<int, NServerClient*> NSCMapId;
//----------------------------------- STATIC VARIABLES
float SCREEN_SIZE_X = 800;
float SCREEN_SIZE_Y = 600;

sf::Font FONT;

bool NET_SERVER_MODE;
NServer* NET_SERVER;
NClient* NET_CLIENT;

sf::Clock PROGRAM_CLOCK;

// --------------------------------------------------------- DRAWING
void DrawDot(float x, float y, sf::RenderWindow &window, sf::Color color = sf::Color(255,0,0))
{
  float r = 3;
  sf::CircleShape shape(r);
  shape.setFillColor(color);
  shape.setPosition(x-r, y-r);
  window.draw(shape, sf::BlendAdd);
}

void DrawLine(float x1, float y1, float x2, float y2, sf::RenderWindow &window)
{
  sf::Vertex line[] =
  {
    sf::Vertex(sf::Vector2f(x1,y1), sf::Color(0,0,0)),
    sf::Vertex(sf::Vector2f(x2,y2), sf::Color(255,255,255))
  };
  window.draw(line, 2, sf::Lines, sf::BlendAdd);
}

float DrawText(float x, float y, std::string text, float size, sf::RenderWindow &window, sf::Color color = sf::Color(255,255,255))
{
  sf::Text shape(text, FONT);//(text, FONT);
  //shape.setFont(FONT);
  //shape.setString(text); //std::to_string(input[i])
  shape.setCharacterSize(size);
  shape.setColor(color);
  shape.setPosition(x,y);
  //shape.setStyle(sf::Text::Bold | sf::Text::Underlined);
  window.draw(shape, sf::BlendAdd);

  return shape.getLocalBounds().width;
}
// ------------------------------------------------------- Input
struct InputHandler
{
  static std::queue<char> textInput;

  static void push(char c);

  static void clearText();

  static bool updateString(std::string &s);
};

std::queue<char> InputHandler::textInput;

void InputHandler::push(char c)
{
  InputHandler::textInput.push(c);
}

void InputHandler::clearText()
{
  InputHandler::textInput = std::queue<char>();
}

bool InputHandler::updateString(std::string &s)
{
  // returns true if enter hit. rest remains in queue
  while(!InputHandler::textInput.empty())
  {
    char c = InputHandler::textInput.front();
    InputHandler::textInput.pop();

    switch(c)
    {
      case 8: // Return
        if(s.size()>0)
        {
          s.pop_back();
        }
        break;
      case 13: // Enter

        return true;
        break;
      default:
        s += c;
        break;
    }
  }
  return false;
}

//-------------------------------------------------------- CHAT
struct EChat
{
  std::list<std::string> lines;
  size_t num_lines_max = 20;
  std::string inputLine;

  size_t blinker = 0;

  bool isActive = true; // true -> catches input, draw more opaque

  EChat_input_function input_handler = NULL; // called when line is entered.
  void setInputFunction(EChat_input_function f)
  {
    input_handler = f;
  }
  void draw(float x, float y, sf::RenderWindow &window)
  {
    blinker++;
    std::string blinker_txt = "";
    if(blinker > 30)
    {
      blinker = 0;
    }else if(blinker > 10)
    {
      blinker_txt = "|";
    }
    DrawText(x+5,y-20, ">" + inputLine + blinker_txt, 15, window, sf::Color(255,255,255));

    float y_pos = y-40;
    for(std::string l:lines)
    {
      drawLine(x+5,y_pos, l, window);
      y_pos -= 18;
    }
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

  void drawLine(float x, float y, const std::string &s, sf::RenderWindow &window)
  {
    // setting default color:
    sf::Color col = sf::Color(155,155,155);
    float x_pos = x;

    std::vector<std::string> tokens;

    if (s.find("rainbow") != std::string::npos)
    {
      std::string rb;

      for(size_t i = 0; i<48; i++)
      {
        rb += "%" + std::to_string(i) + "%#";
      }

      tokens = split(rb, '%');
    }else{
      tokens = split(s, '%');
    }

    bool colEnable = true;

    for(size_t i = 0; i<tokens.size(); i++)
    {
      if((i % 2) == 0 || !colEnable)
      {
        // text
        x_pos += DrawText(x_pos,y, tokens[i], 15, window, col);
      }else{
        // color
        col = sf::Color(155,155,155);

        std::vector<std::string> comp = split(tokens[i], ',');

        if(comp.size() == 3)
        {
          // RGB
          col = sf::Color(str2int(comp[0]),str2int(comp[1]),str2int(comp[2]));
        }else if(comp.size() == 1){
          if(comp[0] == "disable")
          {
            colEnable = false;
          }else{
            // Hue (HSV)

            if(comp[0] == "rand")
            {
              comp[0] = std::to_string(rand()%48);
            }

            int val = std::abs(str2int(comp[0])) % 48;

            if(val < 8)
            {
              col = sf::Color(255, val*32,0);
            }else if(val < 16){
              val-=8;
              col = sf::Color(255-val*32, 255,0);
            }else if(val < 24){
              val-=16;
              col = sf::Color(0, 255,val*32);
            }else if(val < 32){
              val-=24;
              col = sf::Color(0, 255-val*32,255);
            }else if(val < 40){
              val-=32;
              col = sf::Color(val*32,0,255);
            }else{
              val-=40;
              col = sf::Color(255, 0,255-val*32);
            }

            if(comp[0] == "")
            {
              col = sf::Color(155,155,155);
            }
          }
        }
      }

    }

  }

  void push(std::string line)
  {
    lines.push_front(line);
    while(lines.size() > num_lines_max)
    {
      lines.pop_back();
    }
  }

  void update()
  {
    // returns string if enter pressed
    if(isActive)
    {
      if(InputHandler::updateString(inputLine))
      {
        if(inputLine != "")
        {
          if(input_handler!=NULL)
          {
            input_handler(inputLine);
          }else{
            std::cout << "EChat: input handler was NULL !" << std::endl;
            std::cout << inputLine << std::endl;
          }
        }

        inputLine = "";
      }
    }
  }
};
// ------------------------------------------------------- Networking
// just send a string
const uint16_t NET_HEADER_ECHO = 100;

const uint16_t NET_HEADER_PING = 200;
const uint16_t NET_HEADER_PONG = 201;

// announce the recieve port of client
const uint16_t NET_HEADER_CLIENT_ANNOUNCE = 2000;
// server id ping -> as first message over client channel
const uint16_t NET_HEADER_SERVERCLIENT_IDPING = 3000;

// the NPacket
const uint16_t NET_HEADER_NPacket = 4000;

// ---------------- UDP Packets:
struct NUDPWritePacket
{
  std::vector<char> data;
  size_t index = 0; // where to write next, may be beyond memory

  bool send(sf::UdpSocket &socket, sf::IpAddress ip, unsigned short port)
  {
    return (socket.send(data.data(), index, ip, port) == sf::Socket::Done);
  }
};

NUDPWritePacket& operator << (NUDPWritePacket& p, const char& c){
  p.data.resize(p.index+1);
  p.data[p.index] = c;
  p.index++;

  return p;
}

NUDPWritePacket& operator << (NUDPWritePacket& p, const uint16_t& c){
  char b1 = c & 0xFF;
  char b2 = c >> 8;

  p.data.resize(p.index+2);
  p.data[p.index] = b1;
  p.data[p.index+1] = b2;
  p.index+=2;

  return p;
}

NUDPWritePacket& operator << (NUDPWritePacket& p, const size_t& c){
  char b1 = c & 0xFF;
  char b2 = c >> 8;
  char b3 = c >> 16;
  char b4 = c >> 24;


  p.data.resize(p.index+4);
  p.data[p.index] = b1;
  p.data[p.index+1] = b2;
  p.data[p.index+2] = b3;
  p.data[p.index+3] = b4;
  p.index+=4;

  return p;
}

NUDPWritePacket& operator << (NUDPWritePacket& p, const std::string& c){

  uint16_t size = c.size();

  p << size;

  p.data.resize(p.index + size);
  std::memcpy(&(p.data[p.index]), c.data(), c.size());
  p.index+=size;

  return p;
}

NUDPWritePacket& operator << (NUDPWritePacket& p, const std::vector<char>& c){

  uint16_t size = c.size();

  p << size;

  p.data.resize(p.index + size);
  std::memcpy(&(p.data[p.index]), c.data(), c.size());
  p.index+=size;

  return p;
}


struct NUDPReadPacket
{
  static const size_t rcv_buffer_size = sf::UdpSocket::MaxDatagramSize;
  static char rcv_buffer[NUDPReadPacket::rcv_buffer_size];

  std::vector<char> data;
  size_t size = 0;
  size_t index = 0; // where to read next

  void setFromVector(std::vector<char> &vec);
  bool receive(sf::UdpSocket &socket, sf::IpAddress &ip, unsigned short& port);
  bool endOfPacket();
};
char NUDPReadPacket::rcv_buffer[NUDPReadPacket::rcv_buffer_size];
void NUDPReadPacket::setFromVector(std::vector<char> &vec)
{
  data = vec;
  size = vec.size();
  index = 0;
}
bool NUDPReadPacket::receive(sf::UdpSocket &socket, sf::IpAddress &ip, unsigned short& port)
{
  // returns true if successful
  auto status = socket.receive(
    NUDPReadPacket::rcv_buffer, NUDPReadPacket::rcv_buffer_size,
    size, ip, port
  );

  if(status == sf::Socket::Done)
  {

    // success:
    data.resize(size);
    std::memcpy(data.data(), rcv_buffer, size);

    return true;
  }else{
    // failiure:

    return false;
  }
}

bool NUDPReadPacket::endOfPacket()
{
  return (index >= size);
}


NUDPReadPacket& operator >> (NUDPReadPacket& p, char& c){
  c = p.data[p.index];
  p.index++;

  return p;
}

NUDPReadPacket& operator >> (NUDPReadPacket& p, uint16_t& c){
  uint16_t b1 = p.data[p.index];
  uint16_t b2 = p.data[p.index+1];
  p.index+=2;

  c = ((uint16_t(b1)& 0xFF) | ((uint16_t(b2) & 0xFF) << 8));
  return p;
}

NUDPReadPacket& operator >> (NUDPReadPacket& p, size_t& c){
  uint16_t b1 = p.data[p.index];
  uint16_t b2 = p.data[p.index+1];
  uint16_t b3 = p.data[p.index+2];
  uint16_t b4 = p.data[p.index+3];
  p.index+=4;

  c = ((uint16_t(b1)& 0xFF)
    | ((uint16_t(b2) & 0xFF) << 8)
    | ((uint16_t(b3) & 0xFF) << 16)
    | ((uint16_t(b4) & 0xFF) << 24));

  return p;
}

NUDPReadPacket& operator >> (NUDPReadPacket& p, std::string& c){

  uint16_t size;
  p >> size;

  std::string cnew( &(p.data[p.index]), size);

  c = cnew;

  p.index+=size;

  return p;
}

NUDPReadPacket& operator >> (NUDPReadPacket& p, std::vector<char>& c){
  uint16_t size;
  p >> size;

  c.resize(size);
  std::memcpy(c.data(), &(p.data[p.index]), size);

  p.index+=size;

  return p;
}

// ---------------- The Necessary Packets:
/*
Idea:
packets that are enqueued here must be received
but the order is not guaranteed
do not use this for speed critical stuff
to be used for chat, commands, data

Each message has a tag and sequence number.
the tag is used to seperate for different uses
the idea is that one can listen for them elsewhere -> set a function!

the structure of a message:
seq. number, tag, string
*/

const uint16_t NPacket_TAG_CHAT = 1;
const uint16_t NPacket_TAG_DATAREQUEST_REQ = 2;
const uint16_t NPacket_TAG_DATAREQUEST_ANS = 3;
const uint16_t NPacket_TAG_DATAREQUEST_ACK = 4;
const uint16_t NPacket_TAG_DATAREQUEST_DAT = 5;

inline bool sequence_greater_than( NPSeq s1, NPSeq s2 )
{
  return ( ( s1 > s2 ) && ( s1 - s2 <= NPSeq_HALF ) ) ||
  ( ( s1 < s2 ) && ( s2 - s1  > NPSeq_HALF ) );
}

struct NPacket
{
  NPSeq seq;
  uint16_t tag;

  //std::string str;
  std::vector<char> data;

  // book keeping:
  sf::Int32 time_sent; // for timeout and resend
};
struct NPacketDataRequest_Send
{
  uint16_t id;
  std::string fileName;
  std::function<void(char*, size_t)> f_success;
  std::function<void(bool, std::string)> f_failiure;

  // state machine:
  size_t status = 0;
  /*
  0 - not initialized
  1 - initialized
  2 - sent off
  3 - granted, ack sent back, waiting for all packets
  4 - complete
  5 - rejected by server
  6 - timed out
  */

  size_t dataSize;
  size_t packageSize;
  uint16_t num_packages;

  std::vector<bool> rcvd; // true if reveived that package
  char* data;
  uint16_t num_packages_rcvd;

  NPacketDataRequest_Send(uint16_t id, std::string fileName, std::function<void(char*, size_t)> f_success, std::function<void(bool, std::string)> f_failiure)
  :id(id), fileName(fileName), f_success(f_success), f_failiure(f_failiure)
  {
    status = 1;
  }

  void makeReady(size_t size, size_t psize, uint16_t num_p)
  {
    // set datafield for data transmission, conter, bitfield.
    dataSize = size;
    packageSize = psize;
    num_packages = num_p;

    num_packages_rcvd = 0;
    rcvd = std::vector<bool>(num_packages, false);
    data = (char*)std::malloc(dataSize);
  }
};

struct NPacketDataRequest_Rcv
{
  uint16_t id;
  size_t dataSize;
  char* dataPtr;

  size_t packageSize;
  uint16_t num_packages;

  NPacketDataRequest_Rcv(uint16_t id, size_t dataSize, char* dataPtr)
  : id(id), dataSize(dataSize), dataPtr(dataPtr)
  {
    const size_t package_size = 1024;
    num_packages = dataSize / package_size;
    if(dataSize % package_size !=0)
    {
      num_packages++;
    }
    packageSize = package_size;
  }

  void sendPackets(NPacketSendHandler &np_send)
  {
    std::cout << "sending packets: TODO" << std::endl;
  }
};

typedef std::function<void(std::string , NServerClient* , bool& , char* , size_t &, std::string&)> DataReqHandler_t;
struct NPacketRcvHandler // np_rcv
{
  NPSeq highest_seq_num_rcvd = NPSeq_START_AT;

  uint16_t packet_counter = 0;

  std::map<NPSeq, NPSeq> map_missing; // should really be a set...

  NServerClient* client;
  std::map<uint16_t,NPacketDataRequest_Rcv*> requestMap;

  NPacketRcvHandler(NServerClient* client);

  DataReqHandler_t f_dataRequestHandler;
  void setDataRequestHandler(DataReqHandler_t f_handler);

  void createAckMessage(NUDPWritePacket &p);
  bool hasNews; // if the ack situation changed since last createAckMessage

  //typedef void (*NPacket_tag_function)(NPacket &);
  NPacket_tag_function npacket_tag_functions_default = NULL;
  std::map<uint16_t, NPacket_tag_function > npacket_tag_functions_map;
  void incomingProcess(NPacket &np, NPacketSendHandler &np_send);
  void incoming(NUDPReadPacket &p, NPacketSendHandler &np_send);

  void setDefaultTagFunction(NPacket_tag_function f);
  void addTagFunction(uint16_t tag, NPacket_tag_function f);
};


struct NPacketSendHandler // np_send
{
  NPSeq last_sequence_number = NPSeq_START_AT;
  NPSeq oldest_sequence_waiting = NPSeq_START_AT;

  sf::Int32 msgTimeOutLimit = 500; // then resend it
  int num_wait_ack_limit = 100; // this will affect the transmission speed!

  std::queue<NPacket> queue;

  std::map<NPSeq, NPacket> map_sent;

  sf::Int32 time_last_send;
  sf::Int32 minimum_send_delay = 200;

  NPacketSendHandler()
  {
    time_last_send = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
  }

  int enqueue(uint16_t tag, const std::vector<char> &txt);
  void enqueueChat(const std::string &c);
  void update(NPacketRcvHandler &np_rcv, sf::UdpSocket &socket, const sf::IpAddress &ip, const unsigned short &port);
  void readAcks(NUDPReadPacket &p);

  std::string missingToString();

  uint16_t dataRequestNextId = 0; // reasonably never wrap around and overlap.
  std::map<uint16_t,NPacketDataRequest_Send*> requestMap;
  void issueDataRequest(std::string fileName, std::function<void(char*, size_t)> f_success, std::function<void(bool, std::string)> f_failiure);
  void deniedDataRequest(uint16_t req_id, std::string errorMsg);
  void ackDataRequest(uint16_t req_id, uint16_t num_packages, size_t packageSize,size_t dataSize);
  void drawDataRequests(float x, float y, sf::RenderWindow &window);
};

struct NServerClient_identifier
{
  sf::IpAddress remoteIp;
  unsigned short remotePort;
};

struct cmpByNSC_ident {
  bool operator() (const NServerClient_identifier& lhs, const NServerClient_identifier& rhs) const
  {
    if(lhs.remotePort == rhs.remotePort)
    {
      return lhs.remoteIp < rhs.remoteIp;
    }else{
      return lhs.remotePort < rhs.remotePort;
    }
  }
};

struct NServerClient
{
  static int id_last;
  uint16_t id;
  sf::IpAddress remoteIp;
  unsigned short remotePort;

  NPacketRcvHandler np_rcv;
  NPacketSendHandler np_send;

  // time out ticker:
  sf::Int32 last_rcv;
  sf::Int32 last_ping;

  NServerClient(sf::IpAddress remoteIp, unsigned short remotePort, NServer *nserver);

  void sendPacket(NUDPWritePacket &p, sf::UdpSocket &socket);
  void sendIdPing(sf::UdpSocket &socket);
  void sendPing(sf::UdpSocket &socket);
  void sendPong(sf::UdpSocket &socket);
  void rcvPacket(NUDPReadPacket &p, sf::UdpSocket &socket);
  void update(sf::UdpSocket &socket);
  void draw(float x, float y, sf::RenderWindow &window);
};

struct NServer
{
  unsigned short socket_port = 54000;
  sf::UdpSocket socket;

  NSCMapIdentifyer map_identifyer;
  NSCMapId map_id;

  EChat *chat;
  NServer(EChat *chat);
  void handleChatInput(NServerClient* client, const std::string &s);
  void broadcastChat(const std::string &s);
  void addClient(NServerClient* c);
  void removeClient(NServerClient* c);
  NServerClient* getClient(sf::IpAddress ip, unsigned short port);
  void draw(sf::RenderWindow &window);
  void update();
};

struct NClient
{
  sf::UdpSocket socket;
  unsigned short destination_port = 54000;
  sf::IpAddress recipient = "192.168.1.41";

  unsigned short socket_port;

  NPacketRcvHandler np_rcv;
  NPacketSendHandler np_send;

  uint16_t id_on_server;

  EChat *chat;

  // ------------------------------------- STATE MACHINE start
  int finite_state = 0;
  // 0 - initial
  sf::Int32 last_announce; // repeated announcement !
  int number_announcements = 0;

  // 1 - got answer from server -> id
  sf::Int32 last_rcv;

  // ------------------------------------- STATE MACHINE end
  NClient(EChat *chat);
  void draw(sf::RenderWindow &window);
  void update();
  void announcePort();
  void sendPing();
  void sendPong();
};

// ########################################### NPacketSendHandler ##########
// ########################################### NPacketSendHandler ##########
// ########################################### NPacketSendHandler ##########

int NPacketSendHandler::enqueue(uint16_t tag, const std::vector<char> &txt)
{
  // returns sequence number of the Packet
  last_sequence_number++;

  queue.push(NPacket{last_sequence_number, tag, txt});

  return queue.size();
}

void NPacketSendHandler::enqueueChat(const std::string &c)
{
  std::vector<char> v;
  v.resize(c.size());
  std::memcpy(v.data(), c.data(), c.size());

  enqueue(NPacket_TAG_CHAT, v);
}

void NPacketSendHandler::update(NPacketRcvHandler &np_rcv, sf::UdpSocket &socket, const sf::IpAddress &ip, const unsigned short &port)
{
  sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

  if((now_time - time_last_send)>minimum_send_delay && (queue.size() > 0 || map_sent.size() < num_wait_ack_limit || np_rcv.hasNews))
  {
    time_last_send = now_time;
    // something to send:
    NUDPWritePacket p;
    p << NET_HEADER_NPacket;

    np_rcv.createAckMessage(p);

    // update from map (the ones that have timed out)
    std::map<NPSeq, NPacket>::iterator it;

    for (it=map_sent.begin(); it!=map_sent.end(); ++it)
    {
      NPSeq seq = it->first;
      sf::Int32 sent_time = it->second.time_sent;
      if( (now_time - sent_time) > msgTimeOutLimit)
      {
        if( ((float) rand() / (RAND_MAX)) < 0.8 )
        {
          p << it->second.seq << it->second.tag << it->second.data;

          if( ((float) rand() / (RAND_MAX)) < 0.1 )
          {
            p << it->second.seq << it->second.tag << it->second.data; // send double!
          }
        }

        it->second.time_sent = now_time;
      }
    }

    // append new ones
    while(queue.size() > 0 && map_sent.size() < num_wait_ack_limit && p.index < 50000)
    {
      // send from queue, append to map
      NPacket np = queue.front();
      queue.pop();

      np.time_sent = now_time;

      // artificially loose and double some packets:
      if( ((float) rand() / (RAND_MAX)) < 0.8 )
      {
        p << np.seq << np.tag << np.data;

        if( ((float) rand() / (RAND_MAX)) < 0.1 )
        {
          p << np.seq << np.tag << np.data; // send double!
        }
      }

      map_sent.insert(
        std::pair<NPSeq, NPacket>
        (np.seq, np)
      );
    }

    // send packet:
    if(!p.send(socket,ip, port))//socket.send(p, ip, port) != sf::Socket::Done)
    {
        std::cout << "Error on send ! (NPacket)" << std::endl;
    }
  }
}

void NPacketSendHandler::readAcks(NUDPReadPacket &p)
{
  NPSeq highest;
  uint16_t num_missing;

  p >> highest >> num_missing;

  std::vector<NPSeq> missingVec;
  missingVec.reserve(num_missing+1);
  std::vector<NPSeq> missingWrapVec;
  missingWrapVec.reserve(num_missing);

  for(uint16_t i = 0; i<num_missing; i++)
  {
    NPSeq seq;
    p >> seq;

    if(seq > highest){
      // wrapping around!
      missingWrapVec.push_back(seq);
    }else{
      // normal
      missingVec.push_back(seq);
    }
  }
  missingVec.push_back(highest+1);

  std::vector<NPSeq> deletees;
  deletees.reserve(map_sent.size());
  std::map<NPSeq, NPacket>::iterator it;

  // go about deleting left over high ones (wrap)
  it = map_sent.upper_bound(NPSeq_HALF);
  if(highest < NPSeq_HALF && it != map_sent.end())
  {
    std::cout << "WRAP AROUND DANGER!" << std::endl;

    size_t i = 0; // index in missingWrapVec

    do {
      // work through rest of high elements:
      NPSeq seq = it->first;
      std::cout << seq << std::endl;

      while(i<missingWrapVec.size() && seq > missingWrapVec[i])
      {i++;}

      if(i>=missingWrapVec.size() || seq < missingWrapVec[i])
      {
        // can delete:
        deletees.push_back(seq);
        std::cout << "del." << std::endl;
      }

      // loop stuff below
      ++it;
    } while(it != map_sent.end() && i<missingWrapVec.size());
  }

  size_t i = 0; // index in missingVec

  // iterate through all that are still to be acked:
  for (it=map_sent.begin(); it!=map_sent.end(); ++it)
  {
    NPSeq seq = it->first;

    if( seq <= highest )// seq <= highest
    {
      while(i<missingVec.size() && sequence_greater_than(seq, missingVec[i]))
      {
        // advance in acks:
        i++;
      }

      if(i<missingVec.size() && sequence_greater_than(missingVec[i], seq))
      {
        // acked!
        //map_sent.erase(seq);

        deletees.push_back(seq);
      }
    }
  }

  // actually delete the deletees:
  for(size_t i =0 ; i< deletees.size(); i++)
  {
    map_sent.erase(deletees[i]);
  }
}


std::string NPacketSendHandler::missingToString()
{
  std::string ret;
  ret = "n: " + std::to_string(map_sent.size()) + ".  ";

  std::map<NPSeq, NPacket>::iterator it;
  for (it=map_sent.begin(); it!=map_sent.end(); ++it)
  {
    NPSeq seq = it->first;
    ret.append(" " + std::to_string(seq) + ",");
  }

  return ret;
}

void NPacketSendHandler::issueDataRequest(std::string fileName, std::function<void(char*, size_t)> f_success, std::function<void(bool, std::string)> f_failiure)
{
  // f_failiure(noTimeout, errortext)
  // f_success(data, datasize)
  std::cout << "issueDataRequest: " << fileName << std::endl;

  // build request object:
  NPacketDataRequest_Send * req = new NPacketDataRequest_Send(
    ++dataRequestNextId, fileName, f_success, f_failiure
  );
  requestMap.insert(
    std::pair<uint16_t, NPacketDataRequest_Send*>
    (req->id,req)
  );

  // build request message:
  NUDPWritePacket reqMsg;
  reqMsg << req->id;
  reqMsg << fileName;

  enqueue(NPacket_TAG_DATAREQUEST_REQ, reqMsg.data);
  req->status = 2;
  // the next step woud be getting a req_id, save req in a map, sending the request off
}

void NPacketSendHandler::deniedDataRequest(uint16_t req_id, std::string errorMsg)
{
  std::cout << "denied" << std::endl;

  std::map<uint16_t, NPacketDataRequest_Send*>::iterator it;

	it = requestMap.find(req_id);

	if(it != requestMap.end()){
		NPacketDataRequest_Send * req = it->second;
    req->status = 5;
    req->f_failiure(true, errorMsg);

    requestMap.erase(it);
    delete(req);
	}else{
		// not found! not sure what to do!
    std::cout << "DID NOT FIND THE DENIED REQUEST ANYWAY...???" << std::endl;
	}
}
void NPacketSendHandler::ackDataRequest(uint16_t req_id, uint16_t num_packages, size_t packageSize,size_t dataSize)
{
  std::cout << "acked" << std::endl;

  std::map<uint16_t, NPacketDataRequest_Send*>::iterator it;

	it = requestMap.find(req_id);

	if(it != requestMap.end()){
		NPacketDataRequest_Send * req = it->second;

    std::cout << "size: " << std::to_string(dataSize) << ", psize: " << std::to_string(packageSize) <<", #p: " << std::to_string(num_packages) << std::endl;
    req->status = 3;
    req->dataSize = dataSize;
    req->packageSize = packageSize;
    req->num_packages = num_packages;
  }else{
		// not found! not sure what to do!
    std::cout << "DID NOT FIND THE ACKED REQUEST ...???" << std::endl;
	}
}

void NPacketSendHandler::drawDataRequests(float x, float y, sf::RenderWindow &window)
{
  std::map<uint16_t, NPacketDataRequest_Send*>::iterator it;
	for (it=requestMap.begin(); it!=requestMap.end(); ++it)
	{
		NPacketDataRequest_Send* req = it->second;
    DrawText(x,y,
      "ID: " + std::to_string(req->id) + ", status:" + std::to_string(req->status),
      12, window, sf::Color(100,100,100)
    );

    y+=15;
	}

}
// ---------------- NPacketRcvHandler
NPacketRcvHandler::NPacketRcvHandler(NServerClient* client): client(client)
{
  auto f = [](std::string fileName, NServerClient* client, bool& success, char* data, size_t &data_size, std::string& errorMsg)
  {
    std::cout << "DATA REQUEST:" << std::endl;
    if(client!=NULL)
    {
      std::cout << "client id: " << std::to_string(client->id) << std::endl;
    }else{
      std::cout << "by server" << std::endl;
    }

    if(fileName == "testdata")
    {
      // set dummy data here:
      data_size = 100000;
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
  setDataRequestHandler(f);


  // ------------- set dataRequest packet handlers:

  auto f_req = [this, client](NPacket &np, NPacketSendHandler &np_send)
  {
    std::cout << " - DATAREQUEST_REQ" << std::endl;

    // reading packet in np.data:
    NUDPReadPacket p;
    p.setFromVector(np.data);

    uint16_t req_id;
    std::string fileName;
    p >> req_id;
    p >> fileName;

    std::cout << " - id: " << req_id << ", fileName: " << fileName << std::endl;

    // now see if data available:
    bool success;
    char* dataPtr;
    size_t dataSize;
    std::string errorMsg;
    f_dataRequestHandler(fileName, client, success, dataPtr, dataSize, errorMsg);

    if(success)
    {
      std::cout << " - success Data Request" << std::endl;

      // anser, store data somewhere...
      NPacketDataRequest_Rcv* req = new NPacketDataRequest_Rcv(req_id, dataSize, dataPtr);
      requestMap.insert(
        std::pair<uint16_t,NPacketDataRequest_Rcv*>
        (req_id, req)
      );

      uint16_t acked = 1;
      NUDPWritePacket reqMsg;
      reqMsg << req_id;
      reqMsg << acked;
      reqMsg << req->dataSize;
      reqMsg << req->packageSize;
      reqMsg << req->num_packages;

      np_send.enqueue(NPacket_TAG_DATAREQUEST_ANS, reqMsg.data);
    }else{
      std::cout << " - failure Data Request" << std::endl;

      // send denied back:
      uint16_t denied = 0;
      NUDPWritePacket reqMsg;
      reqMsg << req_id;
      reqMsg << denied;
      reqMsg << errorMsg;

      np_send.enqueue(NPacket_TAG_DATAREQUEST_ANS, reqMsg.data);
    }
  };

  auto f_ans = [](NPacket &np, NPacketSendHandler &np_send)
  {
    std::cout << " - DATAREQUEST_ANS" << std::endl;

    // reading packet in np.data:
    NUDPReadPacket p;
    p.setFromVector(np.data);

    uint16_t req_id;
    uint16_t answer; // 0 denied, 1 ack
    p >> req_id;
    p >> answer;

    if(answer == 0)
    {
      // denied
      std::string errorMsg;
      p >> errorMsg;
      np_send.deniedDataRequest(req_id, errorMsg);
    }else{
      // ack
      size_t dataSize;
      size_t packageSize;
      uint16_t num_packages;
      p >> dataSize;
      p >> packageSize;
      p >> num_packages;
      np_send.ackDataRequest(req_id, num_packages, packageSize, dataSize);

      // send ack back.
      NUDPWritePacket reqMsg;
      reqMsg << req_id;
      np_send.enqueue(NPacket_TAG_DATAREQUEST_ACK, reqMsg.data);
    }
  };

  auto f_ack = [this](NPacket &np, NPacketSendHandler &np_send)
  {
    std::cout << "DATAREQUEST_ACK" << std::endl;

    // reading packet in np.data:
    NUDPReadPacket p;
    p.setFromVector(np.data);

    uint16_t req_id;
    p >> req_id;

    std::map<uint16_t,NPacketDataRequest_Rcv*>::iterator it;

  	it = requestMap.find(req_id);

  	if(it != requestMap.end()){
  		NPacketDataRequest_Rcv * req = it->second;

      // push the packets !
      req->sendPackets(np_send);
      requestMap.erase(it);
      delete(req);
    }else{
      std::cout << "DID NOT FIND THE ID ...???" << std::endl;
    }
  };

  auto f_dat = [](NPacket &np, NPacketSendHandler &np_send)
  {
    std::cout << "DATAREQUEST_DAT" << std::endl;
  };

  addTagFunction(NPacket_TAG_DATAREQUEST_REQ, f_req);
  addTagFunction(NPacket_TAG_DATAREQUEST_ANS, f_ans);
  addTagFunction(NPacket_TAG_DATAREQUEST_ACK, f_ack);
  addTagFunction(NPacket_TAG_DATAREQUEST_DAT, f_dat);
}

void NPacketRcvHandler::setDataRequestHandler(DataReqHandler_t f_handler)
{
  f_dataRequestHandler = f_handler;
}

void NPacketRcvHandler::createAckMessage(NUDPWritePacket &p)
{
  hasNews = false;

  p << highest_seq_num_rcvd;
  p << (uint16_t)(map_missing.size());

  // send all that are missing:
  std::map<NPSeq, NPSeq>::iterator it;
	for (it=map_missing.begin(); it!=map_missing.end(); ++it)
	{
		p << (it->second);
  }
}
void NPacketRcvHandler::incomingProcess(NPacket &np, NPacketSendHandler &np_send)
{
  // process according to tag:
  std::map<uint16_t, NPacket_tag_function>::iterator it;

  it = npacket_tag_functions_map.find(np.tag);

  if(it != npacket_tag_functions_map.end())
  {
    // call the function:
    if(it->second != NULL)
    {
      it->second(np, np_send);
    }else{
        std::cout << "tag found, but function is NULL" << std::endl;
    }
  }else{
    // no found: default
    if(npacket_tag_functions_default != NULL)
    {
      npacket_tag_functions_default(np, np_send);
    }else{
      // not even default.
      std::cout << "no matching function. not even default..." << std::endl;
    }
  }
}

void NPacketRcvHandler::incoming(NUDPReadPacket &p, NPacketSendHandler &np_send)
{
  // header already removed.

  // read acks
  np_send.readAcks(p);

  // read messages
  while(!p.endOfPacket())
  {
    hasNews = true;

    NPacket np;
    p >> np.seq >> np.tag >> np.data;


    if(sequence_greater_than(np.seq, highest_seq_num_rcvd))
    {
      // insert all between:
      //if(np.seq > highest_seq_num_rcvd)
      {
        for(NPSeq i = highest_seq_num_rcvd+1; sequence_greater_than(np.seq, i); i++)
        {
          map_missing.insert(
            std::pair<NPSeq, NPSeq>
            (i,i)
          );
        }
      }

      highest_seq_num_rcvd = np.seq;

      packet_counter++;
      //std::cout << "newest: " << np.seq << ":" << np.tag << std::endl;
      incomingProcess(np, np_send);
    }else{
      // one of the missing ones? else reject.
      std::map<NPSeq,NPSeq>::iterator it;

      it = map_missing.find(np.seq);
      if(it != map_missing.end()){
        map_missing.erase(np.seq);

        packet_counter++;
        //std::cout << "missing: " << np.seq << ":" << np.tag << std::endl;
        incomingProcess(np, np_send);
  		}else{
  			// not found -> was a double!
  		}
    }
  }
}

void NPacketRcvHandler::setDefaultTagFunction(NPacket_tag_function f)
{
  // set function if valid
  if(f != NULL)
  {
    npacket_tag_functions_default = f;
  }else{
    std::cout << "new default tag function is NULL !" << std::endl;
  }
}
void NPacketRcvHandler::addTagFunction(uint16_t tag, NPacket_tag_function f)
{
  // test if already set:
  std::map<uint16_t, NPacket_tag_function>::iterator it;

  it = npacket_tag_functions_map.find(tag);

  if(it != npacket_tag_functions_map.end())
  {
    if(it->second != NULL)
    {
      std::cout << "function for tag " << tag << " already exists !" << std::endl;
      return;
    }else{
      // was set but is not valid. need resetting:
      std::cout << "function for tag " << tag << " was set to NULL -> overwrite!" << std::endl;
    }
  }else{
    // no found -> good !
  }

  // set function if valid
  if(f != NULL)
  {
    npacket_tag_functions_map.insert(
      std::pair<uint16_t, NPacket_tag_function>
      (tag, f)
    );
  }else{
    std::cout << "new function for tag " << tag << " is NULL !" << std::endl;
  }
}

// ###################################### NServerClient ######################################
// ###################################### NServerClient ######################################
// ###################################### NServerClient ######################################
// ###################################### NServerClient ######################################



// ###################################### NServerClient
NServerClient::NServerClient(sf::IpAddress remoteIp, unsigned short remotePort, NServer *nserver)
  : remoteIp(remoteIp), remotePort(remotePort), np_rcv(NPacketRcvHandler(this))
{
  NServerClient::id_last++;
  id = NServerClient::id_last;

  last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
  last_ping = last_rcv;

  // set NPacket tag functions:
  auto f_default = [](NPacket &np, NPacketSendHandler &np_send)
  {
    std::cout << "default packet function. tag: " << np.tag << std::endl;
  };
  auto f_chat = [this, nserver](NPacket &np, NPacketSendHandler &np_send)
  {
    std::string cnew( &(np.data[0]), np.data.size());
    nserver->handleChatInput(this, cnew);
  };
  np_rcv.setDefaultTagFunction(f_default);
  np_rcv.addTagFunction(NPacket_TAG_CHAT, f_chat);
}

void NServerClient::sendPacket(NUDPWritePacket &p, sf::UdpSocket &socket)
{
  if(!p.send(socket,remoteIp, remotePort)) //(socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}

void NServerClient::sendIdPing(sf::UdpSocket &socket)
{
  NUDPWritePacket p;

  p << NET_HEADER_SERVERCLIENT_IDPING << id;
  if(!p.send(socket,remoteIp, remotePort))//if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}

void NServerClient::sendPing(sf::UdpSocket &socket)
{
  NUDPWritePacket p;
  p << NET_HEADER_PING;
  if(!p.send(socket,remoteIp, remotePort))//if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}

void NServerClient::sendPong(sf::UdpSocket &socket)
{
  NUDPWritePacket p;
  p << NET_HEADER_PONG;
  if(!p.send(socket,remoteIp, remotePort))//if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}

void NServerClient::rcvPacket(NUDPReadPacket &p, sf::UdpSocket &socket)
{
  last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

  uint16_t header;
  p >> header;

  switch (header)
  {
    case NET_HEADER_ECHO:
      {
        std::string txt;
        p >> txt;

        std::cout << "(" << id <<") Echo: " << txt << std::endl;

        break;
      }
    case NET_HEADER_PING:
    {
      // send back pong:
      sendPong(socket);
      break;
    }
    case NET_HEADER_PONG:
    {
      std::cout << "(" << id <<") Pong" << std::endl;
      break;
    }
    case NET_HEADER_NPacket:
    {
      np_rcv.incoming(p, np_send);
      break;
    }
    default:   // ------------ DEFAULT
      std::cout << "unreadable header: "<< header << std::endl;
      break;
  }
}

void NServerClient::update(sf::UdpSocket &socket)
{
  // time out prevention:
  sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

  if( (now_time - last_rcv) > 500)
  {
    if( (now_time - last_ping) > 300)
    {
      last_ping = now_time;
      sendPing(socket);
    }
  }

  np_send.update(np_rcv, socket, remoteIp, remotePort);
}

void NServerClient::draw(float x, float y, sf::RenderWindow &window)
{
  DrawText(x,y,
    "ID: " + std::to_string(id) + ", " + remoteIp.toString() + ":" + std::to_string(remotePort),
    12, window, sf::Color(255,0,0)
  );

  DrawText(x+150,y,
    "q: " + std::to_string(np_send.queue.size()) + ", w: " + std::to_string(np_send.map_sent.size()),
    12, window, sf::Color(255,255,255)
  );


  DrawText(x+230,y,
    "m: " + std::to_string(np_rcv.map_missing.size()) + ", h: " + std::to_string(np_rcv.highest_seq_num_rcvd) + ", cnt: " + std::to_string(np_rcv.packet_counter),
    12, window, sf::Color(0,255,255)
  );

  sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

  if( (now_time - last_rcv) > 500)
  {
    DrawText(x+400,y,
      "Timeout Danger: " + std::to_string( (float)(now_time - last_rcv)/1000.0 ) + " / 10",
      12, window, sf::Color(255,0,0)
    );
  }

  np_send.drawDataRequests(x+500,y, window);
}

int NServerClient::id_last = 0;

// ######################################## NServer

NServer::NServer(EChat *chat): chat(chat)
{
  // bind socket:
  if(socket.bind(socket_port) != sf::Socket::Done)
  {
    std::cout << "could not bind socket!" << std::endl;
    return;
  }
  socket.setBlocking(false);
  std::cout << "Server Created." << std::endl;

  // ------------ chat
  EChat_input_function f_input = [this](std::string &s)
  {
    handleChatInput(NULL, s);
  };
  chat->setInputFunction(f_input);
}

void NServer::handleChatInput(NServerClient* client, const std::string &s)
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
      chat->push("%16%[help]%% use the percent symbol to get all the %0%R%16%G%32%B%rand% colors!");
      return;
    }
  }

  chat->push(line);
  broadcastChat(line);
}

void NServer::broadcastChat(const std::string &s)
{
  std::map<int, NServerClient*>::iterator it;
	for (it=map_id.begin(); it!=map_id.end(); ++it)
	{
		NServerClient* c = it->second;

    c->np_send.enqueueChat(s);
	}
}

void NServer::addClient(NServerClient* c)
{
	map_identifyer.insert(
    std::pair<NServerClient_identifier, NServerClient*>
    (NServerClient_identifier{c->remoteIp, c->remotePort}, c)
  );

  map_id.insert(
    std::pair<int, NServerClient*>
    (c->id, c)
  );
}

void NServer::removeClient(NServerClient* c)
{
  map_identifyer.erase(
    NServerClient_identifier{c->remoteIp, c->remotePort}
  );

  map_id.erase(
    c->id
  );
}

NServerClient* NServer::getClient(sf::IpAddress ip, unsigned short port)
{
	std::map<NServerClient_identifier, NServerClient*>::iterator it;

	it = map_identifyer.find(NServerClient_identifier{ip, port});

	if(it != map_identifyer.end()){
		return it->second;
	}else{
		return NULL;
	}
}

void NServer::draw(sf::RenderWindow &window)
{
  DrawText(5,5, "Server Mode.", 12, window, sf::Color(255,0,0));

  // draw all clients:
  std::map<int, NServerClient*>::iterator it;

  int counter = 0;
	for (it=map_id.begin(); it!=map_id.end(); ++it)
	{
		NServerClient* c = it->second;

    c->draw(10, 20+counter*15,window);

    counter++;
	}
}

void NServer::update()
{
  bool repeat;

  do
  {
    NUDPReadPacket p;
    sf::IpAddress remoteIp;
    unsigned short remotePort;

    bool success = p.receive(socket, remoteIp, remotePort);
    //auto status = socket.receive(p, remoteIp, remotePort);

    repeat = false;
    if(success)
    {
      repeat = true;

      // find client:
      NServerClient* c = getClient(remoteIp, remotePort);
      if(c == NULL)
      {
        // create only if this was a port announcement:
        uint16_t header;
        p >> header;

        if( header == NET_HEADER_CLIENT_ANNOUNCE)
        {
          // create !
          c = new NServerClient(remoteIp, remotePort, this);
          addClient(c);

          c->sendIdPing(socket);
        }else{
          std::cout << "ignoring msg (header " << header << ")" << std::endl;
        }

      }else{
        // ok, can rcv packet for the client:
        c->rcvPacket(p, socket);
      }
    }
  }while(repeat);

  // update all clients:
  std::map<int, NServerClient*>::iterator it;

  sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

	for (it=map_id.begin(); it!=map_id.end(); ++it)
	{
		NServerClient* c = it->second;

    if((now_time - c->last_rcv) > 10000)
    {
      // timed out
      std::cout << "timeout " << c->id << std::endl;

      removeClient(c);
    }

    c->update(socket);
	}

}


// ########################################### NClient
NClient::NClient(EChat *chat): np_rcv(NPacketRcvHandler(NULL))
{
  if(socket.bind(sf::Socket::AnyPort) != sf::Socket::Done)
  {
    std::cout << "could not bind socket!" << std::endl;
    return;
  }
  socket.setBlocking(false);
  socket_port = socket.getLocalPort();

  announcePort();

  last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

  std::cout << "Client Created. listen on " << socket_port << std::endl;

  // ------------------------------------------------------- EChat.
  EChat_input_function f_input = [this](std::string &s)
  {
    // just send line to server, it will decide how to bounce it back.
    np_send.enqueueChat(s);
  };
  chat->setInputFunction(f_input);

  // -------------------------------------    set NPacket tag functions:
  auto f_default = [](NPacket &np, NPacketSendHandler &np_send)
  {
    std::cout << "default packet function. tag: " << np.tag << std::endl;
  };
  auto f_chat = [chat](NPacket &np, NPacketSendHandler &np_send)
  {
    std::string cnew( &(np.data[0]), np.data.size());

    chat->push(cnew);
  };

  np_rcv.setDefaultTagFunction(f_default);
  np_rcv.addTagFunction(NPacket_TAG_CHAT, f_chat);
}

void NClient::draw(sf::RenderWindow &window)
{
  DrawText(5,5, "Client Mode. connected to " + recipient.toString() + ":" + std::to_string(destination_port), 12, window, sf::Color(0,255,0));

  DrawText(5,20, "State: " + std::to_string(finite_state), 12, window, sf::Color(0,255,0));

  sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
  if( (now_time - last_rcv) > 500)
  {
    DrawText(5,35, "Time out? " + std::to_string( (float)(now_time - last_rcv)/1000.0), 12, window, sf::Color(255,0,0));
  }

  DrawText(5,50,
    "q: " + std::to_string(np_send.queue.size()) + ", w: " + std::to_string(np_send.map_sent.size())
     + np_send.missingToString(),
    12, window, sf::Color(255,255,255)
  );

  DrawText(5,65,
    "m: " + std::to_string(np_rcv.map_missing.size()) + ", h: " + std::to_string(np_rcv.highest_seq_num_rcvd) + ", cnt: " + std::to_string(np_rcv.packet_counter),
    12, window, sf::Color(0,255,255)
  );

  np_send.drawDataRequests(5,100, window);
}

void NClient::update()
{
  bool repeat = false;
  do
  {
    NUDPReadPacket p;
    sf::IpAddress remoteIp;
    unsigned short remotePort;

    bool success = p.receive(socket, remoteIp, remotePort);
    //auto status = socket.receive(p, remoteIp, remotePort);

    repeat = false;
    if(success)
    {
      last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();


      repeat = true;
      uint16_t header;
      p >> header;

      switch (header)
      {
        case NET_HEADER_ECHO:
          {
            std::string txt;
            p >> txt;

            std::cout << "IP: " << remoteIp << " : " << remotePort << std::endl;
            std::cout << "Echo: " << txt << std::endl;

            break;
          }
        case NET_HEADER_SERVERCLIENT_IDPING:
          {
            p >> id_on_server;

            std::cout << "IP: " << remoteIp << " : " << remotePort << std::endl;
            std::cout << "Id Ping: " << id_on_server << std::endl;

            if(finite_state == 0)
            {
              finite_state = 1;
            }

            break;
          }
        case NET_HEADER_PING:
          {
            // send pong back
            sendPong();
            break;
          }
        case NET_HEADER_PONG:
          {
            std::cout << "pong" << std::endl;
            break;
          }
        case NET_HEADER_NPacket:
          {
            np_rcv.incoming(p, np_send);
            break;
          }
        default:   // ------------ DEFAULT
          std::cout << "unreadable header: "<< header << std::endl;
          break;
      }
    }
  }
  while(repeat);

  // STATE MACHINE ITSELF
  switch(finite_state)
  {
    case 0:
    {
      sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

      if((now_time-last_announce) > 1000){
        announcePort();
        std::cout << "send announce Port" << std::endl;
      }
      break;
    }
    case 1:
    {
      np_send.update(np_rcv, socket, recipient, destination_port);

      break;
    }
    default:
    {
      std::cout << "undefined state: " << finite_state << std::endl;
      break;
    }
  }
}

void NClient::announcePort()
{
  NUDPWritePacket p;
  p << NET_HEADER_CLIENT_ANNOUNCE << socket_port;

  if( !p.send(socket, recipient, destination_port))//if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }

  number_announcements++;
  last_announce = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
}

void NClient::sendPing()
{
  NUDPWritePacket p;
  p << NET_HEADER_PING;

  if( !p.send(socket, recipient, destination_port))//if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}

void NClient::sendPong()
{
  NUDPWritePacket p;
  p << NET_HEADER_PONG;

  if( !p.send(socket, recipient, destination_port))//if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}


//-------------------------------------------------------- Main
int main(int argc, char** argv)
{
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
      // server
      NET_SERVER = new NServer(&chat);
    }else{
      // client
      NET_CLIENT = new NClient(&chat);

      // test data request:
      auto f_success = [](char* data, size_t size)
      {
        std::cout << "data received! size: " << std::to_string(size) << std::endl;

        if(data == NULL){return;}
        std::free(data);
      };

      auto f_failure = [](bool noTimeout, std::string errorMsg)
      {
        std::cout << ".test file name. failed" << std::endl;
        std::cout << "ERROR: " << errorMsg << std::endl;
        if(noTimeout)
        {
          std::cout << "not even timeout" << std::endl;
        }else{
          std::cout << "timeout" << std::endl;
        }
      };

      NET_CLIENT->np_send.issueDataRequest("testdata", f_success, f_failure);
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

        /*
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && window.hasFocus()){
          if(!NET_SERVER_MODE)
          {
            std::string txt = "hello " + std::to_string(rand());
            NET_CLIENT->np_send.enqueue(0, txt);
          }
        }
        */

        // chat INPUT



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

    return 0;
}
