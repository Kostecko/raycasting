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

/*
vector<string> mapchar = {
    "          ",
    " ######## ",
    " #      # ",
    " #      # ",
    " #      # ",
    " #      # ",
    " #      # ",
    " #      # ",
    " ######## ",
    "          "
};
*/
/*
const vector<string> mapchar = {
    "##########",
    "#        #",
    "#        #",
    "#        #",
    "#        #",
    "#        #",
    "#        #",
    "#        #",
    "#        #",
    "##########"

};*/


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
bool consoleVisible = 0;





//FUNCTIONS, STRUCTURES, CLASSES

struct Vector2b
{
    bool x;
    bool y;
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

    int n = 0;
    bool moving = false;
    
    float halfcell = _cellsize / 2.f;

    RectangleShape cellshape(Vector2f(_cellsize, _cellsize));
    RectangleShape block;
    RectangleShape line({ 1, 200 });
    CircleShape player(10.f);
    CircleShape point(2.5);

    Font font("assets/Retro.ttf");

    Vector2f offset;
    Vector2f ppos;
    Texture ptxt;

    vector<RectangleShape>map;

    vector<float> lens;
    vector<RectangleShape>lines;
    vector<CircleShape> raypoints;

    vector<RectangleShape>gridLines;

    ptxt.loadFromFile("assets/triangle.png");

    cellshape.setFillColor(Color::Blue);

    player.setOrigin({10, 10});
    player.setPosition({440, 440});
    player.setTexture(&ptxt);
    player.setRotation(degrees(30));
    
    line.setFillColor(Color::Red);
    
    block.setFillColor(Color::Blue);

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
            if (event->is<Event::Closed>())
                window3d.close();

            if (const auto* keypressed = event->getIf<Event::KeyPressed>()) {
                if (keypressed->scancode == Keyboard::Scancode::N)
                    if (n < lens.size())
                        n += 1;
                if (keypressed->scancode == Keyboard::Scancode::M)
                    if (n > 0)
                        n -= 1;
                
            }

        }

        //VARIABLES
        {
            window2d.setTitle("2D\tFPS: " + to_string(int(1.f / dt)));

            dt = clock.restart().asSeconds();

            protrad = player.getRotation().asRadians();
            protdeg = player.getRotation().asDegrees();
             
            ppos = player.getPosition();

            lines.clear();
            lens.clear();
            map.clear();
            raypoints.clear();
            gridLines.clear();

            /*
            mapchar = {
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
            };*/
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


        //LINEs {}
        {
            line.setFillColor(Color::Red);
            line.setPosition(ppos);
            line.setRotation(degrees(protdeg));
            line.setSize({ 1,1000 });
            line.setOrigin({ 0.5,1000 });
            lines.push_back(line);
        }

        Vector2i ppom = toMapPos(ppos);

        for(float f = -fov/2.f;f<fov/2.f;f+=(fov/fovaccuracy)){
            float angdeg = protdeg + f;
            float angrad = degrees(angdeg).asRadians();

            gms = getMark(sin(angrad));
            gmc = getMark(cos(angrad));

            vector<Vector2f> rppos; //raypoints position (do sortowania)

            //NOWE PODEJSCIE, PROBUJE ZROBIC DDA

            Vector2f vertical;
            Vector2f horizontal;

            float dx = _cellsize * (gms == 1? 1.f:0.f) - fmod(ppos.x, _cellsize);
            float dy = _cellsize * (gmc == 1? 0:1) - fmod(ppos.y, _cellsize);

            for(int i =0;i<10;i++){
                float xoff = dx + _cellsize * i * gms;
                float rpx = ppos.x + xoff;
                float rpy = ppos.y - (tan(angrad) == 0 ? 0 : xoff / tan(angrad));

                Vector2i rpi = toMapPos({rpx, rpy});

                if(rpi.x == -1) continue;

                if(mapchar[rpi.y][rpi.x - (gms==1? 0:1)] != ' '){
                    vertical = {rpx, rpy};
                    break;
                    //sqrt((ppos.x - rpx) * (ppos.x - rpx) + (ppos.y - rpy) * (ppos.y - rpy));
                }
            }

            for(int i =0;i<10;i++){
                float yoff = dy - _cellsize * i * gmc;
                
                float rpx = ppos.x - yoff * tan(angrad);
                float rpy = ppos.y + yoff;

                Vector2i rpi = toMapPos({rpx, rpy});

                if(rpi.x == -1) continue;

                if(mapchar[rpi.y - (gmc == 1?1:0)][rpi.x] != ' '){
                    horizontal = {rpx, rpy};
                    //sqrt((ppos.x - rpx) * (ppos.x - rpx) + (ppos.y - rpy) * (ppos.y - rpy));
                    break;
                }
            }
            point.setPosition(min(vertical, horizontal, [ppos](Vector2f a, Vector2f b){
                a.x -= ppos.x; a.y -= ppos.y;
                b.x -= ppos.x; b.y -= ppos.y;
                return sqrt(a.x * a.x + a.y * a.y) < sqrt(b.x * b.x + b.y * b.y); }));

            raypoints.push_back(point);
            
            float x = ppos.x - point.getPosition().x;
            float y = ppos.y - point.getPosition().y;

            lens.push_back(sqrt(x*x + y*y));
        }

        ;//MOVING & STEERING
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


        
        ;//3D RENDER
        
        {
            for (int i = 0; i < lens.size(); i++) {
                float scale = 20.0 / lens[i];
                float chunkwidth = float(_winwidth) / float(lens.size());
                float chunkheight = _winheight * scale;

                block.setSize({chunkwidth,chunkheight});
                block.setOrigin({ 0, chunkheight / 2 });
                block.setPosition({ i * chunkwidth, _winheight/2});

                map.push_back(block);
            }
        }


        
        ;//DRAWING
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


            for (auto& l : lines)
                window2d.draw(l);

            if (grid)
                for (auto& g : gridLines)
                    window2d.draw(g);

            window2d.draw(player);

            for (auto& p : raypoints) 
                window2d.draw(p);
            
            

            //3D VIEW

            for (auto& b : map)
                window3d.draw(b);

            window2d.display();
            window3d.display();
        }
    }
    return 0;

}
