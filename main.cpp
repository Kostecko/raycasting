#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <numbers>
#include <cmath>
#include <algorithm>

using namespace std;
using namespace sf;

//GLOBAL VARIABLES

const int _winwidth = 800;
const int _winheight = 800;
const int _cellsize = 80;

const float speed = 80;
const float rotspeed = 90;

const int fov = 60;
const float fovaccuracy = 120;

const vector<string> mapchar = {
    "##########",
    "#        #",
    "# ##  ## #",
    "##      ##",
    "#  #  #  #",
    "#        #",
    "#  #  #  #",
    "#  ####  #",
    "#        #",
    "##########"
};



//KONTROLKI

bool grid = 1;
bool win2dVisible = 1;
bool win3dVisible = 1;



//FUNCTIONS, STRUCTURES, CLASSES

struct Vector2b
{
    bool x;
    bool y;
};

struct rayCast{
    Vector2f position;
    float length;
    float angle;
    bool isVertical;
};

Vector2b collision(Vector2f pos, float radius, Vector2f offset, float dt) {
    int oldx = floor((pos.x + offset.x * radius) / _cellsize);
    int oldy = floor((pos.y + offset.y * radius) / _cellsize);

    //OLD POS + MOVE + RADIUS
    int newx = floor((pos.x + offset.x * dt * speed + offset.x * radius)/_cellsize);
    int newy = floor((pos.y + offset.y * dt * speed + offset.y * radius)/_cellsize);

    return { mapchar[oldy][newx] == '#', mapchar[newy][oldx] == '#' };
}

Vector2i toMapPos(Vector2f pos){
    if (pos.x < 0 || pos.x >= _winwidth || pos.y < 0 || pos.y >= _winheight || !isfinite(pos.x) || !isfinite(pos.y)) return { -1,-1 };

    return { int(pos.x / _cellsize), int(pos.y / _cellsize) };
}

int getMark(float angle) {
    if (angle < 0) return -1;
    if (angle > 0) return 1;
    return 0;
}

//MAIN

int main() {
    RenderWindow window3d(VideoMode({ _winwidth, _winheight }), "3D");
    RenderWindow window2d(VideoMode({ _winwidth, _winheight }), "2D");

    window2d.setVisible(win2dVisible);
    window3d.setVisible(win3dVisible);
    

    Clock clock;
    float dt=0;

    float protrad;
    float protdeg;
    float gms;
    float gmc;

    bool moving = false;

    RectangleShape cellshape(Vector2f(_cellsize, _cellsize));
    RectangleShape line({ 1, 200 });
    CircleShape player(10.f);
    CircleShape point(2.5);

    Font font("assets/Retro.ttf");

    Vector2f offset;
    Vector2f ppos;
    Texture ptxt;

    vector<RectangleShape>map;

    vector<rayCast> rays;

    vector<RectangleShape>gridLines;

    ptxt.loadFromFile("assets/triangle.png");

    cellshape.setFillColor(Color::Blue);

    player.setOrigin({10, 10});
    player.setPosition({440, 440});
    player.setTexture(&ptxt);
    player.setRotation(degrees(30));

    point.setOrigin({ 2.5,2.5 });

    while (window2d.isOpen() && window3d.isOpen()) {
        while (const optional event = window2d.pollEvent()){
            if (event->is<Event::Closed>()) 
                window2d.close();
                
            if (const auto* keypressed = event->getIf<Event::KeyPressed>()) 
                if (keypressed->scancode == Keyboard::Scancode::Space)
                        cout << "BREAKPOINT!\n";
        }
        while (const optional event = window3d.pollEvent()) {
            if (event->is<Event::Closed>()) window3d.close();
        }

        //VARIABLES
        {
            window2d.setTitle("2D\tFPS: " + to_string(int(1.f / dt)));
            dt = clock.restart().asSeconds();

            protrad = player.getRotation().asRadians();
            protdeg = player.getRotation().asDegrees();
            ppos = player.getPosition();

            map.clear();
            rays.clear();
            gridLines.clear();
        }

        //2D MAP GRID
        {
            if (grid) {
                RectangleShape gridLine;
                gridLine.setFillColor(Color(50, 50, 50));

                gridLine.setSize({_winwidth, 1});
                for (int y = 0; y <= _winheight / _cellsize; y++) {
                    gridLine.setPosition({0, y * float(_cellsize)});
                    gridLines.push_back(gridLine);
                }

                gridLine.setSize({1, _winheight});
                for (int x = 0; x <= _winwidth / _cellsize; x++) {
                    gridLine.setPosition({x * float(_cellsize), 0 });
                    gridLines.push_back(gridLine);
                }
            }
        }


        //RAYCASTER
        {
            Vector2i ppom = toMapPos(ppos);
            for(float f = -fov/2.f; f<fov/2.f; f+=(fov/fovaccuracy)){
                float angrad = degrees(protdeg+f).asRadians();
                int mapX = ppom.x;
                int mapY = ppom.y;

                Vector2f rayDir = {
                    sin(angrad),
                    -cos(angrad)
                };
                Vector2i step = {
                    rayDir.x < 0 ? -1 : 1,
                    rayDir.y < 0 ? -1 : 1,
                };
                Vector2f delta = {
                    abs(rayDir.x) < 0.000001f ? INFINITY : float(_cellsize) / abs(rayDir.x),
                    abs(rayDir.y) < 0.000001f ? INFINITY : float(_cellsize) / abs(rayDir.y),
                };
                Vector2f side = {
                    rayDir.x < 0 ?
                    abs(rayDir.x) < 0.00001f ? INFINITY : (ppos.x - ppom.x * _cellsize) / abs(rayDir.x) :
                    abs(rayDir.x) < 0.00001f ? INFINITY : ((ppom.x + 1) * _cellsize - ppos.x) / abs(rayDir.x),

                    rayDir.y < 0 ?
                    abs(rayDir.y) < 0.00001f ? INFINITY : (ppos.y - ppom.y * _cellsize) / abs(rayDir.y) :
                    abs(rayDir.y) < 0.00001f ? INFINITY : ((ppom.y + 1) * _cellsize - ppos.y) / abs(rayDir.y)
                };

                bool hit = false;

                while(!hit){
                    if(side.x < side.y){
                        mapX += step.x;

                        if(mapchar[mapY][mapX] != ' '){
                            rays.push_back({{ppos.x + rayDir.x * side.x, ppos.y + rayDir.y * side.x}, side.x, angrad, true});
                            hit = true;
                        }
                        side.x += delta.x;
                    }
                    else{
                        mapY += step.y;

                        if(mapchar[mapY][mapX] != ' '){
                            rays.push_back({{ppos.x + rayDir.x * side.y, ppos.y + rayDir.y * side.y}, side.y, angrad, false});
                            hit = true;
                        }
                        side.y += delta.y;
                    }
                }
            }
        }

        //MOVING & STEERING
        {

            if (Keyboard::isKeyPressed(Keyboard::Key::Right)) {
                player.rotate(degrees(rotspeed * dt));
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::Left)) {
                player.rotate(degrees(-rotspeed * dt));
            }

            if (Keyboard::isKeyPressed(Keyboard::Key::W) || Keyboard::isKeyPressed(Keyboard::Key::Up)) {
                offset = { sin(protrad), -cos(protrad) };
                moving = true;
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::S) || Keyboard::isKeyPressed(Keyboard::Key::Down)) {
                offset = { -sin(protrad), cos(protrad) };
                moving = true;
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::A)) {
                offset = { -cos(protrad), -sin(protrad) };
                moving = true;
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::D)) {
                offset = { cos(protrad), sin(protrad) };
                moving = true;
            }
            if (moving) {
                Vector2b col = collision(ppos, player.getRadius(), offset, dt);
                player.move({ offset.x * speed * dt * !col.x, offset.y * speed * dt * !col.y });
            }
            moving = false;
        }


        
        //3D RENDER
        
        {
            RectangleShape block;
            block.setFillColor(Color::Blue);
            for (int i = 0; i < rays.size(); i++) {
                float scale = 20.0 / rays[i].length;
                float chunkwidth = float(_winwidth) / float(rays.size());
                float chunkheight = _winheight * scale;

                block.setSize({chunkwidth,chunkheight});
                block.setOrigin({ 0, chunkheight / 2 });
                block.setPosition({ i * chunkwidth, _winheight/2});

                if(rays[i].isVertical) block.setFillColor(Color(0, 0, 255));
                else block.setFillColor(Color(0, 0, 200));

                map.push_back(block);
            }
        }


        
        //DRAWING
        {
            window2d.clear();
            window3d.clear();

            //2D VIEW
            for (int y = 0; y < _winheight / _cellsize; y++) {
                for (int x = 0; x < _winheight / _cellsize; x++) {
                    if (mapchar[y][x] == '#') {
                        cellshape.setFillColor(Color::Blue);
                        cellshape.setPosition(Vector2f(_cellsize * x, _cellsize * y));
                        window2d.draw(cellshape);
                    }
                    else if (mapchar[y][x] == 'C') {
                        cellshape.setFillColor(Color::Yellow);
                        cellshape.setPosition(Vector2f(_cellsize * x, _cellsize * y));
                        window2d.draw(cellshape);
                    }
                    else if (mapchar[y][x] == 'G') {
                        cellshape.setFillColor(Color::Green);
                        cellshape.setPosition(Vector2f(_cellsize * x, _cellsize * y));
                        window2d.draw(cellshape);
                    }
                    else if (mapchar[y][x] == 'M') {
                        cellshape.setFillColor(Color::Magenta);
                        cellshape.setPosition(Vector2f(_cellsize * x, _cellsize * y));
                        window2d.draw(cellshape);
                    }
                    else if (mapchar[y][x] == ' ') {
                        continue;
                    }
                }
            }

            if (grid)
                for (auto& g : gridLines)
                    window2d.draw(g);

            window2d.draw(player);

            for (auto& p : rays){
                point.setPosition(p.position);
                window2d.draw(point);
            }
            
            

            //3D VIEW

            for (auto& b : map)
                window3d.draw(b);

            window2d.display();
            window3d.display();
        }
    }
    return 0;

}