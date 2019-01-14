#include <iostream>
#include <fstream>
#include <vector>
#include <regex>

// left vs right binding binary tokens.

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
      {Token::Type::Sequencer, std::regex("^(;)(.*)")},
    };

    std::vector<std::pair<std::string,Token::Type>> keyWordList {
      {"and", Token::Type::BinOpAdd},
      {"or", Token::Type::BinOpAdd},
      {"def", Token::Type::KeyWord},
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
    for (size_t step = 0; step < 9; step++) {
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
      case 0: return processBrackets();
      case 1: return processBinaryOperator(Token::Type::Sequencer);
      case 2: return processBinaryOperator(Token::Type::Assign);
      case 3: return processBinaryOperator(Token::Type::BinOpOr);
      case 4: return processBinaryOperator(Token::Type::BinOpAnd);
      case 5: return processBinaryOperator(Token::Type::BinOpEqual);
      case 6: return processBinaryOperator(Token::Type::BinOpRelation);
      case 7: return processBinaryOperator(Token::Type::BinOpAdd);
      case 8: return processBinaryOperator(Token::Type::BinOpMult);
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
private:
};



class ASTNone : public AST{
public:
  ASTNone(){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "None."<< std::endl;
  }
private:
};

class ASTSequencer : public AST{
public:
  ASTSequencer(const Token* token):_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "Sequencer: " << _token->toString() << std::endl;
  }
  const Token* token() const {return _token;}
private:
  const Token* _token;
};

class ASTAssign : public AST{
public:
  ASTAssign(const Token* token):_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "Assign: " << _token->toString() << std::endl;
  }
  const Token* token() const {return _token;}
private:
  const Token* _token;
};

class ASTExpression : public AST{
public:
private:
};

class ASTExpressionValue : public ASTExpression{
public:
private:
};

class ASTExpressionValueVariable : public ASTExpressionValue{
public:
  ASTExpressionValueVariable(const Token* token):_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "ExpressionValueVariable: " << _token->toString() << std::endl;
  }
private:
  const Token* _token;
};

class ASTExpressionValueConst : public ASTExpressionValue{
public:
  ASTExpressionValueConst(const Token* token):_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "ExpressionValueConst: " << _token->toString() << std::endl;
  }
private:
  const Token* _token;
};

class ASTExpressionBinOp : public ASTExpression{
public:
  ASTExpressionBinOp(const Token* token):_token(token){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "ExpressionBinOp: " << _token->toString() << std::endl;
  }
private:
  const Token* _token;
};

class ASTExpressionAssign : public ASTExpression{
public:
  ASTExpressionAssign(
    const Token* token,
    const ASTExpression* left,
    const ASTExpression* right
  ):_token(token),_left(left),_right(right){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "ExpressionAssign: " << _token->toString() << std::endl;
    std::cout << "[AST] " << std::string(indent,' ') << "left:" << std::endl;
    _left->print(indent+3);
    std::cout << "[AST] " << std::string(indent,' ') << "right:" << std::endl;
    _right->print(indent+3);
  }
private:
  const Token* _token;
  const ASTExpression* _left;
  const ASTExpression* _right;
};

class ASTExpressionApply : public ASTExpression{
public:
  ASTExpressionApply(
    const ASTExpression* function,
    const ASTExpression* argument
  ):_function(function),_argument(argument){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "ASTExpressionApply: " << std::endl;
    std::cout << "[AST] " << std::string(indent,' ') << "function:" << std::endl;
    _function->print(indent+3);
    std::cout << "[AST] " << std::string(indent,' ') << "argument:" << std::endl;
    _argument->print(indent+3);
  }
private:
  const ASTExpression* _function;
  const ASTExpression* _argument;
};

class ASTExpressionSequence : public ASTExpression{
public:
  ASTExpressionSequence(
    const Token* token,
    const ASTExpression* left,
    const ASTExpression* right
  ):_token(token),_left(left),_right(right){}
  virtual void print(int const indent=0) const {
    std::cout << "[AST] " << std::string(indent,' ') << "ExpressionSequence: " << _token->toString() << std::endl;
    std::cout << "[AST] " << std::string(indent,' ') << "left:" << std::endl;
    _left->print(indent+3);
    std::cout << "[AST] " << std::string(indent,' ') << "right:" << std::endl;
    _right->print(indent+3);
  }
private:
  const Token* _token;
  const ASTExpression* _left;
  const ASTExpression* _right;
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
          case Token::Type::BinOpMult: return new ASTExpressionBinOp(tokenTree->token());
          case Token::Type::BinOpAdd: return new ASTExpressionBinOp(tokenTree->token());
          case Token::Type::Name: return new ASTExpressionValueVariable(tokenTree->token());
          case Token::Type::Integer: return new ASTExpressionValueConst(tokenTree->token());
          case Token::Type::Float: return new ASTExpressionValueConst(tokenTree->token());
          case Token::Type::String: return new ASTExpressionValueConst(tokenTree->token());
          case Token::Type::Sequencer: return new ASTSequencer(tokenTree->token());
          case Token::Type::Assign: return new ASTAssign(tokenTree->token());
          default: {
            std::cout << "[ASTFactory] Error: did not know what to with terminal token: " << std::endl;
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
      if (transformedChildren.size()==3) {
        const AST* first  = transformedChildren[0];
        const AST* second = transformedChildren[1];
        const AST* third  = transformedChildren[2];
        if (const ASTExpressionBinOp* secondBinOp = dynamic_cast<const ASTExpressionBinOp*>(second)) {
          if (const ASTExpression* firstExpression = dynamic_cast<const ASTExpression*>(first)) {
            const ASTExpressionApply* inner = new ASTExpressionApply(secondBinOp,firstExpression);
            if (const ASTExpression* thirdExpression = dynamic_cast<const ASTExpression*>(third)) {
              return new ASTExpressionApply(inner,thirdExpression);
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
        }else if (const ASTAssign* secondAssign = dynamic_cast<const ASTAssign*>(second)) {
          if (const ASTExpressionValueVariable* firstVariable = dynamic_cast<const ASTExpressionValueVariable*>(first)) {
            if (const ASTExpression* thirdExpression = dynamic_cast<const ASTExpression*>(third)) {
              return new ASTExpressionAssign(secondAssign->token(),firstVariable,thirdExpression);
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
        }else if (const ASTSequencer* secondSequencer = dynamic_cast<const ASTSequencer*>(second)) {
          const ASTExpression* firstExpression = dynamic_cast<const ASTExpression*>(first);
          const ASTExpression* thirdExpression = dynamic_cast<const ASTExpression*>(third);
          if (firstExpression and thirdExpression) {
            return new ASTExpressionSequence(secondSequencer->token(),firstExpression,thirdExpression);
          }else if (firstExpression) {
            if(const ASTNone* thirdNone = dynamic_cast<const ASTNone*>(third)){
              return firstExpression;
            }else{
              std::cout << "[ASTFactory] Error: sequencer non-expression (right):" << std::endl;
              secondSequencer->token()->printContext();
              return NULL;
            }
          }else if (thirdExpression) {
            if(const ASTNone* firstNone = dynamic_cast<const ASTNone*>(first)){
              return thirdExpression;
            }else{
              std::cout << "[ASTFactory] Error: sequencer non-expression (left):" << std::endl;
              secondSequencer->token()->printContext();
              return NULL;
            }
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
    tokenizer.printTokens();

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
