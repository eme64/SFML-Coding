#include <iostream>
#include <fstream>
#include <vector>
#include <regex>

class Token{
public:
  enum Type {
    None,
    Space,//used to separate tokens
    Sequencer,//used to separate steps
    Comment,//used to cut away rest of line
    Name,
    BracketOpen,BracketClose,
    String,Float,Integer,
    Assign,Comma,Dot,
    BinaryOperator,
  };
  const char* typeString() const {
    typeString(_type);
  }

  static const char* typeString(Token::Type const type) {
    switch (type) {
      case Type::None: return "None";
      case Type::Space: return "Space";
      case Type::Sequencer: return "Sequencer";
      case Type::Comment: return "Comment";
      case Type::Name: return "Name";
      case Type::BracketOpen: return "BracketOpen";
      case Type::BracketClose: return "BracketClose";
      case Type::String: return "String";
      case Type::Float: return "Float";
      case Type::Integer: return "Integer";
      case Type::Assign: return "Assign";
      case Type::BinaryOperator: return "BinaryOperator";
      case Type::Comma: return "Comma";
      case Type::Dot: return "Dot";
      default: return "Unknown";
    }
  }
  const char* findCloseBracket() const {// for BracketOpen
    if (_content=="(") {return ")";}
    if (_content=="[") {return "]";}
    if (_content=="{") {return "}";}
    return "Unknown";
  }

  Token() : _content("generic"), _type(Token::Type::None), _file("None"), _line(0), _lineNum(0), _column(0){}
  Token(
    std::string content, Token::Type type,
    std::string file, std::string line,
    int lineNum, int column
  ):  _content(content), _type(type), _file(file), _line(line), _lineNum(lineNum), _column(column){}

  std::string toString() const {
    return _content;
  }
  Token::Type type() const {return _type;}
  void printContext()const {
    std::cout <<_file<<":"<<_lineNum<<":"<<_column<< ": Token \'" << _content << "\' of type " << typeString() << std::endl;
    std::cout << _line << std::endl;
    std::cout << std::string(_column,' ') << "^" << std::endl;
  }
private:
  const std::string _content;
  const Type _type;

  // token origin:
  const std::string _file;
  const std::string _line;
  const int _lineNum;
  const int _column;
};

class Tokenizer{
public:
  Tokenizer(std::string fileName):_file(fileName){
    std::cout << "[Tokenizer] init from file: " << fileName << std::endl;
    std::ifstream file(fileName);
    codeLines.resize(0);
    std::string line;
    while(std::getline(file, line))
    {
      codeLines.push_back(line);
    }
  }
  void printLines(){
    int i=0;
    for(auto const& line: codeLines) {
      std::cout << "[printLines] "<< i << ": '" << line << "'"<< std::endl;
      i++;
    }
  }
  void tokenize(){
    _tokens.resize(0);
    tokenErrors.resize(0);
    int i=0;
    for(auto const& line: codeLines) {
      std::pair<std::vector<Token*>,std::string> res = tokenize(line,i);
      std::vector<Token*> tVec = res.first;
      std::string text = res.second;
      if (text!="") {
        std::string error = std::to_string(i)+":"+text;
        tokenErrors.push_back(error);
        std::cout << "[tokenize] [Error] " << error << std::endl;
      }
      _tokens.reserve(_tokens.size() + tVec.size());
      _tokens.insert(_tokens.end(),tVec.begin(),tVec.end());
      i++;
    }
  }

  std::pair<std::vector<Token*>,std::string> tokenize(std::string const &line, int const lineNum){
    std::string text = line;

    bool lineHasEndSeparator = true;
    if(text[text.size()-1]=='\\'){
        lineHasEndSeparator = false;
        text = text.substr(0,text.size()-1);
    }

    std::vector<std::pair<Token::Type,std::regex>> reList {
      {Token::Type::Space, std::regex("^(\\s+)(.*)")},
      {Token::Type::Comment, std::regex("^(#)(.*)")},
      {Token::Type::Name, std::regex("^([a-zA-Z][\\w]*)(.*)")},
      {Token::Type::BracketOpen, std::regex("^(\\(|\\[|\\{)(.*)")},
      {Token::Type::BracketClose, std::regex("^(\\)|\\]|\\})(.*)")},
      {Token::Type::String, std::regex("^\"(.*)\"(.*)")},
      {Token::Type::Float, std::regex("^(\\d+\\.\\d*)(.*)")},
      {Token::Type::Integer, std::regex("^(\\d+)(.*)")},
      {Token::Type::Assign, std::regex("^(=)(.*)")},
      {Token::Type::BinaryOperator, std::regex("^(-|\\+)(.*)")},
      {Token::Type::Comma, std::regex("^(,)(.*)")},
      {Token::Type::Dot, std::regex("^(\\.)(.*)")},
      {Token::Type::Sequencer, std::regex("^(;)(.*)")},
    };

    std::vector<Token*> tVec;
    std::smatch m;

    bool someMatch;
    do {
      someMatch = false;

      for(auto const& re: reList){
        int parsePos = line.size()-text.size()-(lineHasEndSeparator?0:1);
        if (std::regex_search(text,m,re.second)){
          someMatch = true;
          std::string t = m[1];
          text = m[2];
          if(re.first==Token::Type::Space){break;}
          if(re.first==Token::Type::Comment){return std::make_pair(tVec,"");}
          tVec.push_back(new Token(t,re.first,_file,line,lineNum,parsePos));
          break;// restart for
        }
      }
    } while(someMatch);

    if(lineHasEndSeparator){
      tVec.push_back(new Token("",Token::Type::Sequencer,_file,line,lineNum,line.size()));
    }

    return std::make_pair(tVec,text);
  }

  void printTokens() const {
    int i=0;
    for(const Token* token: _tokens) {
      token->printContext();
      //std::cout << "[printTokens] "<< i << ": " << token->typeString() << ": "<< token->toString() << std::endl;
      i++;
    }
  }

  const std::vector<Token*> tokens(){return _tokens;}
private:
  std::vector<std::string> codeLines;
  std::vector<Token*> _tokens;
  std::vector<std::string> tokenErrors;
  const std::string _file;
};

class TokenTree{
public:
  TokenTree(Token* const token)
  :_token(token),_isTerminal(true){}
  TokenTree(std::vector<Token*> const &tokens)
  :_token(NULL),_isTerminal(false){
    _children.resize(0);
    _children.reserve(tokens.size());
    for (size_t i = 0; i < tokens.size(); i++) {
      _children.push_back(new TokenTree(tokens[i]));
    }
  }
  TokenTree(std::vector<TokenTree*> const &children)
  :_token(NULL),_isTerminal(false),_children(children){}

  bool process(int const step=0) {
    if (step>2) {return true;}
    if (_children.size()==0) {return true;}
    //std::cout << "[TokenTree][processGo] pre "<< step << std::endl;
    //print(0);
    for (TokenTree* const t:_children) {
      bool res = t->process(step+1);
      if (not res) {return false;}
    }
    bool res = processStep(step);
    if (not res) {return false;}
    //std::cout << "[TokenTree][processGo] post "<< step << std::endl;
    //print(0);
    return process(step+1);
  }

  bool processStep(int const step){
    switch (step) {
      case 0: return processBrackets();
      case 1: return processBinaryOperator(Token::Type::Sequencer);
      case 2: return processBinaryOperator(Token::Type::Assign);
    }
  }

  bool processBrackets(){
    std::stack<std::vector<TokenTree*>> childrenStack;
    std::stack<TokenTree*> openTokenStack;
    childrenStack.push(std::vector<TokenTree*>());

    for (size_t i = 0; i < _children.size(); i++) {
      TokenTree* t = _children[i];
      if (t->token() and t->token()->type()==Token::Type::BracketOpen) {
        //std::cout << "open " << t->token()->toString() << std::endl;
        childrenStack.push(std::vector<TokenTree*>());
        openTokenStack.push(t);
      }else if (t->token() and t->token()->type()==Token::Type::BracketClose) {
        //std::cout << "close " << t->token()->toString() << std::endl;
        if (openTokenStack.empty()) {
          std::cout << "[TokenTree][process][Error] too many close brackets." << std::endl;
          t->token()->printContext();
          return false;
        }
        TokenTree* openMatch = openTokenStack.top();
        openTokenStack.pop();
        if (openMatch->token()->findCloseBracket()==t->token()->toString()) {
          TokenTree* innerNode = new TokenTree(childrenStack.top());
          childrenStack.pop();
          innerNode->process();
          TokenTree* outerNode = new TokenTree(std::vector<TokenTree*>{
            openMatch,innerNode,t
          });
          childrenStack.top().push_back(outerNode);
        }else{
          std::cout << "[TokenTree][process][Error] Bracket mismatch." << std::endl;
          std::cout << "[TokenTree][process][Error] opened: " << openMatch->token()->toString() << std::endl;
          openMatch->token()->printContext();
          std::cout << "[TokenTree][process][Error] close expect: " << openMatch->token()->findCloseBracket() << std::endl;
          std::cout << "[TokenTree][process][Error] close given: " << t->token()->toString() << std::endl;
          t->token()->printContext();
          return false;
        }
      }else{
        childrenStack.top().push_back(t);
      }
    }

    if (not openTokenStack.empty()) {
      std::cout << "[TokenTree][process][Error] Finished with opened brackets." << std::endl;
      while(not openTokenStack.empty()){
        TokenTree* openMatch = openTokenStack.top();
        openTokenStack.pop();
        openMatch->token()->printContext();
      }
      return false;
    }
    _children = childrenStack.top();
    return true;
  }

  bool processBinaryOperator(Token::Type const type){
    std::stack<std::vector<TokenTree*>> childrenStack;
    std::stack<TokenTree*> sequencerTokenStack;
    childrenStack.push(std::vector<TokenTree*>());
    for (size_t i = 0; i < _children.size(); i++) {
      TokenTree* t = _children[i];
      if (t->token() and t->token()->type()==type) {
        //std::cout << "[TokenTree][process] Sequencer" << std::endl;
        childrenStack.push(std::vector<TokenTree*>());
        sequencerTokenStack.push(t);
      }else{
        //std::cout << "[TokenTree][process] non-Sequencer" << std::endl;
        childrenStack.top().push_back(t);
      }
    }
    if (sequencerTokenStack.empty()) {
      std::cout << "[TokenTree][process][" << Token::typeString(type) << "] NO SEQ" << std::endl;
    }else{
      std::cout << "[TokenTree][process][" << Token::typeString(type) << "] SEQ: " << sequencerTokenStack.size() << std::endl;
      TokenTree* innerNode = new TokenTree(childrenStack.top());
      childrenStack.pop();

      while (not sequencerTokenStack.empty()) {
        TokenTree* sequencer = sequencerTokenStack.top();
        sequencerTokenStack.pop();
        TokenTree* secondNode = new TokenTree(childrenStack.top());
        childrenStack.pop();
        if (sequencerTokenStack.empty()) {
          _children = std::vector<TokenTree*>{
            secondNode,sequencer,innerNode
          };
        }else{
          innerNode = new TokenTree(std::vector<TokenTree*>{
            secondNode,sequencer,innerNode
          });
        }
      }
    }

    return true;
  }

  void print(int const indent=0){
    if (_isTerminal) {
      std::cout << "[TokenTree] " << std::string(indent,' ') << _token->toString() << ": " << _token->typeString() << std::endl;
    } else {
      for(auto const& tree: _children){
        tree->print(indent+4);
      }
    }
  }
  const Token* token(){return _token;};
private:
  bool const _isTerminal;
  Token* const  _token;//NULL if not terminal.
  std::vector<TokenTree*> _children;
};

int main(int argc, char** argv)
{
  std::cout << "[init]" << std::endl;

  for(int i=1;i<argc;i++){
    std::cout << "[args] "<< i << ": " << argv[i] << std::endl;
  }

  if(argc<3){
    std::cout << "[error] need at least 2 args." << std::endl;
    return 1;
  }else{
    std::string inFileName = argv[1];
    std::string outFileName = argv[2];
    std::cout << "[config] read from: " << inFileName << std::endl;
    std::cout << "[config] write to: " << outFileName << std::endl;

    Tokenizer tokenizer(inFileName);
    tokenizer.printLines();
    tokenizer.tokenize();
    tokenizer.printTokens();

    TokenTree* tree = new TokenTree(tokenizer.tokens());
    tree->print();
    if (tree->process()) {
      tree->print();
    }
  }

  std::cout << "[fin]" << std::endl;
  return 0;
}
