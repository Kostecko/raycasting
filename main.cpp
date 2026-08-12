#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <numbers>
#include <cmath>
#include <windows.h>

using namespace std;
using namespace sf;

//GLOBAL VARIABLES

const int _winwidth = 800;
const int _winheight = 800;
const int _cellsize = 80;

const float speed = 200;
const float rotspeed = 90;

const int fov = 30;
const float fovaccuracy = 60;

/**/
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






//KONTROLKI

bool grid = 1;
bool win2dVisible = 1;
bool win3dVisible = 0;
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

    bool x = false;;
    bool y = false;


    if (mapchar[oldy][newx] == '#') {
        x = true;
    }
    if (mapchar[newy][oldx] == '#') {
        y = true;
    }

    return { x, y };
}

Vector2i toMapPos(Vector2f pos){
    static int errorcounter = 0;
    if (pos.x < 0 || pos.x > _winwidth || pos.y < 0 || pos.y > _winheight) {
        //printf("toMapPos() error!\ttimes: %i\n", errorcounter); errorcounter++; 
        return { -1,-1 };
    }
    return { static_cast<int>(pos.x / _cellsize), static_cast<int>(pos.y / _cellsize) };
}

int getMark(float angle) {
    if (angle < 0) return -1;
    if (angle > 0) return 1;
    return 0;
}


void sortray(vector<CircleShape>& vec, Vector2f ppos) {
    vector<CircleShape> output;

    Vector2f vdist1 = ppos - vec[0].getPosition();
    Vector2f vdist2 = ppos - vec[1].getPosition();

    float fdist1 = sqrt(vdist1.x * vdist1.x + vdist1.y * vdist1.y);
    float fdist2 = sqrt(vdist2.x * vdist2.x + vdist2.y * vdist2.y);

    if (fdist1 < fdist2) {
        output.push_back(vec[0]);
        output.push_back(vec[1]);
    }
    else {
        output.push_back(vec[1]);
        output.push_back(vec[0]);
    }


    vec = output;
}

//MAIN

int main() {
    RenderWindow window3d(VideoMode({ _winwidth, _winheight }), "3D");
    RenderWindow window2d(VideoMode({ _winwidth, _winheight }), "2D");

    window2d.setVisible(win2dVisible);
    window3d.setVisible(win3dVisible);
    ShowWindow(GetConsoleWindow(), consoleVisible);
    

    Clock clock;
    float dt=0;

    float protrad;
    float protdeg;
    int n = 0;
    bool moving = false;

    RectangleShape cellshape(Vector2f(_cellsize, _cellsize));
    RectangleShape block;
    RectangleShape line({ 1, 200 });
    CircleShape player(10.f);
    CircleShape point(5.f);

    Vector2f offset;
    Vector2f ppos;
    Texture ptxt;
    vector<RectangleShape>map;
    vector<RectangleShape>lines;
    vector<float> lens;
    vector<vector<CircleShape>> raypoints;
    vector<RectangleShape>gridLines;
    vector<Vector2i> nbpix;

    ptxt.loadFromFile("triangle.png");

    cellshape.setFillColor(Color::Blue);

    player.setOrigin({10, 10});
    player.setPosition({440, 440});
    player.setTexture(&ptxt);
    player.setRotation(degrees(30));
    
    line.setFillColor(Color::Red);
    
    block.setFillColor(Color::Blue);

    point.setOrigin({ 5,5 });

    while (window2d.isOpen() && window3d.isOpen()) {
        while (const optional event = window2d.pollEvent()){
            if (event->is<Event::Closed>()) 
                window2d.close();
                
            if (const auto* keypressed = event->getIf<Event::KeyPressed>()) {
                if (keypressed->scancode == Keyboard::Scancode::Space)
                    cout << "BREAKPOINT!\n";
            }

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
            window2d.setTitle(to_string(int(1.f/dt)));

            dt = clock.restart().asSeconds();

            protrad = player.getRotation().asRadians();
            protdeg = player.getRotation().asDegrees();
            ppos = player.getPosition();

            lines.clear();
            lens.clear();
            map.clear();
            raypoints.clear();
            gridLines.clear();

            /**/
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
            };
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
            //point.setOrigin({ 5,5 });
            //point.setFillColor(Color::Green);
            //point.setPosition({ ppos.x - xdist, ppos.y + xdist / tan(protrad) });
            //    raypoints.push_back(point);
            //point.setFillColor(Color::Magenta);
            //point.setPosition({ ppos.x + tan(protrad) * ydist,ppos.y - ydist });
            //    raypoints.push_back(point);
            //line.setFillColor(Color::Green);
            //line.setRotation(degrees(0));
            //line.setSize({ xdist,1 });
            //line.setOrigin({ xdist,0.5 });
            //    lines.push_back(line);
            //line.setFillColor(Color::Magenta);
            //line.setRotation(degrees(0));
            //line.setSize({ 1, ydist });
            //line.setOrigin({ 0.5,ydist });
            //    lines.push_back(line);
        }


        float halfcell = _cellsize / 2.f;

        Vector2i ppom = toMapPos(ppos);
        Vector2f cfpp;
        Vector2f pver;
        Vector2f phor;
        Vector2f delta;

        float gms = getMark(sin(protrad));
        float gmc = getMark(cos(protrad));

        bool bver = 0;
        bool bhor = 0;

        vector<Vector2f> rppos; //raypoints position (do sortowania)

        for (int i = 0; i < 5; i++) {
            cfpp = { //center float pixel position
                float((ppom.x + i * gms) * _cellsize + halfcell),
                float((ppom.y - (i + 1) * gmc) * _cellsize + halfcell)
            };

            delta = {
                float(cfpp.x + halfcell * gms - ppos.x),
                float(cfpp.y + halfcell * gmc - ppos.y)
            };

            pver = {
                ppos.x + delta.x,
                ppos.y - delta.x / tan(protrad)
            };

            phor = {
                ppos.x - delta.y * tan(protrad),
                ppos.y + delta.y
            };

            //pointvec.x jest graniczny, punkt jest na nastepnym pixelu

            Vector2i pverm = toMapPos(pver); //Point VERtical on Map
            Vector2i phorm = toMapPos(phor); //Point HORizontal on Map

            



            //Jezeli pixel na ktorym UMOWNIE stoi punkt nie jest pusty, nie rysuje go i break;

            //if (mapchar[pverm.y][pverm.x - (gms == 1 ? 1 : 0)] != ' ') bver = 1; //GREEN
            //if (mapchar[phorm.y - (gmc == 1 ? 0 : 1)][phorm.x] != ' ') bhor = 1; //MAGENTA
            //
            //if (bver || bhor) break;
            //
            //bver = 0;
            //bhor = 0;
            //
            ////Jezeli uderza sciany rysuje ostatn punkt i break;
            //
            //if (mapchar[pverm.y][pverm.x - (gms == 1 ? 0 : 1)] != ' ') { //GREEN
            //    point.setFillColor(Color::Green);
            //    point.setPosition(pver);
            //    raypoints.push_back({ point });
            //    break;
            //}
            //
            //if (mapchar[phorm.y - (gmc == 1 ? 1 : 0)][phorm.x] != ' ') { //MAGENTA
            //    point.setFillColor(Color::Magenta);
            //    point.setPosition(phor);
            //    raypoints.push_back({ point });
            //    break;
            //}

            //point.setFillColor(Color::Green);
            //point.setPosition(pver); 
            //raypoints.push_back({ point }); 
            // 
            //point.setFillColor(Color::Magenta);
            //point.setPosition(phor);
            //raypoints.push_back({ point });

            if (pverm.x != -1) rppos.push_back(pver); //GREEN
            if (phorm.x != -1) rppos.push_back(phor); //MAGENTA
        }

        sort(rppos.begin(), rppos.end(), [](Vector2f a, Vector2f b) {
                return (a.x + a.y) < (b.x + b.y);
            });


        for (int i = 1; i <= rppos.size();i++) {
            point.setFillColor(Color(200.f/i, 200.f/i, 200.f/i));
            point.setPosition(rppos[i-1]);
            raypoints.push_back({ point });
        }

        
        ;//MOVING & STEERING
        {

            if (Keyboard::isKeyPressed(Keyboard::Key::Right)) {
                player.rotate(degrees(rotspeed * dt));
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::Left)) {
                player.rotate(degrees(-rotspeed * dt));
            }

            if (Keyboard::isKeyPressed(Keyboard::Key::W)) {
                offset = { sin(protrad), -cos(protrad) };
                moving = true;
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::S)) {
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

            //window2d.draw(line);

            window2d.draw(player);

            for (auto& v : raypoints)
                for(auto& p : v)
                    window2d.draw(p);

            if(grid)
                for (auto& g : gridLines)
                    window2d.draw(g);

            //window2d.draw(point);

            //3D VIEW

            //for (auto& b : map)
            //    window3d.draw(b);

            window2d.display();
            window3d.display();
        }
    }
    return 0;

}