#include <iostream>
#include <vector>

class CodeParser{
public:
  CodeParser(std::string fileName){
    std::cout << "[CodeParser] init from file: " << fileName << std::endl;
    // FixMe: Read file here.
  }
private:
  std::vector<std::string> codeLines;
};

int main(int argc, char** argv)
{
  std::cout << "[init]" << std::endl;

  for(int i=1;i<argc;i++){
    std::cout << i << " " << argv[i] << std::endl;
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
  }

  std::cout << "[fin]" << std::endl;
  return 0;
}
