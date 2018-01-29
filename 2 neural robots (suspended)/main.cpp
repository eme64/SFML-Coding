#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <list>
#include <SFML/Graphics.hpp>

struct Robot;
struct Node;
struct Connector;

struct NNetwork;
struct NLayer;

struct NLayer
{
  // just a linear function plus a nonlinear from m -> n dimensions
  size_t dim_in;
  size_t dim_out;

  std::vector<float> matrix;

  std::vector<float> result;

  NLayer(size_t d_in, size_t d_out)
  {
    dim_in = d_in;
    dim_out = d_out;

    matrix = std::vector<float>((dim_in + 1) * dim_out); // +1 for constant/offset

    for(size_t o=0; o<dim_out; o++){
      for(size_t i = 0; i<dim_in; i++){
        matrix[o*(dim_in+1) + i] = ((float) rand() / (RAND_MAX))*2.0 - 1.0;
      }
    }

    result = std::vector<float>(dim_out);
  }

  void compute(std::vector<float> &input)
  {
    for(size_t o=0; o<dim_out; o++){
      result[o] = 0;

      for(size_t i = 0; i<dim_in; i++){
        result[o] += matrix[o*(dim_in+1) + i] * input[i];
      }
      result[o] += matrix[o*(dim_in+1) + dim_in]; // offset

      // nonlinearity:
      result[o] = 1.0 / (1.0  +  std::exp(-result[o]));
    }
  }
};

struct NNetwork
{
  size_t num_layers;
  std::vector<NLayer*> layers;

  std::vector<float> input;

  size_t output_size;

  NNetwork(size_t in_size)
  {
    output_size = in_size;
    input = std::vector<float>(in_size);

    num_layers = 0;
    layers = std::vector<NLayer*>(0);
  }

  void add_layer(size_t out_size)
  {
    NLayer* l = new NLayer(output_size, out_size);
    output_size = out_size;

    num_layers++;
    layers.push_back(l);
  }

  void compute(std::vector<float> &my_input, std::vector<float> &result)
  {
    input = my_input;
    result = input;
    for(size_t i = 0; i<num_layers; i++){
      NLayer* l = layers[i];
      l->compute(result);
      result = l->result;
    }
  }

  void draw(sf::RenderWindow &window, sf::Font &font, int x, int y)
  {
    // draw input
    for(int i=0; i<input.size(); i++){
      sf::RectangleShape shape(sf::Vector2f(50, 20));
      shape.setFillColor(sf::Color(50,50,255 * input[i]));
      shape.setPosition(x + i*52, y);
      window.draw(shape, sf::BlendAdd);

      sf::Text text;
      text.setFont(font);
      text.setString(std::to_string(input[i]));
      text.setCharacterSize(10);
      text.setColor(sf::Color(255,255,255));
      text.setPosition(x + i*52 + 1, y + 1);
      //text.setStyle(sf::Text::Bold | sf::Text::Underlined);
      window.draw(text, sf::BlendAdd);
    }

    // draw layers

  }
};

struct Node
{
  float x; float y; float r;
  float vx; float vy;
  float mass;

  sf::Color color;

  float friction;

  Node(float xpos, float ypos, float radius, sf::Color drawcolor)
  {
    x = xpos;
    y = ypos;
    vx = 0;
    vy = 0;
    color = drawcolor;
    r = radius;

    mass = 1.0;

    friction = 0.95;
  }

  void update()
  {
    //float da = (((float) rand() / (RAND_MAX))-0.5)*0.9;

    x+=vx;
    y+=vy;

    vx*=friction;
    vy*=friction;

    if(x<0){x=0; vx = std::abs(vx);}
    if(y<0){y=0; vy = std::abs(vy);}

    if(x>800){x=800; vx = -std::abs(vx);}
    if(y>600){y=600; vy = -std::abs(vy);}

    /*
    if(((float) rand() / (RAND_MAX)) < 0.1){
      color.r = 50 + 206*((float) rand() / (RAND_MAX));
      color.g = 50 + 206*((float) rand() / (RAND_MAX));
      color.b = 50 + 206*((float) rand() / (RAND_MAX));
    }
    */
  }

  void draw(sf::RenderWindow &window)
  {
    sf::CircleShape shape(r);
    shape.setFillColor(color);
    shape.setPosition(x-r, y-r);
    window.draw(shape, sf::BlendAdd);
  }
};

struct Connector
{
  Node*node_1;
  Node*node_2;

  float strength;
  float length;
  float ref_length;

  Connector(Node*n1, Node*n2)
  {
    node_1 = n1;
    node_2 = n2;

    strength = 0.1;
    length = std::sqrt((n1->x  -  n2->x)*(n1->x  -  n2->x)   +  (n1->y  -  n2->y)*(n1->y  -  n2->y));
    ref_length = length;
  }

  void update()
  {
    float dx = node_1->x  -  node_2->x;
    float dy = node_1->y  -  node_2->y;

    float is_len = std::sqrt(dx*dx   +  dy*dy);
    dx = dx/is_len; // normalize
    dy = dy/is_len;

    float dl = is_len - length;

    float force = dl*strength;

    node_1->vx -= dx*force/node_1->mass;
    node_1->vy -= dy*force/node_1->mass;
    node_2->vx += dx*force/node_2->mass;
    node_2->vy += dy*force/node_2->mass;
  }

  void draw(sf::RenderWindow &window)
  {
    sf::Vertex line[] =
    {
        sf::Vertex(sf::Vector2f(node_1->x, node_1->y)),
        sf::Vertex(sf::Vector2f(node_2->x, node_2->y))
    };

    window.draw(line, 2, sf::Lines, sf::BlendAdd);
  }
};

struct Robot
{
  std::vector<Node*> node_list;
  std::vector<Connector*> con_list;

  NNetwork net = NNetwork(3);

  Robot(float x_off, float y_off)
  {
    int r = 100 + 156*((float) rand() / (RAND_MAX));
    int g = 100 + 156*((float) rand() / (RAND_MAX));
    int b = 100 + 156*((float) rand() / (RAND_MAX));
    float radius = 4;

    Node* n1 = new Node(0 + x_off,0 + y_off, radius, sf::Color(r,g,b));
    Node* n2 = new Node(20 + x_off,0 + y_off, radius, sf::Color(r,g,b));
    Node* n3 = new Node(10 + x_off,15 + y_off, radius, sf::Color(r,g,b));
    //Node* n4 = new Node(10 + x_off,-15 + y_off, radius, sf::Color(r,g,b));

    node_list.push_back(n1);
    node_list.push_back(n2);
    node_list.push_back(n3);
    //node_list.push_back(n4);

    con_list.push_back(new Connector(n1, n2));
    con_list.push_back(new Connector(n1, n3));
    con_list.push_back(new Connector(n2, n3));

    /*
    con_list.push_back(new Connector(n1, n4));
    con_list.push_back(new Connector(n2, n4));
    con_list.push_back(new Connector(n3, n4));
    */

    net = NNetwork(3);//node_list.size()
  }

  void update()
  {
    std::vector<float> input(node_list.size());
    std::vector<float> output(net.output_size);
    net.compute(input, output);

    for (std::vector<Connector*>::iterator it = con_list.begin(); it != con_list.end(); it++)
      {
          (**it).update();
      }

    for (std::vector<Node*>::iterator it = node_list.begin(); it != node_list.end(); it++)
      {
          (**it).update();
      }
  }

  void draw(sf::RenderWindow &window, sf::Font &font)
  {
    net.draw(window, font, node_list[0]->x, node_list[0]->y );

    for (std::vector<Connector*>::iterator it = con_list.begin(); it != con_list.end(); it++)
      {
          (**it).draw(window);
      }

    for (std::vector<Node*>::iterator it = node_list.begin(); it != node_list.end(); it++)
      {
          (**it).draw(window);
      }


  }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "neural robots");
    window.setVerticalSyncEnabled(true);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf"))
    {
        std::cout << "font could not be loaded!" << std::endl;
    }

    std::list<Robot*> robot_list;

    for(int i=0; i<10; i++)
    {
      float xx = 800*((float) rand() / (RAND_MAX));
      float yy = 600*((float) rand() / (RAND_MAX));
      Robot* n = new Robot(xx,yy);
      robot_list.push_back(n);
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

        for (std::list<Robot*>::iterator it = robot_list.begin(); it != robot_list.end(); it++)
        {
            (**it).update();
        }

        for (std::list<Robot*>::iterator it = robot_list.begin(); it != robot_list.end(); it++)
        {
            (**it).draw(window, font);
        }

        window.display();
    }

    return 0;
}
