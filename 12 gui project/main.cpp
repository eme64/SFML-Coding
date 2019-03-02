#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <SFML/Graphics.hpp>
// #include <SFML/Audio.hpp>
// #include <list>
// #include <queue>
// #include <algorithm>
// #include <thread>
// #include <valarray>
// #include <complex>
// #include <mutex>
// #include <forward_list>
// #include <functional>

#include "ep_gui.hpp"


//-------------------------------------------------------- Main
int main()
{
  // ------------------------------------ WINDOW
  EP::GUI::MasterWindow* masterWindow = new EP::GUI::MasterWindow(1000,600,"window title",false);
  {
    EP::GUI::Window* window1 = (new EP::GUI::Window("window1",masterWindow->area(),200,100,200,300,"MyWindow"));
    EP::GUI::Window* window2 = (new EP::GUI::Window("window2",masterWindow->area(),250,150,300,200,"MyWindow"));
  }
  while (masterWindow->isAlive()) {
    masterWindow->update();
    masterWindow->draw();
  }
}
