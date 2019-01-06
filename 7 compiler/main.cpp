#include <iostream>
#include <fstream>
#include <vector>
#include <regex>

class Token{
public:
  enum Type {
    None,
    Space,NewLine,Comment,Name,
    BracketOpen,BracketClose,
    String,Float,Integer,
    Assign,Minus,Plus,Comma,Dot,
  };

  Token() : _content("generic"), _type(Token::Type::None) {}
  Token(std::string content, Token::Type type)
  : _content(content), _type(type) {}

  std::string toString() const {
    return _content;
  }
  Token::Type type() const {return _type;}
private:
  std::string _content;
  Type _type;
};

class Tree{
public:
  virtual void print(int const indent=0) const{
    std::cout << "[Tree] " << std::string(indent, ' ') << "Tree" << std::endl;
  }
private:
};

class TreeStatement : Tree{
public:
  virtual void print(int const indent=0) const{
    std::cout << "[Tree] " << std::string(indent, ' ') << "TreeStatement" << std::endl;
  }
private:
};

class TreeStatementSequence : TreeStatement{
public:
  virtual void print(int const indent=0) const {
    std::cout << "[Tree] " << std::string(indent, ' ') << "TreeStatementSequence" << std::endl;
    for (auto s : _sequence) {
      s->print(indent+2);
    }
  }
private:
  std::vector<TreeStatement*> _sequence;
};

class TreeFactory{
public:
  static Tree* makeSequence(
    std::vector<Token> const &tokens,
    int const from,
    int const to
  ){
    int start = from;
    TreeStatementSequence* tree = new TreeStatementSequence();
    /*
    while(start<to){
      if(to-start<2){
        delete tree;
        return NULL;
      }

      switch (tokens[start].type()) {
        case Token::Type::Name:{
          switch (tokens[from+1].type()) {
            case Token::Type::BracketOpen:{
              const int bracketEnd = findMatchingBracket(tokens,1,to);
              if(bracketEnd==to){
                delete tree;
                return NULL;
              }

              children.push_back(Tree(tokens,2,bracketEnd));
              break;
            }
            case Token::Type::Assign:{
              type=Type::Assign;
              break;
            }
          }
          break;
        }
        default:{
          delete tree;
          return NULL;
          break;
        }


    }
    */
    return tree;
  }

  int findMatchingBracket(
    std::vector<Token> const &tokens,
    int const from,
    int const to
  )
  {
    int counter = 0;
    for(int i = from; i < to; ++i) {
      switch (tokens[i].type()) {
        case Token::Type::BracketOpen:
          counter++;
          break;
        case Token::Type::BracketClose:
          if (counter==1) {
            return i;
          }
          counter--;
          break;
        default:
          break;
      }
    }
    return to;//failed.
  }
private:
};

class CodeParser{
public:
  CodeParser(std::string fileName){
    std::cout << "[CodeParser] init from file: " << fileName << std::endl;
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
    tokens.resize(0);
    tokenErrors.resize(0);
    int i=0;
    for(auto const& line: codeLines) {
      std::pair<std::vector<Token>,std::string> res = tokenize(line);
      std::vector<Token> tVec = res.first;
      std::string text = res.second;
      if (text!="") {
        std::string error = std::to_string(i)+":"+text;
        tokenErrors.push_back(error);
        std::cout << "[tokenize] [Error] " << error << std::endl;
      }
      tokens.reserve(tokens.size() + tVec.size());
      tokens.insert(tokens.end(),tVec.begin(),tVec.end());
      tokens.push_back(Token("",Token::Type::NewLine));
      i++;
    }
  }

  std::pair<std::vector<Token>,std::string> tokenize(std::string const line){
    std::string text = line;

    std::vector<std::pair<Token::Type,std::regex>> reList {
      {Token::Type::Space, std::regex("^(\\s+)(.*)")},
      {Token::Type::Comment, std::regex("^(#)(.*)")},
      {Token::Type::Name, std::regex("^([a-zA-Z][\\w]*)(.*)")},
      {Token::Type::BracketOpen, std::regex("^(\\()(.*)")},
      {Token::Type::BracketClose, std::regex("^(\\))(.*)")},
      {Token::Type::String, std::regex("^\"(.*)\"(.*)")},
      {Token::Type::Float, std::regex("^(\\d+\\.\\d*)(.*)")},
      {Token::Type::Integer, std::regex("^(\\d+)(.*)")},
      {Token::Type::Assign, std::regex("^(=)(.*)")},
      {Token::Type::Minus, std::regex("^(-)(.*)")},
      {Token::Type::Plus, std::regex("^(\\+)(.*)")},
      {Token::Type::Comma, std::regex("^(,)(.*)")},
      {Token::Type::Dot, std::regex("^(\\.)(.*)")},
    };

    std::vector<Token> tVec;
    std::smatch m;

    bool someMatch;
    do {
      someMatch = false;

      for(auto const& re: reList){
        if (std::regex_search(text,m,re.second)){
          someMatch = true;
          std::string t = m[1];
          text = m[2];
          if(re.first==Token::Type::Space){break;}
          if(re.first==Token::Type::Comment){return std::make_pair(tVec,"");}
          tVec.push_back(Token(t,re.first));
          break;// restart for
        }
      }
    } while(someMatch);

    return std::make_pair(tVec,text);
  }

  void printTokens() const {
    int i=0;
    for(auto const& token: tokens) {
      std::cout << "[printTokens] "<< i << ": " << token.toString() << std::endl;
      i++;
    }
  }

  void makeTree(){
    tree = TreeFactory::makeSequence(tokens,0,tokens.size());
  }

  void printTree(){
    if(tree==NULL){
      std::cout << "[printTree] no tree" << std::endl;
    }else{
      std::cout << "[printTree]" << std::endl;
      tree->print();
    }
  }
private:
  std::vector<std::string> codeLines;
  std::vector<Token> tokens;
  std::vector<std::string> tokenErrors;
  Tree* tree = NULL;
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

    CodeParser parser(inFileName);
    parser.printLines();
    parser.tokenize();
    parser.printTokens();
    parser.makeTree();
    parser.printTree();
  }

  std::cout << "[fin]" << std::endl;
  return 0;
}
