#include <iostream>
#include <fstream>
#include <vector>
#include <regex>

// fix bracket use.

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
    Assign,Comma,Dot,Colon,DoubleColon,
    BinOpMult,BinOpAdd,BinOpRelation,BinOpEqual,
    BinOpAnd,BinOpOr,
    KeyWord,
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
      case Type::BinOpMult: return "BinOpMult";
      case Type::BinOpAdd: return "BinOpAdd";
      case Type::BinOpRelation: return "BinOpRelation";
      case Type::BinOpEqual: return "BinOpEqual";
      case Type::BinOpAnd: return "BinOpAnd";
      case Type::BinOpOr: return "BinOpOr";
      case Type::Comma: return "Comma";
      case Type::Colon: return "Colon";
      case Type::DoubleColon: return "DoubleColon";
      case Type::Dot: return "Dot";
      case Type::KeyWord: return "KeyWord";
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
        std::string error = std::to_string(i)+": "+text;
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
      {Token::Type::Name, std::regex("^([a-zA-Z_][\\w]*(?:::[a-zA-Z_][\\w]*)*)(.*)")},
      {Token::Type::BracketOpen, std::regex("^(\\(|\\[|\\{)(.*)")},
      {Token::Type::BracketClose, std::regex("^(\\)|\\]|\\})(.*)")},
      {Token::Type::String, std::regex("^\"(.*)\"(.*)")},
      {Token::Type::Float, std::regex("^(\\d+\\.\\d*)(.*)")},
      {Token::Type::Integer, std::regex("^(\\d+)(.*)")},
      {Token::Type::BinOpEqual, std::regex("^(==|!=)(.*)")},
      {Token::Type::Assign, std::regex("^(=)(.*)")},
      {Token::Type::BinOpMult, std::regex("^(\\*|/)(.*)")},
      {Token::Type::BinOpAdd, std::regex("^(-|\\+)(.*)")},
      {Token::Type::BinOpRelation, std::regex("^(<=|>=)(.*)")},
      {Token::Type::BinOpRelation, std::regex("^(<|>)(.*)")},
      {Token::Type::BinOpAnd, std::regex("^(&&)(.*)")},
      {Token::Type::BinOpOr, std::regex("^(\\|\\|)(.*)")},
      {Token::Type::Comma, std::regex("^(,)(.*)")},
      {Token::Type::Dot, std::regex("^(\\.)(.*)")},
      {Token::Type::DoubleColon, std::regex("^(::)(.*)")},
      {Token::Type::Colon, std::regex("^(:)(.*)")},
      {Token::Type::Sequencer, std::regex("^(;)(.*)")},
    };

    std::vector<std::pair<std::string,Token::Type>> keyWordList {
      {"and", Token::Type::BinOpAdd},
      {"or", Token::Type::BinOpAdd},
      {"def", Token::Type::KeyWord},
      {"if", Token::Type::KeyWord},
      {"elif", Token::Type::KeyWord},
      {"else", Token::Type::KeyWord},
      {"local", Token::Type::KeyWord},
      {"global", Token::Type::KeyWord},
      {"const", Token::Type::KeyWord},
      {"function", Token::Type::KeyWord},
      {"class", Token::Type::KeyWord},
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
          if(re.first==Token::Type::Name){
            bool keyWordFound=false;
            for(auto const& kw: keyWordList){
              if (kw.first==t) {
                tVec.push_back(new Token(t,kw.second,_file,line,lineNum,parsePos));
                keyWordFound = true;
                break;// inner for
              }
            }
            if (keyWordFound) {break;}// restart for
          }
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

  bool process() {
    for (size_t step = 0; step < 11; step++) {
      bool res = processStep(step);
      if (not res) {return false;}
    }
    return true;
  }

  bool processStep(int const step) {
    if (_children.size()==0) {return true;}
    for (TokenTree* const t:_children) {
      bool res = t->processStep(step);
      if (not res) {return false;}
    }
    return processStepExec(step);
  }

  bool processStepExec(int const step){
    switch (step) {
      case 0:  return processBrackets();
      case 1:  return processBinaryOperator(Token::Type::Sequencer,false);
      case 2:  return processBinaryOperator(Token::Type::Assign,false);
      case 3:  return processBinaryOperator(Token::Type::BinOpOr,true);
      case 4:  return processBinaryOperator(Token::Type::BinOpAnd,true);
      case 5:  return processBinaryOperator(Token::Type::BinOpEqual,true);
      case 6:  return processBinaryOperator(Token::Type::BinOpRelation,true);
      case 7:  return processBinaryOperator(Token::Type::BinOpAdd,true);
      case 8:  return processBinaryOperator(Token::Type::BinOpMult,true);
      case 9:  return processBinaryOperator(Token::Type::Colon,true);
      case 10: return processBinaryOperator(Token::Type::DoubleColon,true);
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

  bool processBinaryOperator(Token::Type const type, bool const leftToRight){
    bool hasType = false;
    for (size_t i = 0; i < _children.size(); i++) {
      TokenTree* t = _children[i];
      if (t->token() and t->token()->type()==type) {
        hasType = true;
        break;
      }
    }
    if(not hasType){return true;}


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
      //std::cout << "[TokenTree][process][" << Token::typeString(type) << "] NO SEQ" << std::endl;
    }else{
      //std::cout << "[TokenTree][process][" << Token::typeString(type) << "] SEQ: " << sequencerTokenStack.size() << std::endl;
      if (leftToRight) {// reverse both stacks.
        std::stack<std::vector<TokenTree*>> tmp_childrenStack;
        std::stack<TokenTree*> tmp_sequencerTokenStack;
        while (not sequencerTokenStack.empty()) {
          tmp_sequencerTokenStack.push(sequencerTokenStack.top());
          sequencerTokenStack.pop();
        }
        while (not childrenStack.empty()) {
          tmp_childrenStack.push(childrenStack.top());
          childrenStack.pop();
        }
        childrenStack.swap(tmp_childrenStack);
        sequencerTokenStack.swap(tmp_sequencerTokenStack);
      }

      TokenTree* innerNode;
      if (childrenStack.top().size()==1) {
        innerNode = childrenStack.top()[0];
      }else{
        innerNode = new TokenTree(childrenStack.top());
      }
      childrenStack.pop();

      while (not sequencerTokenStack.empty()) {
        TokenTree* sequencer = sequencerTokenStack.top();
        sequencerTokenStack.pop();
        TokenTree* secondNode;
        if (childrenStack.top().size()==1) {
          secondNode = childrenStack.top()[0];
        }else{
          secondNode = new TokenTree(childrenStack.top());
        }
        childrenStack.pop();
        if (sequencerTokenStack.empty()) {
          if (leftToRight) {
            _children = std::vector<TokenTree*>{innerNode,sequencer,secondNode};
          } else {
            _children = std::vector<TokenTree*>{secondNode,sequencer,innerNode};
          }
        }else{
          if (leftToRight) {
            innerNode = new TokenTree(std::vector<TokenTree*>{innerNode,sequencer,secondNode});
          } else {
            innerNode = new TokenTree(std::vector<TokenTree*>{secondNode,sequencer,innerNode});
          }
        }
      }
    }

    return true;
  }

  void print(int const indent=0){
    if (indent==0) {
      std::cout << "[TokenTree] ------- print:" << std::endl;
    }
    if (_isTerminal) {
      std::cout << "[TokenTree] " << std::string(indent,' ') << _token->toString() << ": " << _token->typeString() << std::endl;
    } else {
      std::cout << "[TokenTree] " << std::string(indent,' ') << "Node: " << _children.size() << std::endl;
      for(auto const& tree: _children){
        tree->print(indent+4);
      }
    }
  }
  const Token* token() const {return _token;};
  const bool isTerminal() const {return _isTerminal;};
  const std::vector<TokenTree*>& children() const {return _children;}
private:
  bool const _isTerminal;
  Token* const  _token;//NULL if not terminal.
  std::vector<TokenTree*> _children;
};

class AST{
public:
  virtual void print(int const indent=0) const {
    std::cout << "[AST] ------- print:" << std::endl;
  }
  virtual bool writeable() const {return false;}
  virtual bool evaluable() const {return false;}
private:
};

class ASTNone : public AST{
public:
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "None."<< std::endl;
  }
private:
};

class ASTSequencerSymbol : public AST{
public:
  ASTSequencerSymbol(const Token* token)
  :_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "Sequencer Symbol: " << _token->toString() << std::endl;
  }
  const Token* token() const {return _token;}
private:
  const Token* _token;
};

class ASTBracketOpenSymbol : public AST{
public:
  ASTBracketOpenSymbol(const Token* token)
  :_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "BracketOpen Symbol: " << _token->toString() << std::endl;
  }
  const Token* token() const {return _token;}
private:
  const Token* _token;
};

class ASTBracketCloseSymbol : public AST{
public:
  ASTBracketCloseSymbol(const Token* token)
  :_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "BracketClose Symbol: " << _token->toString() << std::endl;
  }
  const Token* token() const {return _token;}
private:
  const Token* _token;
};

class ASTAssignSymbol : public AST{
public:
  ASTAssignSymbol(const Token* token)
  :_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "Assign Symbol: " << _token->toString() << std::endl;
  }
  const Token* token() const {return _token;}
private:
  const Token* _token;
};

class ASTColonSymbol : public AST{
public:
  ASTColonSymbol(const Token* token)
  :_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "Colon Symbol: " << _token->toString() << std::endl;
  }
  const Token* token() const {return _token;}
private:
  const Token* _token;
};

class ASTKeyWord : public AST{
public:
  ASTKeyWord(const Token* token)
  :_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "KeyWord: " << _token->toString() << std::endl;
  }
  Token const* token() const {return _token;}
private:
  const Token* _token;
};

class ASTName : public AST{
public:
  ASTName(const Token* token)
  :_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "Name: " << _token->toString() << std::endl;
  }
  virtual bool writeable() const {return true;}
  virtual bool evaluable() const {return true;}
private:
  const Token* _token;
};

class ASTVariableDefinition : public AST{
public:
  ASTVariableDefinition(const bool global,const bool constant, const ASTName* name)
  :_global(global),_constant(constant),_name(name),_type(NULL){}
  ASTVariableDefinition(const ASTVariableDefinition* vd)
  :_global(vd->global()),_constant(vd->constant()),_name(vd->name()),_type(vd->type()){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "VariableDefinition: " << std::endl;
    std::cout << "[AST] " << std::string(indent,' ') << "global: " << _global << std::endl;
    std::cout << "[AST] " << std::string(indent,' ') << "constant: " << _constant << std::endl;
    _name->print(indent+3);
    if (_type) {
      std::cout << "[AST] " << std::string(indent,' ') << "Type: " << std::endl;
      _type->print(indent+3);
    }else{
      std::cout << "[AST] " << std::string(indent,' ') << "Type: No type" << std::endl;
    }
  }
  virtual bool writeable() const {return true;}
  virtual bool evaluable() const {return false;}
  ASTName const* type() const {return _type;}
  bool typeIs(const ASTName* type){
    if (_type) {
      return false;
    }else{
      _type = type;
      return true;
    }
  }
  bool global() const {return _global;}
  bool constant() const {return _constant;}
  const ASTName* name() const {return _name;}
private:
  const bool _global;
  const bool _constant;
  const ASTName* _name;
  ASTName const* _type;
};

class ASTConst : public AST{
public:
  ASTConst(const Token* token)
  :_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "Const: " << _token->toString() << std::endl;
  }
  virtual bool evaluable() const {return true;}
private:
  const Token* _token;
};

class ASTBinOp : public AST{
public:
  ASTBinOp(const Token* token)
  :_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "BinOp: " << _token->toString() << std::endl;
  }
private:
  const Token* _token;
};

class ASTAssign : public AST{
public:
  ASTAssign(
    const Token* token,
    const AST* left,
    const AST* right
  )
  :_token(token),_left(left),_right(right){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "Assign: " << _token->toString() << std::endl;
    std::cout << "[AST] " << std::string(indent,' ') << "left:" << std::endl;
    _left->print(indent+3);
    std::cout << "[AST] " << std::string(indent,' ') << "right:" << std::endl;
    _right->print(indent+3);
  }
  virtual bool evaluable() const {return true;}
private:
  const Token* _token;
  const AST* _left;
  const AST* _right;
};

class ASTApply : public AST{
public:
  ASTApply(
    const AST* function,
    const AST* argument
  )
  :_function(function),_argument(argument){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "ASTApply: " << std::endl;
    std::cout << "[AST] " << std::string(indent,' ') << "function:" << std::endl;
    _function->print(indent+3);
    std::cout << "[AST] " << std::string(indent,' ') << "argument:" << std::endl;
    _argument->print(indent+3);
  }
  virtual bool evaluable() const {return true;}
private:
  const AST* _function;
  const AST* _argument;
};

class ASTIf : public AST{
public:
  ASTIf(
    const AST* condition,
    const AST* ifTrue,
    const AST* ifFalse
  )
  :_condition(condition),_ifTrue(ifTrue),_ifFalse(ifFalse){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "ExpressionIf " << std::endl;
    std::cout << "[AST] " << std::string(indent,' ') << "condition:" << std::endl;
    _condition->print(indent+3);
    std::cout << "[AST] " << std::string(indent,' ') << "ifTrue:" << std::endl;
    _ifTrue->print(indent+3);
    std::cout << "[AST] " << std::string(indent,' ') << "ifFalse:" << std::endl;
    _ifFalse->print(indent+3);
  }
  virtual bool evaluable() const {return true;}
private:
  const AST* _condition;
  const AST* _ifTrue;
  const AST* _ifFalse;
};

class ASTSequence : public AST{
public:
  ASTSequence(
    const Token* token,
    const AST* left,
    const AST* right
  )
  :_token(token),_left(left),_right(right){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "Sequence: " << _token->toString() << std::endl;
    std::cout << "[AST] " << std::string(indent,' ') << "left:" << std::endl;
    _left->print(indent+3);
    std::cout << "[AST] " << std::string(indent,' ') << "right:" << std::endl;
    _right->print(indent+3);
  }
  virtual bool evaluable() const {return true;}
private:
  const Token* _token;
  const AST* _left;
  const AST* _right;
};

class ASTFactory{
public:
  static const AST* map(TokenTree* const tokenTree){
    if (tokenTree->isTerminal()) {
      if (not tokenTree->token()) {
        std::cout << "[ASTFactory] Error: terminal TokenTree has no token." << std::endl;
        return NULL;
      } else {
        switch (tokenTree->token()->type()) {
          case Token::Type::BinOpMult: return new ASTBinOp(tokenTree->token());
          case Token::Type::BinOpAdd: return new ASTBinOp(tokenTree->token());
          case Token::Type::BinOpEqual: return new ASTBinOp(tokenTree->token());
          case Token::Type::BinOpRelation: return new ASTBinOp(tokenTree->token());
          case Token::Type::BinOpAnd: return new ASTBinOp(tokenTree->token());
          case Token::Type::BinOpOr: return new ASTBinOp(tokenTree->token());
          case Token::Type::Name: return new ASTName(tokenTree->token());
          case Token::Type::Integer: return new ASTConst(tokenTree->token());
          case Token::Type::Float: return new ASTConst(tokenTree->token());
          case Token::Type::String: return new ASTConst(tokenTree->token());
          case Token::Type::Sequencer: return new ASTSequencerSymbol(tokenTree->token());
          case Token::Type::Assign: return new ASTAssignSymbol(tokenTree->token());
          case Token::Type::BracketOpen: return new ASTBracketOpenSymbol(tokenTree->token());
          case Token::Type::BracketClose: return new ASTBracketCloseSymbol(tokenTree->token());
          case Token::Type::KeyWord: return new ASTKeyWord(tokenTree->token());
          case Token::Type::Colon: return new ASTColonSymbol(tokenTree->token());
          default: {
            std::cout << "[ASTFactory] Error: did not know what to do with terminal token: " << std::endl;
            tokenTree->token()->printContext();
          }
        }
      }
    }else{
      // --------- transform
      const std::vector<TokenTree*> &children = tokenTree->children();
      std::vector<const AST*> transformedChildren;
      transformedChildren.reserve(children.size());
      for (size_t i = 0; i < children.size(); i++) {
        const AST* ast = ASTFactory::map(children[i]);
        if (not ast) {return NULL;}
        transformedChildren.push_back(ast);
      }
      // --------- find match
      if (transformedChildren.size()==0) {
        return new ASTNone();
      }
      const AST* first  = transformedChildren[0];
      if (transformedChildren.size()==3) {
        const AST* second = transformedChildren[1];
        const AST* third  = transformedChildren[2];
        if (const ASTBinOp* secondBinOp = dynamic_cast<const ASTBinOp*>(second)) {
          if (first->evaluable()) {
            const ASTApply* inner = new ASTApply(secondBinOp,first);
            if (third->evaluable()) {
              return new ASTApply(inner,third);
            }else{
              std::cout << "[ASTFactory] Error: could not apply BinOp (right):" << std::endl;
              std::cout << "[ASTFactory] BinOp:" << std::endl;
              first->print();
              std::cout << "[ASTFactory] right:" << std::endl;
              third->print();
              return NULL;
            }
          }else{
            std::cout << "[ASTFactory] Error: could not apply BinOp (left):" << std::endl;
            std::cout << "[ASTFactory] BinOp:" << std::endl;
            secondBinOp->print();
            std::cout << "[ASTFactory] left:" << std::endl;
            first->print();
            std::cout << "[ASTFactory] right:" << std::endl;
            third->print();
            return NULL;
          }
        }else if (const ASTAssignSymbol* secondAssign = dynamic_cast<const ASTAssignSymbol*>(second)) {
          if (first->writeable()) {
            if (third->evaluable()) {
              return new ASTAssign(secondAssign->token(),first,third);
            }else{
              std::cout << "[ASTFactory] Error: could not assign variable to non-expression (right):" << std::endl;
              secondAssign->token()->printContext();
              return NULL;
            }
          }else{
            std::cout << "[ASTFactory] Error: could not assign to non-variable (left):" << std::endl;
            secondAssign->token()->printContext();
            return NULL;
          }
        }else if (const ASTSequencerSymbol* secondSequencer = dynamic_cast<const ASTSequencerSymbol*>(second)) {
          if (first->evaluable() and third->evaluable()) {
            return new ASTSequence(secondSequencer->token(),first,third);
          }else if (first->evaluable()) {
            if(const ASTNone* thirdNone = dynamic_cast<const ASTNone*>(third)){
              return first;
            }else{
              std::cout << "[ASTFactory] Error: sequencer non-expression (right):" << std::endl;
              secondSequencer->token()->printContext();
              return NULL;
            }
          }else if (third->evaluable()) {
            if(const ASTNone* firstNone = dynamic_cast<const ASTNone*>(first)){
              return third;
            }else{
              std::cout << "[ASTFactory] Error: sequencer non-expression (left):" << std::endl;
              secondSequencer->token()->printContext();
              return NULL;
            }
          }
        }else if (const ASTBracketOpenSymbol* firstBracket = dynamic_cast<const ASTBracketOpenSymbol*>(first)) {
          if (const ASTBracketCloseSymbol* thirdBracket = dynamic_cast<const ASTBracketCloseSymbol*>(third)) {
              return second;
          }
        }else if (const ASTColonSymbol* secondColon = dynamic_cast<const ASTColonSymbol*>(second)) {

          if (const ASTName* thirdName = dynamic_cast<const ASTName*>(third)) {
            if (const ASTName* firstName = dynamic_cast<const ASTName*>(first)) {
              bool constant = false;
              bool global = false;
              ASTVariableDefinition* vd = new ASTVariableDefinition(global,constant,firstName);
              vd->typeIs(thirdName);
              return vd;
            }else if (const ASTVariableDefinition* firstVarDef = dynamic_cast<const ASTVariableDefinition*>(first)) {
              ASTVariableDefinition* vd = new ASTVariableDefinition(firstVarDef);
              if (vd->typeIs(thirdName)) {
                return vd;
              }else{
                std::cout << "[ASTFactory] Error: superfluous type assignment:" << std::endl;
                secondColon->token()->printContext();
                return NULL;
              }
            }else{
              std::cout << "[ASTFactory] Error: bad item before colon:" << std::endl;
              secondColon->token()->printContext();
              return NULL;
            }
          }else{
            std::cout << "[ASTFactory] Error: bad item after colon (need type name):" << std::endl;
            secondColon->token()->printContext();
            return NULL;
          }
        }
      }
      if (const ASTKeyWord* firstKeyWord = dynamic_cast<const ASTKeyWord*>(first)) {
        std::string const firstString(firstKeyWord->token()->toString());
        if (firstString == "if") {
          std::vector<const AST*> conditions;
          std::vector<const AST*> ifTrue;
          AST const* ifFalse = NULL;
          int i = 1;
          bool expectCondition = true;//true: cond, ifTrue. false: ifFalse, end
          Token const* lastKeyWord = firstKeyWord->token();
          while (true) {
            if (expectCondition) {
              std::cout << "[ASTFactory] go: if/elif " << i << std::endl;
              if (transformedChildren.size()<i+2) {
                std::cout << "[ASTFactory] Error: if/elif: require condition and expression:" << std::endl;
                lastKeyWord->printContext();
                return NULL;
              }
              const AST* cond = transformedChildren[i];
              if (not cond->evaluable()) {
                std::cout << "[ASTFactory] Error: if/elif: condition not evaluable:" << std::endl;
                lastKeyWord->printContext();
                return NULL;
              }
              const AST* ifT = transformedChildren[i+1];
              if (not ifT->evaluable()) {
                std::cout << "[ASTFactory] Error: if/elif: if true statement not evaluable:" << std::endl;
                lastKeyWord->printContext();
                return NULL;
              }
              // success:
              conditions.push_back(cond);
              ifTrue.push_back(ifT);
              if (transformedChildren.size()==i+2) {
                // keef ifFalse=NULL
                break;
              }else{
                bool elifElseSuccess=false;
                if(const ASTKeyWord* nextKeyWord = dynamic_cast<const ASTKeyWord*>(transformedChildren[i+2])){
                  std::string nextString = nextKeyWord->token()->toString();
                  if (nextString=="elif") {
                    elifElseSuccess = true;
                    lastKeyWord = nextKeyWord->token();
                  }else if (nextString=="else") {
                    elifElseSuccess = true;
                    expectCondition = false;
                    lastKeyWord = nextKeyWord->token();
                  }
                  i+=3;
                }
                if (not elifElseSuccess) {
                  std::cout << "[ASTFactory] Error: if/elif: failed to find next elif/else/end:" << std::endl;
                  lastKeyWord->printContext();
                  return NULL;
                }
              }
            }else{// expect only one ifFalse expression:
              std::cout << "[ASTFactory] go: else " << i << std::endl;
              if (transformedChildren.size()<i+1) {
                std::cout << "[ASTFactory] Error: else: expression:" << std::endl;
                lastKeyWord->printContext();
                return NULL;
              }
              if (transformedChildren.size()>i+1) {
                std::cout << "[ASTFactory] Error: else: only require one expression after else:" << std::endl;
                lastKeyWord->printContext();
                return NULL;
              }
              const AST* ifF = transformedChildren[i];
              if (not ifF->evaluable()) {
                std::cout << "[ASTFactory] Error: else: else statement not evaluable:" << std::endl;
                lastKeyWord->printContext();
                return NULL;
              }
              // success:
              ifFalse = ifF;
              break;
            }
          }
          std::cout << "[ASTFactory] success: if/elif " << std::endl;
          AST const* insideNode = ifFalse;
          if (ifFalse==NULL) {
            insideNode = new ASTNone();
          }
          for (int i = conditions.size()-1; i >= 0; i--) {
            std::cout << "[ASTFactory] success: if/elif " << i << std::endl;
            insideNode = new ASTIf(conditions[i], ifTrue[i], insideNode);
          }
          return insideNode;
        }else if (firstString == "global" or firstString == "const") {
          int i = 0;
          std::vector<std::string> variableQualifiers;
          ASTKeyWord const* nextKeyWord = NULL;
          while (i<transformedChildren.size() and (nextKeyWord = dynamic_cast<const ASTKeyWord*>(transformedChildren[i]))) {
            std::string nextString= nextKeyWord->token()->toString();
            if (nextString == "global" or nextString == "const") {
              variableQualifiers.push_back(nextString);
              i++;
            }else{
              std::cout << "[ASTFactory] Error: bad keyword after variable definition qualifiers:" << std::endl;
              nextKeyWord->token()->printContext();
              return NULL;
            }
          }
          if (not i==transformedChildren.size()-1) {
            std::cout << "[ASTFactory] Error: incorrect variable definition (qualifiers):" << std::endl;
            firstKeyWord->token()->printContext();
            return NULL;
          }
          if (const ASTName* lastName = dynamic_cast<const ASTName*>(transformedChildren[i])) {
            bool constant = false;
            bool global = false;
            for (auto &qualifier : variableQualifiers) {
              if (qualifier=="global") {
                global = true;
              }else if (qualifier=="const") {
                constant=true;
              }
            }
            return new ASTVariableDefinition(global,constant,lastName);
          }else{
            std::cout << "[ASTFactory] Error: incorrect variable definition (name):" << std::endl;
            firstKeyWord->token()->printContext();
            return NULL;
          }
        }
      }

      // --------- no match
      std::cout << "[ASTFactory] Error: Could not build parent node from:" << std::endl;
      for (size_t i = 0; i < children.size(); i++) {
        std::cout << "[ASTFactory] child " << i << std::endl;
        if (transformedChildren[i]) {
          transformedChildren[i]->print();
        }else{
          std::cout << "[ASTFactory] NULL" << std::endl;
        }
      }
      return NULL;
    }
  }
private:
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
    //tokenizer.printTokens();

    TokenTree* tree = new TokenTree(tokenizer.tokens());
    tree->print();
    if (tree->process()) {
      tree->print();
      const AST* ast = ASTFactory::map(tree);
      if (ast) {
        ast->print();
      }
    }
  }

  std::cout << "[fin]" << std::endl;
  return 0;
}
