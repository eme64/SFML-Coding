/* TODO
 - smaller packet size? 500
 - differing send speeds for different clients?
 - datagram Size check ??? -> some seem to be too big... - from sent-> chech size!
 - react to benchmark data?
 -> what we need for better control:
    - RTT
    - num packets received lately -> can then increase per rcvd.

- check memory leeks
- user names

---- General Ideas about the Net Framework:
 - game session: lobby and game


--- net framework complete:
chat handler.
data req handler
semi reliable packets -> command
... game objects. -> render against others? compress data? delete?

 --- completed:

 - package sequence wrap around -> infinite messages!
 - replaced sfml packages with my own packaging
 - simple chat + colors
 - deconstructors -> timeout handling
 - data requesting (more infos below)
 - add different time outs for NPackets
 - game objects and transmission (seq numbered)
  -> synched send from server.

--- beware:
currently there is an extra part which "forgets" to send packages
or sends them twice. This will have to be eventually turned off
but it is very helpful to test if my code can actually handle it.
-> EVP_DEBUG

---- data request:
all over semi-reliable transmission
1. request: req id, file-name by string. can be interpreted as one wishes.
2. answer: req id, denied + reason / ack + num packages to be sent.
3. request: req id, ack
4. answer: injection of all packages, will possibly jam the queue!

also: need timeouts (in deconstructor of np_send / np_rcv)
      data splitting and unification at either end.

generic interface:
make request to a np_send, give a function to answer to with data/error
request handler at np_rcv -> takes file name, id of requester -> gets data / error
rest taken care of by system!

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

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <list>
#include <queue>
#include <map>
#include <time.h>


// ###############   DEBUG   #############
// ###############   DEBUG   #############
// ###############   DEBUG   #############
//#define EVP_DEBUG // will make send more unreliable.
float EVP_DEBUG_IMPAIRMENT = 0.1; // 0.1

// ############### DECLARATIONS #############
// ############### DECLARATIONS #############
// ############### DECLARATIONS #############
struct InputHandler;
struct EChat;
typedef std::function<void(std::string &)> EChat_input_function;


// header for NUDP...Packet
const uint16_t NET_HEADER_ECHO = 100; // just send a string
const uint16_t NET_HEADER_PING = 200; // ping - pong
const uint16_t NET_HEADER_PONG = 201;
const uint16_t NET_HEADER_CLIENT_ANNOUNCE = 2000; // announce the recieve port of client
const uint16_t NET_HEADER_SERVERCLIENT_IDPING = 3000; // server id ping -> as first message over client channel
const uint16_t NET_HEADER_NPacket = 4000; // the NPacket
const uint16_t NET_HEADER_GameObjects = 5000;
struct NUDPWritePacket; // and operators
struct NUDPReadPacket; // and operators


// NPacket -- The semi-reliable Packet
const uint16_t NPacket_TAG_CHAT = 1;
const uint16_t NPacket_TAG_DATAREQUEST_REQ = 2; // 1. request: req id, file-name by string. can be interpreted as one wishes.
const uint16_t NPacket_TAG_DATAREQUEST_ANS = 3; // 2. answer: req id, denied + reason / ack + num packages to be sent.
const uint16_t NPacket_TAG_DATAREQUEST_ACK = 4; // 3. request: req id, ack
const uint16_t NPacket_TAG_DATAREQUEST_DAT = 5; // 4. answer: injection of all packages, will possibly jam the queue!
const uint16_t NPacket_TAG_COMMAND = 6; // to be used by library user
struct NPacket;
typedef uint16_t NPSeq;
const NPSeq NPSeq_HALF = 32768;
const NPSeq NPSeq_START_AT = 32768 + 32750; // 0 if not debugging
struct NPacketDataRequest_Send;
struct NPacketDataRequest_Rcv;
// typedef std::function<void(std::string , NServerClient* , bool& , char*& , size_t &, std::string&)> DataReqHandler_t;
struct NPacketRcvHandler;
struct NPacketSendHandler;
typedef std::function<void(NPacket &, NPacketSendHandler &)> NPacket_tag_function;


// SERVER - CLIENT
struct NServerClient_identifier;
struct cmpByNSC_ident;
struct NServerClient;
struct NServer;
typedef std::function<void(NServerClient*, const std::string &)> ChatInputHandler_t;
typedef std::map<NServerClient_identifier, NServerClient*, cmpByNSC_ident> NSCMapIdentifyer;
typedef std::map<int, NServerClient*> NSCMapId;
struct NClient;

struct GameObject;
struct GameObjectHandler;

// ############### STATIC VARIABLES #############
// ############### STATIC VARIABLES #############
// ############### STATIC VARIABLES #############
float SCREEN_SIZE_X = 800;
float SCREEN_SIZE_Y = 600;

sf::Font FONT;

bool NET_SERVER_MODE;
NServer* NET_SERVER;
NClient* NET_CLIENT;

sf::Clock PROGRAM_CLOCK;

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

// ###############     DRAWING     ############
// ###############     DRAWING     ############
// ###############     DRAWING     ############
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


// ###############     INPUT     ############
// ###############     INPUT     ############
// ###############     INPUT     ############
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

  bool isActive = false; // true -> catches input, draw more opaque

  EChat_input_function input_handler = NULL; // called when line is entered.
  void setInputFunction(EChat_input_function f)
  {
    input_handler = f;
  }
  void draw(float x, float y, sf::RenderWindow &window)
  {
    if (isActive) {
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

    }

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
            isActive = false;
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

// ###########       NUDP...Packet     ################
// ###########       NUDP...Packet     ################
// ###########       NUDP...Packet     ################
struct NUDPWritePacket
{
  std::vector<char> data;
  size_t index = 0; // where to write next, may be beyond memory

  bool send(sf::UdpSocket &socket, sf::IpAddress ip, unsigned short port)
  {
    return (socket.send(data.data(), index, ip, port) == sf::Socket::Done);
  }

  void writePacket(NUDPWritePacket &p)
  {
    data.resize(index + p.index);
    std::memcpy(&(data[index]), p.data.data(), p.index);
    index += p.index;
  }

  void clear()
  {
    data.resize(0);
    index = 0;
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

NUDPWritePacket& operator << (NUDPWritePacket& p, const float& c){
  char* b = (char*)&c;

  p.data.resize(p.index+4);
  p.data[p.index] = b[0];
  p.data[p.index+1] = b[1];
  p.data[p.index+2] = b[2];
  p.data[p.index+3] = b[3];
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

NUDPReadPacket& operator >> (NUDPReadPacket& p, float& c){
  uint16_t b1 = p.data[p.index];
  uint16_t b2 = p.data[p.index+1];
  uint16_t b3 = p.data[p.index+2];
  uint16_t b4 = p.data[p.index+3];
  p.index+=4;

  char* b = (char*)&c;
  b[0] = b1;
  b[1] = b2;
  b[2] = b3;
  b[3] = b4;

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

// ################################# NPacket ########################
// ################################# NPacket ########################
// ################################# NPacket ########################
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

  sf::Int32 timeout;
  // book keeping:
  sf::Int32 time_sent; // for timeout and resend
};

// ################## NPacket DataRequest ################
// ################## NPacket DataRequest ################
// ################## NPacket DataRequest ################
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
  6 - timed out - not used
  */

  size_t dataSize;
  size_t packageSize;
  uint16_t num_packages;

  std::vector<bool> rcvd; // true if reveived that package
  char* data = NULL;
  bool free_data_in_deconstructor = false;  // usually user destroyed, maybe wants to keep?
  uint16_t num_packages_rcvd;

  NPacketDataRequest_Send(uint16_t id, std::string fileName, std::function<void(char*, size_t)> f_success, std::function<void(bool, std::string)> f_failiure)
  :id(id), fileName(fileName), f_success(f_success), f_failiure(f_failiure)
  {
    // free_data_in_deconstructor = true; // make deconstructor delete data, if any exists
    // now must do that in f_success!
    status = 1;
  }

  ~NPacketDataRequest_Send()
  {
    if(data != NULL && (free_data_in_deconstructor || status != 4))
    {
      if(status != 4)
      {
        // must have timed out!
        f_failiure(false, "Did not finish in time. status before timeout: " + std::to_string(status));
        status = 6;
      }
      // if status != 4 -> was not even given out over f_success ! -> must free here.
      std::free(data);
      data = NULL;
    }
  }

  void makeReady(size_t size, size_t psize, uint16_t num_p)
  {
    // set datafield for data transmission, conter, bitfield.
    dataSize = size;
    packageSize = psize;
    num_packages = num_p;
    status = 3;

    num_packages_rcvd = 0;
    rcvd = std::vector<bool>(num_packages, false);
    data = (char*)std::malloc(dataSize);
  }

  bool setData(size_t num_of_this_package, std::vector<char> &v)
  {
    // returns true if transmission complete
    num_packages_rcvd++;
    rcvd[num_of_this_package] = true;

    size_t v_size = packageSize;
    if(num_of_this_package == num_packages-1)
    {
      v_size = dataSize % packageSize;
    }
    if(v.size() != v_size)
    {
      std::cout << "setData: size mismatch !!!" << std::endl;
    }
    std::memcpy(&(data[num_of_this_package*packageSize]), v.data(), v_size);

    if(num_packages_rcvd == num_packages)
    {
      bool some_failed = false;
      for(size_t i; i<num_packages; i++)
      {
        if(!rcvd[i])
        {
          std::cout << "somehow package missing: " << i << std::endl;
          some_failed = true;
        }
      }

      if(some_failed)
      {
        f_failiure(true, "Count all here, check at least one missing !?");
      }else{
        status = 4;
        f_success(data, dataSize);
        return true;
      }
    }
    return false;
  }
};

struct NPacketDataRequest_Rcv
{
  uint16_t id;
  size_t dataSize;
  char* dataPtr = NULL;
  bool free_dataPtr_in_destructor = true;

  size_t packageSize;
  uint16_t num_packages;

  NPacketDataRequest_Rcv(uint16_t id, size_t dataSize, char* dataPtr);
  ~NPacketDataRequest_Rcv();
  void sendPackets(NPacketSendHandler &np_send);
};



typedef std::function<void(std::string , NServerClient* , bool& , char*& , size_t &, std::string&)> DataReqHandler_t;
std::function<void(NServerClient*, NPacket &)> NPacketCommandHandler;
struct NPacketRcvHandler // np_rcv
{
  NPSeq highest_seq_num_rcvd = NPSeq_START_AT;

  uint16_t packet_counter = 0;

  std::map<NPSeq, NPSeq> map_missing; // should really be a set...

  NServerClient* client;
  std::map<uint16_t,NPacketDataRequest_Rcv*> requestMap;

  // benchmarking:
  size_t num_packets_rcvd = 0; // UDP
  size_t num_subpackets_rcvd = 0; // NET HEADER
  size_t num_NPackets_rcvd = 0;
  size_t num_NPackets_double = 0;

  NPacketRcvHandler(NServerClient* client);
  ~NPacketRcvHandler();

  DataReqHandler_t f_dataRequestHandler;
  void setDataRequestHandler(DataReqHandler_t f_handler);

  void createAckMessage(NUDPWritePacket &p);
  bool hasNews; // if the ack situation changed since last createAckMessage

  //typedef void (*NPacket_tag_function)(NPacket &);
  NPacket_tag_function npacket_tag_functions_default = NULL;
  std::map<uint16_t, NPacket_tag_function> npacket_tag_functions_map;
  void incomingProcess(NPacket &np, NPacketSendHandler &np_send);
  void incoming(NUDPReadPacket &p, NPacketSendHandler &np_send);

  void setDefaultTagFunction(NPacket_tag_function f);
  void addTagFunction(uint16_t tag, NPacket_tag_function f);

  void drawDataRequests(float x, float y, sf::RenderWindow &window);

  void drawBenchmarking(float x, float y, sf::RenderWindow &window);
};


struct NPacketSendHandler // np_send
{
  NPSeq last_sequence_number = NPSeq_START_AT;
  NPSeq oldest_sequence_waiting = NPSeq_START_AT;

  sf::Int32 msgTimeOutLimit = 500; // then resend it
  sf::Int32 msgTimeOutLimit_chat = 300; // then resend it
  sf::Int32 msgTimeOutLimit_data = 2000; // then resend it
  int num_wait_ack_limit = 100; // this will affect the transmission speed!

  std::queue<NPacket> queue;

  std::map<NPSeq, NPacket> map_sent;

  sf::Int32 time_last_send;
  sf::Int32 minimum_send_delay = 100;

  // for benchmarking only:
  size_t num_packets_sent = 0; // udp-send
  size_t num_subpackets_sent = 0; // NET HEADER
  size_t num_NPackets_sent = 0; // semi-reliable
  size_t num_NPackets_resent = 0;

  NPacketSendHandler();
  ~NPacketSendHandler();

   // queue to this via enqueueUnreliable, sent in update with reliable ones.
  NUDPWritePacket periodicPacket;
  void enqueueUnreliable(NUDPWritePacket &p);

  int enqueue(uint16_t tag, const std::vector<char> &txt, sf::Int32 time_out = 0);
  void enqueueChat(const std::string &c);
  void update(NPacketRcvHandler &np_rcv, sf::UdpSocket &socket, const sf::IpAddress &ip, const unsigned short &port, bool forceUpdate = false);
  void readAcks(NUDPReadPacket &p);

  std::string missingToString();

  uint16_t dataRequestNextId = 0; // reasonably never wrap around and overlap.
  std::map<uint16_t,NPacketDataRequest_Send*> requestMap;
  void issueDataRequest(std::string fileName, std::function<void(char*, size_t)> f_success, std::function<void(bool, std::string)> f_failiure);
  void deniedDataRequest(uint16_t req_id, std::string errorMsg);
  void ackDataRequest(uint16_t req_id, uint16_t num_packages, size_t packageSize,size_t dataSize);
  void rcvdDataRequest(uint16_t req_id, size_t req_num_of_this_package, std::vector<char> &v);
  void drawDataRequests(float x, float y, sf::RenderWindow &window);

  void drawBenchmarking(float x, float y, sf::RenderWindow &window);
};

// ####################### SERVER CLIENT ###########################
// ####################### SERVER CLIENT ###########################
// ####################### SERVER CLIENT ###########################
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

  NServerClient(sf::IpAddress remoteIp, unsigned short remotePort, NServer *nserver, DataReqHandler_t f_dataReq);

  void sendIdPing(sf::UdpSocket &socket);
  void sendPing(sf::UdpSocket &socket);
  void sendPong(sf::UdpSocket &socket);
  void rcvPacket(NUDPReadPacket &p, sf::UdpSocket &socket);
  void update(sf::UdpSocket &socket, bool doUpdate);
  void draw(float x, float y, sf::RenderWindow &window);
};

struct NServer
{
  unsigned short socket_port = 54000;
  sf::UdpSocket socket;

  NSCMapIdentifyer map_identifyer;
  NSCMapId map_id;

  // server lets out synchronized packets -> makes broadcast Obj easyer
  sf::Int32 send_delay = 100;
  sf::Int32 last_send = 0;

  GameObjectHandler* gobj_handler;
  ChatInputHandler_t f_chatInputHandler;
  DataReqHandler_t f_dataReqHandler;

  EChat *chat;
  NServer(unsigned short listen_port, EChat *chat, ChatInputHandler_t f_chat, DataReqHandler_t f_dataReq);
  ~NServer();
  void broadcastChat(const std::string &s);
  void addClient(NServerClient* c);
  void removeClient(NServerClient* c);
  NServerClient* getClient(sf::IpAddress ip, unsigned short port);
  void draw(void* context, sf::RenderWindow &window);
  void update(void* context);
};

struct NClient
{
  sf::UdpSocket socket;
  unsigned short destination_port = 54000;
  sf::IpAddress recipient =  "1.2.3.4"; //"192.168.1.41";//

  unsigned short socket_port;

  NPacketRcvHandler np_rcv;
  NPacketSendHandler np_send;

  uint16_t id_on_server;

  EChat *chat;

  GameObjectHandler* gobj_handler;

  // ------------------------------------- STATE MACHINE start
  int finite_state = 0;
  // 0 - initial
  sf::Int32 last_announce; // repeated announcement !
  int number_announcements = 0;

  // 1 - got answer from server -> id
  sf::Int32 last_rcv;

  // ------------------------------------- STATE MACHINE end
  NClient(sf::IpAddress server_ip, unsigned short server_port, EChat *chat, DataReqHandler_t f_dataReqHandler);
  ~NClient();
  void draw(void* context, sf::RenderWindow &window);
  void update(void* context);
  void announcePort();
  void sendPing();
  void sendPong();
};

// ############################## GAME OBJECTS ##############################
// ############################## GAME OBJECTS ##############################
// ############################## GAME OBJECTS ##############################
struct GameObject
{
  size_t id;
  uint16_t type;
  bool isDead = false; // true on server only
  sf::Int32 _timeout;
  // for timeout:
  // server: time of death
  // client: since last rcv

  virtual void toPacket(NUDPWritePacket& p) = 0;
  virtual void fromPacket(NUDPReadPacket& p) = 0;

  virtual void draw(void* context, sf::RenderWindow &window) = 0;
  virtual void updateServer(void* context) = 0;
  virtual void updateClient(void* context) = 0;

  void killServer()
  {
    if(!isDead)
    {
      isDead = true;
      _timeout = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
    }
  }
};

std::function<GameObject*(size_t, uint16_t)> new_GameObject;
struct GameObjectHandler
{
  NPSeq last_seq = 0;
  // tick up on send.
  // only rcv if higher seq.
  std::map<size_t, GameObject*> objects_map;

  GameObjectHandler()
  {

  }
  ~GameObjectHandler()
  {
    std::map<size_t, GameObject*>::iterator it;
    for (it=objects_map.begin(); it!=objects_map.end(); ++it)
  	{
  		GameObject* obj = it->second;

      delete(obj);
    }
    objects_map.clear();
  }

  void populate_debug()
  {
    for(size_t i = 1; i<101; i++)
    {
      GameObject* obj = new_GameObject(i, 0);
      objects_map.insert(
        std::pair<size_t, GameObject*>
        (obj->id,obj)
      );
    }
  }

  void toPacket(NUDPWritePacket& p)
  {
    // sequence
    last_seq++;
    p << last_seq;

    // seperate the objects first (so know number of each section):
    uint16_t count_normal = 0;
    NUDPWritePacket p_normal;

    uint16_t count_dead = 0;
    NUDPWritePacket p_dead;

    std::map<size_t, GameObject*>::iterator it;
    for (it=objects_map.begin(); it!=objects_map.end(); ++it)
  	{
  		GameObject* obj = it->second;

      if(obj->isDead)
      {
        p_dead << obj->id;
        count_dead++;
      }else{
        obj->toPacket(p_normal);
        count_normal++;
      }
    }

    // copy the sections to packet:
    p << count_normal;
    p.writePacket(p_normal);

    p << count_dead;
    p.writePacket(p_dead);
  }

  void fromPacket(NUDPReadPacket& p)
  {
    sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
    // see if really need to do update:
    NPSeq seq;
    p >> seq;
    if(sequence_greater_than(seq, last_seq))
    {
      last_seq = seq;
    }else{
      std::cout << "seq too low" << std::endl;
      return;
    }

    uint16_t count_normal;
    p >> count_normal;
    for(uint16_t i = 0; i<count_normal; i++)
    {
      size_t id_rcvd;
      uint16_t type_rcvd;
      p >> id_rcvd >> type_rcvd;

      std::map<size_t, GameObject*>::iterator it;
      it = objects_map.find(id_rcvd);

      if(it != objects_map.end())
      {
        // found
        GameObject* obj = it->second;
        obj->fromPacket(p);
        obj->_timeout = now_time;
      }else{
        // not found -> create:
        GameObject* obj = new_GameObject(id_rcvd, type_rcvd);
        obj->fromPacket(p);
        objects_map.insert(
          std::pair<size_t, GameObject*>
          (obj->id,obj)
        );
        obj->_timeout = now_time;
      }
    }

    uint16_t count_dead;
    p >> count_dead;
    for(uint16_t i = 0; i<count_dead; i++)
    {
      size_t id_rcvd;
      p >> id_rcvd;

      std::map<size_t, GameObject*>::iterator it;
      it = objects_map.find(id_rcvd);

      if(it != objects_map.end())
      {
        // found
        GameObject* obj = it->second;
        objects_map.erase(it);
        delete(obj);
      }else{
        // not found -> ignore
      }
    }
  }

  void draw(void* context, sf::RenderWindow &window)
  {
    DrawText(700, 500, "num gobj: " + std::to_string(objects_map.size()), 12, window, sf::Color(255,0,255));
    std::map<size_t, GameObject*>::iterator it;
    for (it=objects_map.begin(); it!=objects_map.end(); ++it)
  	{
  		GameObject* obj = it->second;
      if (!obj->isDead) {
        obj->draw(context, window);
      }
    }
  }

  void updateServer(void* context)
  {
    sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

    std::vector<uint16_t> deletees;

    std::map<size_t, GameObject*>::iterator it;
    for (it=objects_map.begin(); it!=objects_map.end(); ++it)
  	{
  		GameObject* obj = it->second;
      if (!obj->isDead) {
        obj->updateServer(context);
      }else if(now_time - obj->_timeout > 2000){
        // timeout after death -> delete it for good:
        deletees.push_back(obj->id);
        delete(obj);
      }
    }

    for (size_t i = 0; i < deletees.size(); i++) {
      objects_map.erase(deletees[i]);
    }
  }

  void updateClient(void* context)
  {
    sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

    std::vector<uint16_t> deletees;

    std::map<size_t, GameObject*>::iterator it;
    for (it=objects_map.begin(); it!=objects_map.end(); ++it)
  	{
  		GameObject* obj = it->second;

      if(now_time - obj->_timeout > 2000)
      {
        // object timed out -> delete:
        deletees.push_back(obj->id);
        delete(obj);
      }else{
        obj->updateClient(context);
      }
    }

    for (size_t i = 0; i < deletees.size(); i++) {
      objects_map.erase(deletees[i]);
    }
  }
};
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

// ########################################### NPacketDataRequest_Send ##########
// ########################################### NPacketDataRequest_Rcv ##########
// ########################################### NPacketDataRequest_Rcv ##########
NPacketDataRequest_Rcv::NPacketDataRequest_Rcv(uint16_t id, size_t dataSize, char* dataPtr)
: id(id), dataSize(dataSize), dataPtr(dataPtr)
{
  // free_dataPtr_in_destructor = false; // keep data
  // currently dataPtr is freed in deconstructor.

  const size_t package_size = 1024*4; //1024
  num_packages = dataSize / package_size;
  if(dataSize % package_size !=0)
  {
    num_packages++;
  }
  packageSize = package_size;
}

NPacketDataRequest_Rcv::~NPacketDataRequest_Rcv()
{
  if(dataPtr != NULL && free_dataPtr_in_destructor)
  {
    std::free(dataPtr);
    dataPtr = NULL;
  }
}

void NPacketDataRequest_Rcv::sendPackets(NPacketSendHandler &np_send)
{
  // for now just naively submitt all to queue immediately
  for(size_t i = 0; i<num_packages; i++)
  {
    size_t v_size = packageSize;
    if(i == num_packages-1)
    {
      v_size = dataSize % packageSize;
    }
    std::vector<char> v;
    v.resize(v_size);
    std::memcpy(v.data(), &(dataPtr[i*packageSize]), v_size);

    NUDPWritePacket p;
    p << id; // req_id
    p << i; // num of this package
    p << v; // data of package

    np_send.enqueue(NPacket_TAG_DATAREQUEST_DAT, p.data, np_send.msgTimeOutLimit_data);
  }
}



// ################### NPacketRcvHandler ###################
// ################### NPacketRcvHandler ###################
// ################### NPacketRcvHandler ###################
NPacketRcvHandler::NPacketRcvHandler(NServerClient* client): client(client)
{
  auto f = [](std::string fileName, NServerClient* client, bool& success, char* &data, size_t &data_size, std::string& errorMsg)
  {
    std::cout << "[RH] DATA REQUEST -> not set right?" << std::endl;

    success = false;
    errorMsg = "the data req handler has not been set properly !";
  };
  setDataRequestHandler(f);


  // ------------- set dataRequest packet handlers:

  auto f_req = [this, client](NPacket &np, NPacketSendHandler &np_send)
  {
    // reading packet in np.data:
    NUDPReadPacket p;
    p.setFromVector(np.data);

    uint16_t req_id;
    std::string fileName;
    p >> req_id;
    p >> fileName;

    // now see if data available:
    bool success;
    char* dataPtr;
    size_t dataSize;
    std::string errorMsg;
    f_dataRequestHandler(fileName, client, success, dataPtr, dataSize, errorMsg);

    if(success)
    {
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
      req = NULL;
    }else{
      std::cout << "DID NOT FIND THE ID ...???" << std::endl;
    }
  };

  auto f_dat = [](NPacket &np, NPacketSendHandler &np_send)
  {
    NUDPReadPacket p;
    p.setFromVector(np.data);

    uint16_t req_id;
    size_t req_num_of_this_package;
    std::vector<char> v;
    p >> req_id;
    p >> req_num_of_this_package;
    p >> v;

    np_send.rcvdDataRequest(req_id, req_num_of_this_package, v);
  };

  addTagFunction(NPacket_TAG_DATAREQUEST_REQ, f_req);
  addTagFunction(NPacket_TAG_DATAREQUEST_ANS, f_ans);
  addTagFunction(NPacket_TAG_DATAREQUEST_ACK, f_ack);
  addTagFunction(NPacket_TAG_DATAREQUEST_DAT, f_dat);

  auto f_cmd = [client](NPacket &np, NPacketSendHandler &np_send)
  {
    NPacketCommandHandler(client, np);
  };
  addTagFunction(NPacket_TAG_COMMAND, f_cmd);
}

NPacketRcvHandler::~NPacketRcvHandler()
{
  std::map<uint16_t, NPacketDataRequest_Rcv*>::iterator it;
	for (it=requestMap.begin(); it!=requestMap.end(); ++it)
	{
		NPacketDataRequest_Rcv* req = it->second;
    delete(req);
  }
  requestMap.clear();
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

    num_NPackets_rcvd++;
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
        num_NPackets_double++;
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

void NPacketRcvHandler::drawDataRequests(float x, float y, sf::RenderWindow &window)
{
  std::map<uint16_t, NPacketDataRequest_Rcv*>::iterator it;
	for (it=requestMap.begin(); it!=requestMap.end(); ++it)
	{
		NPacketDataRequest_Rcv* req = it->second;
    DrawText(x,y,
      "ID: " + std::to_string(req->id) +
      ", size: " + std::to_string(req->dataSize) +
      ", #pack: " + std::to_string(req->num_packages),
      12, window, sf::Color(100,100,100)
    );

    y+=15;
	}

}

void NPacketRcvHandler::drawBenchmarking(float x, float y, sf::RenderWindow &window)
{
  DrawText(x,y,
      "UDP: " + std::to_string(num_packets_rcvd),
      12, window, sf::Color(0,255,255)
  );

  DrawText(x+120,y,
      "NET: " + std::to_string(num_subpackets_rcvd),
      12, window, sf::Color(0,255,255)
  );

  DrawText(x+240,y,
      "NP rcvd: " + std::to_string(num_NPackets_rcvd),
      12, window, sf::Color(0,255,255)
  );

  DrawText(x+360,y,
      "NP double: " + std::to_string(num_NPackets_double),
      12, window, sf::Color(0,255,255)
  );

  DrawText(x+480,y,
      "NP eff.cy: " + std::to_string(1.0 - (float)num_NPackets_double /(float)num_NPackets_rcvd),
      12, window, sf::Color(0,255,255)
  );
}



// ########################################### NPacketSendHandler ##########
// ########################################### NPacketSendHandler ##########
// ########################################### NPacketSendHandler ##########
NPacketSendHandler::NPacketSendHandler()
{
  time_last_send = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
}
NPacketSendHandler::~NPacketSendHandler()
{
  std::map<uint16_t, NPacketDataRequest_Send*>::iterator it;
	for (it=requestMap.begin(); it!=requestMap.end(); ++it)
	{
		NPacketDataRequest_Send* req = it->second;
    delete(req);
  }
  requestMap.clear();
}

void NPacketSendHandler::enqueueUnreliable(NUDPWritePacket &p)
{
  periodicPacket.writePacket(p);
}

int NPacketSendHandler::enqueue(uint16_t tag, const std::vector<char> &txt, sf::Int32 time_out)
{
  // returns sequence number of the Packet
  last_sequence_number++;

  if(time_out == 0)
  {
    time_out = msgTimeOutLimit;
  }
  queue.push(NPacket{last_sequence_number, tag, txt, time_out});

  return queue.size();
}

void NPacketSendHandler::enqueueChat(const std::string &c)
{
  std::vector<char> v;
  v.resize(c.size());
  std::memcpy(v.data(), c.data(), c.size());

  enqueue(NPacket_TAG_CHAT, v, msgTimeOutLimit_chat);
}

void NPacketSendHandler::update(NPacketRcvHandler &np_rcv, sf::UdpSocket &socket, const sf::IpAddress &ip, const unsigned short &port, bool forceUpdate)
{
  sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

  if(((now_time - time_last_send)>minimum_send_delay || forceUpdate) && (queue.size() > 0 || map_sent.size() < num_wait_ack_limit || np_rcv.hasNews || periodicPacket.index > 0))
  {
    time_last_send = now_time;
    // something to send:
    //NUDPWritePacket p; // ------------------ simply add to it!
    periodicPacket << NET_HEADER_NPacket;
    num_subpackets_sent++;

    np_rcv.createAckMessage(periodicPacket);

    // update from map (the ones that have timed out)
    std::map<NPSeq, NPacket>::iterator it;

    for (it=map_sent.begin(); it!=map_sent.end(); ++it)
    {
      NPSeq seq = it->first;
      sf::Int32 sent_time = it->second.time_sent;
      if( (now_time - sent_time) > it->second.timeout)
      {
        #ifdef EVP_DEBUG
          if( ((float) rand() / (RAND_MAX)) > EVP_DEBUG_IMPAIRMENT )
          {
            periodicPacket << it->second.seq << it->second.tag << it->second.data;

            if( ((float) rand() / (RAND_MAX)) < EVP_DEBUG_IMPAIRMENT )
            {
              periodicPacket << it->second.seq << it->second.tag << it->second.data; // send double!
            }
          }
        #else
          periodicPacket << it->second.seq << it->second.tag << it->second.data;
        #endif

        num_NPackets_resent++;

        it->second.time_sent = now_time;
      }
    }

    // append new ones
    while(queue.size() > 0 && map_sent.size() < num_wait_ack_limit && periodicPacket.index < 50000)
    {
      // send from queue, append to map
      NPacket np = queue.front();
      queue.pop();

      np.time_sent = now_time;

      #ifdef EVP_DEBUG
        // artificially loose and double some packets:
        if( ((float) rand() / (RAND_MAX)) > EVP_DEBUG_IMPAIRMENT )
        {
          periodicPacket << np.seq << np.tag << np.data;

          if( ((float) rand() / (RAND_MAX)) < EVP_DEBUG_IMPAIRMENT )
          {
            periodicPacket << np.seq << np.tag << np.data; // send double!
          }
        }
      #else
        periodicPacket << np.seq << np.tag << np.data;
      #endif
      num_NPackets_sent++;

      map_sent.insert(
        std::pair<NPSeq, NPacket>
        (np.seq, np)
      );
    }

    // send packet:
    num_packets_sent++;
    if(!periodicPacket.send(socket,ip, port))//socket.send(p, ip, port) != sf::Socket::Done)
    {
        std::cout << "Error on send ! (NPacket)" << std::endl;
    }
    periodicPacket.clear();
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
    //std::cout << "WRAP AROUND DANGER!" << std::endl;

    size_t i = 0; // index in missingWrapVec

    do {
      // work through rest of high elements:
      NPSeq seq = it->first;
      //std::cout << seq << std::endl;

      while(i<missingWrapVec.size() && seq > missingWrapVec[i])
      {i++;}

      if(i>=missingWrapVec.size() || seq < missingWrapVec[i])
      {
        // can delete:
        deletees.push_back(seq);
        //std::cout << "del." << std::endl;
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
  std::map<uint16_t, NPacketDataRequest_Send*>::iterator it;

	it = requestMap.find(req_id);

	if(it != requestMap.end()){
		NPacketDataRequest_Send * req = it->second;
    req->status = 5;
    req->f_failiure(true, errorMsg);

    requestMap.erase(it);
    delete(req);
    req = NULL;
	}else{
		// not found! not sure what to do!
    std::cout << "DID NOT FIND THE DENIED REQUEST ANYWAY...???" << std::endl;
	}
}
void NPacketSendHandler::ackDataRequest(uint16_t req_id, uint16_t num_packages, size_t packageSize,size_t dataSize)
{
  std::map<uint16_t, NPacketDataRequest_Send*>::iterator it;

	it = requestMap.find(req_id);

	if(it != requestMap.end()){
		NPacketDataRequest_Send * req = it->second;
    req->makeReady(dataSize, packageSize, num_packages);
  }else{
		// not found! not sure what to do!
    std::cout << "DID NOT FIND THE ACKED REQUEST ...???" << std::endl;
	}
}

void NPacketSendHandler::rcvdDataRequest(uint16_t req_id, size_t req_num_of_this_package, std::vector<char> &v)
{
  std::map<uint16_t, NPacketDataRequest_Send*>::iterator it;

	it = requestMap.find(req_id);

	if(it != requestMap.end()){
		NPacketDataRequest_Send * req = it->second;
    bool transmissionDone = req->setData(req_num_of_this_package, v);
    if(transmissionDone)
    {
      requestMap.erase(it);
      delete(req);
      req = NULL;
    }
  }else{
		// not found! not sure what to do!
    std::cout << "DID NOT FIND THE REQUEST ...??? -> cannot rcv data." << std::endl;
	}
}

void NPacketSendHandler::drawDataRequests(float x, float y, sf::RenderWindow &window)
{
  std::map<uint16_t, NPacketDataRequest_Send*>::iterator it;
	for (it=requestMap.begin(); it!=requestMap.end(); ++it)
	{
		NPacketDataRequest_Send* req = it->second;

    std::string statusComment = "";
    if(req->status == 3)
    {
      statusComment = ", " + std::to_string(req->num_packages_rcvd) + " of " + std::to_string(req->num_packages);
    }
    DrawText(x,y,
      "ID: " + std::to_string(req->id) + ", name: " + req->fileName + ", status:" + std::to_string(req->status) + statusComment,
      12, window, sf::Color(100,100,100)
    );

    y+=15;
	}

}

void NPacketSendHandler::drawBenchmarking(float x, float y, sf::RenderWindow &window)
{
  DrawText(x,y,
      "UDP: " + std::to_string(num_packets_sent),
      12, window, sf::Color(255,255,255)
  );

  DrawText(x+120,y,
      "NET: " + std::to_string(num_subpackets_sent),
      12, window, sf::Color(255,255,255)
  );

  DrawText(x+240,y,
      "NP send: " + std::to_string(num_NPackets_sent),
      12, window, sf::Color(255,255,255)
  );

  DrawText(x+360,y,
      "NP resend: " + std::to_string(num_NPackets_resent),
      12, window, sf::Color(255,255,255)
  );

  DrawText(x+480,y,
      "NP eff.cy: " + std::to_string(1.0 - (float)num_NPackets_resent /(float)(num_NPackets_sent + num_NPackets_resent)),
      12, window, sf::Color(255,255,255)
  );
}

// ###################################### NServerClient ######################################
// ###################################### NServerClient ######################################
// ###################################### NServerClient ######################################
NServerClient::NServerClient(sf::IpAddress remoteIp, unsigned short remotePort, NServer *nserver, DataReqHandler_t f_dataReq)
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
    nserver->f_chatInputHandler(this, cnew);
  };
  np_rcv.setDefaultTagFunction(f_default);
  np_rcv.addTagFunction(NPacket_TAG_CHAT, f_chat);
  np_rcv.setDataRequestHandler(f_dataReq);
}

void NServerClient::sendIdPing(sf::UdpSocket &socket)
{
  NUDPWritePacket p;

  p << NET_HEADER_SERVERCLIENT_IDPING << id;
  np_send.num_subpackets_sent++;
  np_send.num_packets_sent++;
  if(!p.send(socket,remoteIp, remotePort))//if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}

void NServerClient::sendPing(sf::UdpSocket &socket)
{
  NUDPWritePacket p;
  p << NET_HEADER_PING;
  np_send.num_subpackets_sent++;
  np_send.num_packets_sent++;
  if(!p.send(socket,remoteIp, remotePort))//if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}

void NServerClient::sendPong(sf::UdpSocket &socket)
{
  NUDPWritePacket p;
  p << NET_HEADER_PONG;
  np_send.num_subpackets_sent++;
  np_send.num_packets_sent++;
  if(!p.send(socket,remoteIp, remotePort))//if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}

void NServerClient::rcvPacket(NUDPReadPacket &p, sf::UdpSocket &socket)
{
  last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

  np_rcv.num_packets_rcvd++;

  while(!p.endOfPacket())
  {
    np_rcv.num_subpackets_rcvd++;
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
}

void NServerClient::update(sf::UdpSocket &socket, bool doUpdate)
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

  if(doUpdate)
  {
    np_send.update(np_rcv, socket, remoteIp, remotePort, true);
  }
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

  np_send.drawDataRequests(x+50,y +45, window);
  np_rcv.drawDataRequests(x+250,y+45, window);
  np_send.drawBenchmarking(x+50,y +15, window);
  np_rcv.drawBenchmarking(x+50,y +30, window);
}

int NServerClient::id_last = 0;

// ########################## NServer ########################################
// ########################## NServer ########################################
// ########################## NServer ########################################
NServer::NServer(unsigned short listen_port, EChat *chat, ChatInputHandler_t f_chat, DataReqHandler_t f_dataReq)
: chat(chat), f_chatInputHandler(f_chat), f_dataReqHandler(f_dataReq), socket_port(listen_port)
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
    f_chatInputHandler(NULL, s);
  };
  chat->setInputFunction(f_input);

  // ------------ game stuff:
  gobj_handler = new GameObjectHandler();
  gobj_handler->populate_debug();
}

NServer::~NServer()
{
  std::map<int, NServerClient*>::iterator it;
	for (it=map_id.begin(); it!=map_id.end(); ++it)
	{
		NServerClient* c = it->second;
    delete(c);
	}

  map_identifyer.clear();
  map_id.clear();

  delete(gobj_handler);
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

void NServer::draw(void* context, sf::RenderWindow &window)
{
  // ----------- draw GAME
  gobj_handler->draw(context, window);

  // ----------- draw HUD

  DrawText(5,5, "Server Mode.", 12, window, sf::Color(255,0,0));

  // draw all clients:
  std::map<int, NServerClient*>::iterator it;

  int counter = 0;
	for (it=map_id.begin(); it!=map_id.end(); ++it)
	{
		NServerClient* c = it->second;

    c->draw(10, 20+counter*95,window);

    counter++;
	}
}

void NServer::update(void* context)
{
  bool repeat;

  do
  {
    NUDPReadPacket p;
    sf::IpAddress remoteIp;
    unsigned short remotePort;

    bool success = p.receive(socket, remoteIp, remotePort);

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
          c = new NServerClient(remoteIp, remotePort, this, f_dataReqHandler);
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

  // update GAME:
  gobj_handler->updateServer(context);

  // update all clients:
  sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

  bool doUpdate = false;
  NUDPWritePacket gobj_packet;

  if(now_time - last_send > send_delay)
  {
    last_send = now_time;
    doUpdate = true;

    // create gobj_packet:
    gobj_packet << NET_HEADER_GameObjects;

     // the indirection allows for easyer discarding on seq.
    NUDPWritePacket temp;
    gobj_handler->toPacket(temp);
    gobj_packet << temp.data;
  }

  std::map<int, NServerClient*>::iterator it;
	for (it=map_id.begin(); it!=map_id.end(); ++it)
	{
		NServerClient* c = it->second;

    if((now_time - c->last_rcv) > 10000)
    {
      // timed out
      std::cout << "timeout " << c->id << std::endl;

      removeClient(c);
    }

    if(doUpdate)
    {
      c->np_send.periodicPacket.writePacket(gobj_packet);
    }
    c->update(socket, doUpdate);
	}

}


// ############################## NClient ##############################
// ############################## NClient ##############################
// ############################## NClient ##############################
NClient::NClient(sf::IpAddress server_ip, unsigned short server_port, EChat *chat, DataReqHandler_t f_dataReqHandler)
: np_rcv(NPacketRcvHandler(NULL)), recipient(server_ip), destination_port(server_port)
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
  np_rcv.setDataRequestHandler(f_dataReqHandler);

  // ------------ GAME:
  gobj_handler = new GameObjectHandler();
}

NClient::~NClient()
{
  delete(gobj_handler);
}

void NClient::draw(void* context, sf::RenderWindow &window)
{
  // -------------- GAME:
  gobj_handler->draw(context, window);

  // --------------  HUD
  DrawText(5,5, "Client Mode. connected to " + recipient.toString() + ":" + std::to_string(destination_port), 12, window, sf::Color(0,255,0));

  DrawText(5,20, "State: " + std::to_string(finite_state) + ", ID: " + std::to_string(id_on_server), 12, window, sf::Color(0,255,0));

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

  np_send.drawDataRequests(5,140, window);
  np_rcv.drawDataRequests(200, 140, window);
  np_send.drawBenchmarking(5, 100, window);
  np_rcv.drawBenchmarking(5, 120, window);
}

void NClient::update(void* context)
{
  bool repeat = false;
  do
  {
    NUDPReadPacket p;
    sf::IpAddress remoteIp;
    unsigned short remotePort;

    bool success = p.receive(socket, remoteIp, remotePort);

    repeat = false;
    if(success)
    {
      last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
      np_rcv.num_packets_rcvd++;

      while(!p.endOfPacket())
      {
        np_rcv.num_subpackets_rcvd++;

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
          case NET_HEADER_GameObjects:
            {
               // the indirection allows for easyer discarding on seq.
              NUDPReadPacket temp;
              std::vector<char> v;
              p >> v;
              temp.setFromVector(v);
              gobj_handler->fromPacket(temp);
              break;
            }
          default:   // ------------ DEFAULT
            std::cout << "unreadable header: "<< header << std::endl;
            break;
        }
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

  gobj_handler->updateClient(context);
}

void NClient::announcePort()
{
  NUDPWritePacket p;
  p << NET_HEADER_CLIENT_ANNOUNCE << socket_port;
  np_send.num_subpackets_sent++;
  np_send.num_packets_sent++;
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
  np_send.num_subpackets_sent++;
  np_send.num_packets_sent++;
  if( !p.send(socket, recipient, destination_port))//if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}

void NClient::sendPong()
{
  NUDPWritePacket p;
  p << NET_HEADER_PONG;
  np_send.num_subpackets_sent++;
  np_send.num_packets_sent++;
  if( !p.send(socket, recipient, destination_port))//if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
  {
      std::cout << "Error on send !" << std::endl;
  }
}
