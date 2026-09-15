#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <numbers>
#include <cmath>
#include <algorithm>
#include <unordered_map>

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
    "# YY  YY #",
    "#Y      Y#",
    "#  M  M  #",
    "#        #",
    "#  G  G  #",
    "#  GGGG  #",
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

struct ray{
    Vector2f position;
    float length;
    Angle angle;
    bool isVertical;
    Color color;
};

Vector2i toMapPos(Vector2f pos){
    if (pos.x < 0 || pos.x >= _winwidth || pos.y < 0 || pos.y >= _winheight || !isfinite(pos.x) || !isfinite(pos.y)) return { -1,-1 };

    return { int(pos.x / _cellsize), int(pos.y / _cellsize) };
}

Vector2b collision(Vector2f pos, Vector2f dir, float radius, float dt) {
    Vector2i oldpos = toMapPos(pos);
    pos = {pos.x + dir.x * speed * dt + dir.x * radius, pos.y + dir.y * speed * dt + dir.y * radius};
    Vector2i newpos = toMapPos(pos);


    bool hitX = mapchar[oldpos.y][newpos.x] != ' ';
    bool hitY = mapchar[newpos.y][oldpos.x] != ' ';
    bool hitDiag = mapchar[newpos.y][newpos.x] != ' ';

    return {hitX || hitDiag, hitY || hitDiag};
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
    Vector2f ppos;

    RectangleShape cellshape(Vector2f(_cellsize, _cellsize));
    cellshape.setFillColor(Color::Blue);

    Texture ptxt;
    ptxt.loadFromFile("assets/triangle.png");
    
    CircleShape player(10.f);
    player.setOrigin({10, 10});
    player.setPosition({440, 440});
    player.setTexture(&ptxt);
    player.setRotation(degrees(30));

    CircleShape point(2.5);
    point.setOrigin({ 2.5,2.5 });

    RectangleShape floor_({_winwidth, _winheight/2});
    floor_.setPosition({0, _winheight/2});
    floor_.setFillColor(Color(100, 100, 100));

    //Font font("assets/Retro.ttf");
    
    vector<RectangleShape>map;
    vector<ray> rays;
    vector<RectangleShape>gridLines;

    unordered_map<char, Color> cell_t{
        {'#', Color::Blue},
        {'Y', Color::Yellow},
        {'G', Color::Green},
        {'M', Color::Magenta}
    };

    //2D MAP GRID
    {
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
            dt = clock.restart().asSeconds();
            window2d.setTitle("2D\tFPS: " + to_string(int(1.f / dt)));

            protrad = player.getRotation().asRadians();
            protdeg = player.getRotation().asDegrees();
            ppos = player.getPosition();

            map.clear();
            rays.clear();
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
                    abs(rayDir.x) < 1e-6f ? INFINITY : float(_cellsize) / abs(rayDir.x),
                    abs(rayDir.y) < 1e-6f ? INFINITY : float(_cellsize) / abs(rayDir.y),
                };
                Vector2f side = {
                    rayDir.x < 0 ?
                    delta.x == INFINITY ? delta.x : (ppos.x - ppom.x * _cellsize) / abs(rayDir.x) :
                    delta.x == INFINITY ? delta.x : ((ppom.x + 1) * _cellsize - ppos.x) / abs(rayDir.x),

                    rayDir.y < 0 ?
                    delta.y == INFINITY ? delta.y : (ppos.y - ppom.y * _cellsize) / abs(rayDir.y) :
                    delta.y == INFINITY ? delta.y : ((ppom.y + 1) * _cellsize - ppos.y) / abs(rayDir.y)
                };

                bool hit = false;

                while(!hit){
                    if(side.x < side.y){
                        mapX += step.x;

                        if(mapchar[mapY][mapX] != ' '){
                            Vector2f position = {ppos.x + rayDir.x * side.x, ppos.y + rayDir.y * side.x};
                            rays.push_back({position, side.x, degrees(f), true, cell_t[mapchar[mapY][mapX]]});
                            hit = true;
                        }
                        side.x += delta.x;
                    }
                    else if(side.y < side.x){
                        mapY += step.y;

                        if(mapchar[mapY][mapX] != ' '){
                            Vector2f position = {ppos.x + rayDir.x * side.y, ppos.y + rayDir.y * side.y};
                            rays.push_back({position, side.y, degrees(f), false, cell_t[mapchar[mapY][mapX]]});
                            hit = true;
                        }
                        side.y += delta.y;
                    }
                    else{
                        int diagX = mapX + step.x;
                        int diagY = mapY + step.y;

                        bool hitX = mapchar[mapY][diagX] != ' ';
                        bool hitY = mapchar[diagY][mapX] != ' ';
                        bool hitDiag = mapchar[diagY][diagX] != ' ';

                        if(hitX || hitY || hitDiag){
                            Color _color = hitX ? cell_t[mapchar[mapY][diagX]] : 
                                (hitY ? cell_t[mapchar[diagY][mapX]] : 
                                cell_t[mapchar[diagY][diagX]]);

                            Vector2f position = {ppos.x + rayDir.x * side.x, ppos.y + rayDir.y * side.x};
                            rays.push_back({position, side.x, degrees(f), true, _color});
                            hit = true;
                        }
                        mapX = diagX;
                        mapY = diagY;
                        
                        side.y +=delta.y;
                        side.x +=delta.x;
                    }
                }
            }
        }

        //MOVING & STEERING
        {
            if (Keyboard::isKeyPressed(Keyboard::Key::Right)) player.rotate(degrees(rotspeed * dt));
            if (Keyboard::isKeyPressed(Keyboard::Key::Left)) player.rotate(degrees(-rotspeed * dt));
            
            float _sin = sin(protrad);
            float _cos = cos(protrad);
            
            Vector2f dir{0.f,0.f}; 

            if(Keyboard::isKeyPressed(Keyboard::Key::W) || 
            Keyboard::isKeyPressed(Keyboard::Key::Up)) dir += {_sin, -_cos};
            
            if(Keyboard::isKeyPressed(Keyboard::Key::S) || 
            Keyboard::isKeyPressed(Keyboard::Key::Down)) dir += {-_sin, _cos};

            if (Keyboard::isKeyPressed(Keyboard::Key::A)) dir += {-_cos, -_sin};
            
            if (Keyboard::isKeyPressed(Keyboard::Key::D)) dir += {_cos, _sin};

            float len = hypot(dir.x, dir.y);

            if(len > 1e-6f){
                dir /= len;
                Vector2b col = collision(ppos, dir, player.getRadius() ,dt);
                player.move({!col.x * dir.x * speed * dt, !col.y * dir.y * speed * dt});
            }
        }
        
        //3D RENDER
        if(win3dVisible){
            RectangleShape block;
            block.setFillColor(Color::Blue);
            for (int i = 0; i < rays.size(); i++) {
                float correctedLength = rays[i].length * cos(rays[i].angle.asRadians());
                float scale = 20.0 / correctedLength;
                float chunkwidth = float(_winwidth) / float(rays.size());
                float chunkheight = _winheight * scale;

                block.setSize({chunkwidth,chunkheight});
                block.setOrigin({ 0, chunkheight / 2 });
                block.setPosition({ i * chunkwidth, _winheight/2});

                if(rays[i].isVertical) block.setFillColor(rays[i].color);
                else block.setFillColor(rays[i].color * Color(150, 150, 150));

                map.push_back(block);
            }
        }

        //DRAWING
        {
            //2D VIEW
            if(win2dVisible){
                window2d.clear();
                for (int y = 0; y < _winheight / _cellsize; y++) {
                    for (int x = 0; x < _winheight / _cellsize; x++) {
                        if (mapchar[y][x] == '#') {
                            cellshape.setFillColor(Color::Blue);
                            cellshape.setPosition(Vector2f(_cellsize * x, _cellsize * y));
                            window2d.draw(cellshape);
                        }
                        else if (mapchar[y][x] == 'Y') {
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
                if (grid) for (auto& g : gridLines) window2d.draw(g);

                window2d.draw(player);

                for (auto& p : rays){
                    point.setPosition(p.position);
                    window2d.draw(point);
                }

                window2d.display();
            }

            //3D VIEW
            if(win3dVisible){
                window3d.clear(Color(0, 153, 153));
                window3d.draw(floor_);
                for (auto& b : map)
                    window3d.draw(b);
                window3d.display();
            }
        }
    }
    return 0;

}