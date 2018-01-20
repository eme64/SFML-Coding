#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <list>
#include <SFML/Graphics.hpp>

struct Node;

struct Node
{
  float x; float y; float r; float a;
  sf::Color color;

  Node *parent;

  Node(float xpos, float ypos, float radius, float alpha, sf::Color drawcolor, Node *myparent)
  {
    x = xpos;
    y = ypos;
    color = drawcolor;
    r = radius;
    a = alpha;
    parent = myparent;
  }

  void update()
  {
    if(parent != NULL)
    {
      float pfac = 0.3;
      float cfac = 0.5;

      a = (1.0-pfac)*a + pfac*parent->a;
      x = (1.0-pfac)*x + pfac*parent->x;
      y = (1.0-pfac)*y + pfac*parent->y;


      color.r = (1.0-cfac)*color.r + cfac*parent->color.r;
      color.g = (1.0-cfac)*color.g + cfac*parent->color.g;
      color.b = (1.0-cfac)*color.b + cfac*parent->color.b;

    }else{
      // Independent movement
      float da = (((float) rand() / (RAND_MAX))-0.5)*0.9;
      float speed = 4.0;

      a+=da;
      x+=std::cos(a)*speed;
      y+=std::sin(a)*speed;

      if(x<0){x=0; a=0;}
      if(y<0){y=0; a=M_PI*0.5;}

      if(x>800){x=800; a=M_PI;}
      if(y>600){y=600; a=M_PI*1.5;}

      if(((float) rand() / (RAND_MAX)) < 0.1){
        color.r = 50 + 206*((float) rand() / (RAND_MAX));
        color.g = 50 + 206*((float) rand() / (RAND_MAX));
        color.b = 50 + 206*((float) rand() / (RAND_MAX));
      }
    }
  }

  void draw(sf::RenderWindow &window)
  {
    sf::CircleShape shape(r*2);
    shape.setFillColor(color);
    shape.setPosition(x-r, y-r);
    window.draw(shape, sf::BlendAdd);
  }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "first attempts");
    window.setVerticalSyncEnabled(true);


    std::list<Node*> node_list;

    Node *last_node = NULL;
    for(int i=0; i<1000; i++)
    {
      float xx = 800*((float) rand() / (RAND_MAX));
      float yy = 600*((float) rand() / (RAND_MAX));

      int r = 100 + 156*((float) rand() / (RAND_MAX));
      int g = 100 + 156*((float) rand() / (RAND_MAX));
      int b = 100 + 156*((float) rand() / (RAND_MAX));

      float radius;
      if(last_node !=NULL){
        radius = 2;
        xx = last_node->x;
        yy = last_node->y;
      }else{
        radius = 3;
      }

      Node* n = new Node(xx,yy, radius, 0, sf::Color(r,g,b), last_node);
      node_list.push_back(n);

      if( ((float) rand() / (RAND_MAX)) < 0.05)
      {
        last_node = NULL;
      }else{
        last_node = n;
      }
    }


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
                // we don't process other types of events
                default:
                    break;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }
        /*
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
          pos_x-= 1.0;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
          pos_x+= 1.0;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
          pos_y-= 1.0;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
          pos_y+= 1.0;
        }
        */
        window.clear();

        for (std::list<Node*>::iterator it = node_list.begin(); it != node_list.end(); it++)
        {
            (**it).update();
        }

        for (std::list<Node*>::iterator it = node_list.begin(); it != node_list.end(); it++)
        {
            (**it).draw(window);
        }

        window.display();
    }

    return 0;
}
