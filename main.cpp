#include <curses.h>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <ctime>

//Hi! If u are trying to edit this code to tweak my game feel free to do so! I have left some comments that can explain which part of the code is what, but you might still have to dig a bit cos it is not the most optimized :P

struct catsprite {
    std::vector<std::string> sprite;

};

struct bullet {
    int x, y;
    int dx;
};

struct mouse {
    int x, y;
    bool alive;
    int row;
    int fdel;
};

struct powerup {
    int x, y;
    int type;

    // 1 -> spreadshot
    // 2 -> frenzy
    // 3 -> double points (wip)
    // 4 -> bomb
};

struct explosion {
    int x,y;
    int timer;
    int cdel;
};

struct bgstar{
    int x, y;
    int speed;
};

int main (){
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(FALSE);
    start_color();

    init_pair(1, COLOR_YELLOW, COLOR_BLACK); //cat
    init_pair(2, COLOR_RED, COLOR_BLACK); //evil stuff such as bomb and explosion
    init_pair(3, COLOR_CYAN, COLOR_BLACK); //mice
    init_pair(4, COLOR_GREEN, COLOR_BLACK); // powerup
    init_pair(5, COLOR_WHITE, COLOR_BLACK); // background

    bkgd(COLOR_PAIR(5));

    catsprite cat = {
        { "=^. .^=",
          "=^- -^=", 
          "=^>.<^=",
          "=^o o^=",
          "=^! !^=",
        }
    };

    srand(time(0));

    int currsprite = 0;

    int Xborder, Yborder;
    getmaxyx(stdscr, Yborder, Xborder); //window/termial broder

    if (Yborder < 22 || Xborder < 50){
        endwin();
        printf("Terminal too small: please increase the window size!\n" );
        return 1;
    }

    int Xpl = Xborder / 2 - 2;
    int Ypl = Yborder - 4;

    int lives = 3; //# of bombs the cat can withstanc
    int nextlife = 5000; //# of points for an extra life
    int hurtface = 0;

    bool r = true;
    int blinktimer = 160;

    float cooldown = 0;

    std::vector<bullet> bullets;
    int btick = 0;
    int fireface = 0;

    std::vector<powerup> powerups;
    int ptick = 0;
    int pface = 0;
    int poweruptime = 0;
    int powerup = 0;

    std::vector<explosion> boom;
    int bombblink = 31;
    int bobmtype = 0;

    int maxbullets[] = { 2, 6, 4, 2};
    const char* powername[] { "none", "Spreadshot", "Frenzy", "Double points"};

    std::vector<mouse> mice;
    int dir = 1;
    int mtick = 0;
    int s = 0;
    int wave = 1;
    int wstart = 0;
    int entrtick = 0;

    auto Wave = [&](){ //initiate spawn of mice
        mice.clear();
        bullets.clear(); 
        powerups.clear(); 
        boom.clear();

        dir = 1;
        mtick = 0;

        for (int row = 0; row < 4; row++){
            for (int col = 0; col < 8; col++){
                mice.push_back({10 + col * 5, -3, true, 3 + row * 2, col * 3 + row * 8});
            }
        }
        wstart = 62;
    };

    std::vector<bgstar> stars;
    for (int i = 0; i < 40; i++){
        stars.push_back({rand() % Xborder, rand() % Yborder, 3 + rand() % 4});
    }
    int stick = 0;

    bool titlescreen = true;
    int titltick = 0;

    while (titlescreen == true){ //titlescreen this while function
        titltick++;

        int inp = getch();
        if (inp == 'q'){
            endwin();
            return 0;
        }
        if (inp == ' '){
            titlescreen = false;
        }

        stick++; //bg stars logic under here
        for (int i = 0; i < stars.size(); i++){
            if (stick % stars[i].speed == 0){
                stars[i].y++;
                if (stars[i].y >= Yborder){
                    stars[i].y = 0;
                    stars[i].x = rand() % Xborder;
                    stars[i].speed = 3 + rand() % 4;
                }
            }
        }

       erase();
       
        for (int i = 0; i < stars.size(); i++){
            if (rand() % 40 == 0){
                continue;
            }
            attron(A_DIM);
            if (stars[i].speed <= 4){
                mvprintw(stars[i].y, stars[i].x, "*");
            }
            else {
                mvprintw(stars[i].y, stars[i].x, ".");
            }
            attroff(A_DIM);
        }

        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(Yborder / 2 - 4, Xborder / 2 - 12, "C A T    I N V A D E R S");
        mvprintw(Yborder / 2 - 3, Xborder / 2 - 20, "made by pav! - inspo from space invaders");
        attroff(COLOR_PAIR(1) | A_BOLD);

        attron(COLOR_PAIR(1));
        mvprintw(Yborder / 2 - 1, Xborder / 2 - 3, "%s", cat.sprite[(titltick % 80 > 72) ? 1 : 0].c_str());
        attroff(COLOR_PAIR(1));

        if ((titltick / 16) % 2 == 0){
            mvprintw(Yborder / 2 + 2, Xborder / 2 - 10, "PRESS SPACE TO START");
        }

        mvprintw(Yborder / 2 + 4, Xborder / 2 - 15, "A/D move   SPACE shoot   Q quit");

        wnoutrefresh(stdscr);
        doupdate();
        napms(32);
    }

    Wave();

    beep();
    while (r){
        blinktimer--;

        if (lives <= 0){
            r = false;
        }
        
        if (bombblink > 16){
            bobmtype = 0;
            bombblink--;
        }
        else if (bombblink <= 16 && bombblink > 0) {
            bobmtype = 1;
            bombblink--;
        }
        else {
            bombblink = 31;
        }

        if (poweruptime > 0){
            poweruptime--;
        }
        else {
            powerup = 0;
        }

        if (cooldown > 0){
            cooldown--;
        }

        if (blinktimer > 155){
            currsprite = 1;
        }
        else if (blinktimer == 0){
            blinktimer = 160;
        }
        else {
            currsprite = 0;
        }

        if (s >= nextlife){
            lives++;
            nextlife += 5000; 
        }

        int ch = getch();
        
        stick++; //bg stars logic under here
        for (int i = 0; i < stars.size(); i++){
            if (stick % stars[i].speed == 0){
                stars[i].y++;
                if (stars[i].y >= Yborder){
                    stars[i].y = 0;
                    stars[i].x = rand() % Xborder;
                    stars[i].speed = 3 + rand() % 4;
                }
            }
        }



        //keypresses and what they do belows VVV
        if (ch == 'q'){ 
            r = 0;
        }
        if ((ch == 'a' || ch == KEY_LEFT) && Xpl > 3){
            Xpl--;
        }
        if ((ch == 'd' || ch == KEY_RIGHT) && Xpl < Xborder - 4){
            Xpl++;
        }
        if (ch == ' ' && cooldown == 0){
            switch (powerup){
                case 1:
                    if (bullets.size() <= 5){
                    bullets.push_back({Xpl, Ypl - 1, -1});
                    bullets.push_back({Xpl, Ypl - 1, 0});
                    bullets.push_back({Xpl, Ypl - 1, 1});
                    fireface = 10;
                    beep();
                    cooldown = 32;
                    }
                    break;
                case 2: 
                    if (bullets.size() <= 3){
                        bullets.push_back({Xpl, Ypl - 1, 0});
                        fireface = 10;
                        beep();
                        cooldown = 8;
                    }
                    break;
                default:
                    if (bullets.size() <= 1){
                        bullets.push_back({Xpl, Ypl - 1, 0});
                        fireface = 10;
                        beep();
                        cooldown = 32;
                    }
                    break;
            }
        }
        if (ch == '1') {
            powerups.push_back({Xpl, 1, 1});
        }

        if (ch == '2') {
            powerups.push_back({Xpl, 1, 2});
        }
        if (ch == '3'){
            powerups.push_back({Xpl, 1, 3});
        }
        if (ch == '4') {
            powerups.push_back({Xpl, 1, 4});
        }

        if (hurtface > 0){
            currsprite = 2;
            hurtface--;
        }

        if (pface > 0){
            currsprite = 4;
            pface--;
        }

        if (fireface > 0){
            currsprite = 3;
            fireface--;
        }

        ptick++; //powerup logic
        if (ptick > 6){
            ptick = 0;
            for (int i = 0; i < powerups.size(); i++){
                powerups[i].y++;
            }

            for (int i = powerups.size() - 1; i >= 0; i--){
                if (powerups[i].y == Ypl && (powerups[i].x >= Xpl - 3 && powerups[i].x <= Xpl + 3)){
                    switch (powerups[i].type){
                        case 4:
                            boom.push_back({powerups[i].x, powerups[i].y, 16, 0});
                            break;
                        default:
                            beep();
                            powerup = powerups[i].type;
                            pface = 10;
                            poweruptime = 170;
                            if (powerup == 3){
                                s+= 200;
                            }
                            else {
                                s += 100;
                            }
                            break;  
                    }
                    powerups.erase(powerups.begin() + i);
                }
            }

            for (int i = powerups.size() - 1; i >= 0; i--){
                if (powerups[i].y > Ypl){
                    powerups.erase(powerups.begin() + i);
                }
            }
        }


        btick++; //bulet logic
        if (btick > 5){
            btick = 0;
            for (int i = 0; i < bullets.size(); i++){
                bullets[i].y--;
                bullets[i].x += bullets[i].dx;
            }

            for (int i = bullets.size() - 1; i >= 0; i--){
                if (bullets[i].y < 0){
                    bullets.erase(bullets.begin() + i);
                    continue;
                }

                for (int p = powerups.size() - 1; p >= 0; p--){
                    if (powerups[p].type == 4 && powerups[p].x == bullets[i].x && abs(powerups[p].y - bullets[i].y) <= 1){
                        boom.push_back({powerups[p].x, powerups[p].y, 16, 0});
                        if (powerup == 3){
                            s += 30;
                        }
                        else {
                            s += 15;
                        }
                        bullets.erase(bullets.begin() + i);
                        powerups.erase(powerups.begin() + p);
                        break;
                    }
                }
            }

            for (int i = bullets.size() - 1; i >= 0; i--){
                for (int m = 0; m < mice.size(); m++){
                    if (mice[m].alive == false || mice[m].y < 3){
                        continue;
                    }
                    if (abs(mice[m].y - bullets[i].y) <= 1 && bullets[i].x >= mice[m].x && bullets[i]. x <= mice[m].x + 2){
                        mice[m].alive = false;
                        beep();
                        if (powerup == 3){
                            s += 100;
                        }
                        else {
                            s += 50;
                        }
                        if (rand() % 5 == 0){
                            powerups.push_back({mice[m].x + 1, mice[m].y + 1, 1 + (rand() % 3)});
                        }
                        bullets.erase(bullets.begin() + i);
                        break;
                    }
                }
            }
        }



        for (int k = 0; k < boom.size(); k++){ // bomb explosion logic
            if (boom[k].cdel > 0 || boom[k].timer != 16){
                continue;
            }

            beep();

            int x = boom[k].x; 
            int y = boom[k].y;  

            for (int i = bullets.size() - 1; i >= 0; i--){
                if (abs(x - bullets[i].x) <= 2 && abs(y - bullets[i].y) <= 2 ) {
                    bullets.erase(bullets.begin() + i);
                }
            }

            for (int i = powerups.size() - 1; i >= 0; i--){
                if (abs(x - powerups[i].x) <= 2 && abs(y - powerups[i].y) <= 2 ){
                    if (powerups[i].type == 4){
                        boom.push_back({powerups[i].x, powerups[i].y, 16, 10});
                        if (powerup == 3){
                            s += 10;
                        }
                        else {
                            s += 5;
                        }
                    }
                    powerups.erase(powerups.begin() + i);
                }
            }

            if (abs(y - Ypl) <= 2 && Xpl + 3 >= x - 2 && Xpl - 3 <= x + 2){
                lives--;
                s -= 250;
                hurtface = 15;
            }

            for (int i = 0; i < mice.size(); i++){
               if (mice[i].alive == false){
                    continue;
               } 
               if (abs(y - mice[i].y) <= 2 && mice[i].x + 2 >= x - 2 && mice[i].x <= x + 2){
                    mice[i].alive = false;
                    if (powerup == 3){
                        s += 100;
                    }
                    else {
                        s += 50;
                    }
                    if (rand() % 3 == 0){
                        powerups.push_back({mice[i].x + 1, mice[i].y + 1, 1 + (rand() % 3)});
                    }
               }
            }
        }

        for (int i = boom.size() - 1; i >= 0; i--){
            if (boom[i].cdel > 0){
                boom[i].cdel--;
            }
            else {
                boom[i].timer--;
                if (boom[i].timer < 1){
                    boom.erase(boom.begin() + i);
                }
            }
        }


        mtick++;  //mice logic from here to belows
        entrtick++;

        std::vector<int> alive;
        int entr = 0;

        for (int i = 0; i < mice.size(); i++){ //counting how many are alive and making the ones that have to enter enter. oh and also the etick % value can be changed for the mice to enter slower or faster
            if (mice[i].alive == false){
                continue;
            }
                alive.push_back(i);
            if (mice[i].fdel > 0){
                mice[i].fdel--;
                entr++;
            }
            else if (mice[i].y < mice[i].row){
                if (entrtick % 2 == 0){
                    mice[i].y++;
                }
                entr++;
            }
        }

        int bobmodd = 1501 - wave * 100; //mice makes bomb to attack the cat
        if (bobmodd < 500) {
            bobmodd = 500;
        }
        if (entr == 0){
            for (int i = 0; i < alive.size(); i++){
                if (rand() % bobmodd == 0){
                    powerups.push_back({mice[alive[i]].x + 1, mice[alive[i]].y + 1, 4});
                }
            }
        }


        if (alive.empty()){ //if all dead make new wave basically
            wave++;
            Wave();
        }

        for (int i = 0; i < alive.size(); i++){
            if (mice[alive[i]].y >= Ypl - 1){
                r = false;
            }
        }

        int mtickd = 5 + (int)alive.size() / 3 - wave / 3; // ticks needed for the mice to move and do other stuff
        if (mtickd < 1){
            mtickd = 1;
        }

        if (entr == 0 && mtick > mtickd) {
            mtick = 0;
            bool hitedg = false;

            for (int i = 0; i < mice.size(); i++){
                if (mice[i].alive == false){
                    continue;
                }
                if ((dir == 1 && mice[i].x + 3 >= Xborder - 1) || (dir == -1 && mice[i].x <= 1)){
                    hitedg = true;
                }
            }

            for (int i = 0; i < mice.size(); i++){
                if (hitedg == true){
                    mice[i].y++;
                }
                else {
                    mice[i].x += dir;
                }
            }
            if (hitedg == true){
                dir = -dir;
            }
        }
        
        erase(); //printing the actuall stuffs such as the mice and the cat and the powerups and the bombs on the screen
        for (int i = 0; i < stars.size(); i++){ //star print
            if (rand() % 40 == 0){
                continue;
            }
            attron(A_DIM);
            if (stars[i].speed <= 4){
                mvprintw(stars[i].y, stars[i].x, "*");
            }
            else {
                mvprintw(stars[i].y, stars[i].x, ".");
            }
            attroff(A_DIM);
        }
        
        
        attron(COLOR_PAIR(1)); //cat print
        mvprintw(Ypl, Xpl - 3, "%s", cat.sprite[currsprite].c_str());
        attroff(COLOR_PAIR(1));
        for (int i = 0; i < bullets.size(); i++){ //bullet print
            attron(A_BOLD);
            mvprintw(bullets[i].y, bullets[i].x, "^");
            attroff(A_BOLD);
        }
        for (int i = 0; i < powerups.size(); i++){ //powerup & bomb print
            switch(powerups[i].type) {
                case 4:
                    attron(COLOR_PAIR(2));
                    if (bobmtype == 1){
                        mvprintw(powerups[i].y, powerups[i].x, "@");
                    }
                    else {
                        mvprintw(powerups[i].y, powerups[i].x, "O");
                    }
                    attroff(COLOR_PAIR(2));
                    break;
                default:
                    attron(COLOR_PAIR(4));
                    mvprintw(powerups[i].y, powerups[i].x, "#");
                    attroff(COLOR_PAIR(4));
                    break;
            }
        }

        attron(COLOR_PAIR(2)); //explosion/boom or whatever print
        for (int i = 0; i < boom.size(); i++){
            if (boom[i].cdel > 0){
                mvprintw(boom[i].y, boom[i].x, bobmtype == 1 ? "@" : "O" );
            }
            else {
                for (int y = -2; y <= 2; y++){
                    mvprintw(boom[i].y + y, boom[i].x-2, "*****");
                }
            }
        }
        attroff(COLOR_PAIR(2));
        attron(COLOR_PAIR(3));
        for (int i = 0; i < mice.size(); i++){ //mice print
            if (mice[i].alive == true){
                mvprintw(mice[i].y, mice[i].x, dir == 1 ? "~o>" : "<o~");
            }
        }
        attroff(COLOR_PAIR(3)); //HUD PRINT
        mvprintw(0, 0, "bullets=%d/%d cooldown=%.1f powertime=%d power=%s", (int)bullets.size(), maxbullets[powerup], cooldown/31, poweruptime/31, powername[powerup]);
        mvprintw(1, 0, "lives=");
        attron(COLOR_PAIR(2));
        printw("%d", lives);
        attroff(COLOR_PAIR(2));
        printw(" score=%d", s);
        attroff(COLOR_PAIR(2));
        mvprintw(2, 0, "wave = %d", wave);

        if (wstart > 0){ //The giant WAVE: # thing that appears
            wstart--;
            mvprintw(Yborder / 2, Xborder / 2 - 5, "WAVE %d", wave);
        }
        wnoutrefresh(stdscr);
        doupdate();

        napms(32);
    }

    endwin();

    int gotick = 0;
    bool goscreen = true;

    while (goscreen == true){ //Game over screen
        gotick++;

        int inp = getch();
        if (gotick > 100 && inp == ' ' || inp == 'q'){
            goscreen = false;
        }

         stick++; //bg stars logic under here
        for (int i = 0; i < stars.size(); i++){
            if (stick % stars[i].speed == 0){
                stars[i].y++;
                if (stars[i].y >= Yborder){
                    stars[i].y = 0;
                    stars[i].x = rand() % Xborder;
                    stars[i].speed = 3 + rand() % 4;
                }
            }
        }

       erase();
       
        for (int i = 0; i < stars.size(); i++){
            if (rand() % 40 == 0){
                continue;
            }
            attron(A_DIM);
            if (stars[i].speed <= 4){
                mvprintw(stars[i].y, stars[i].x, "*");
            }
            else {
                mvprintw(stars[i].y, stars[i].x, ".");
            }
            attroff(A_DIM);
        }

        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(Yborder / 2 - 4, Xborder / 2 - 5, "GAME  OVER");
        attroff(COLOR_PAIR(2) | A_BOLD);

        attron(COLOR_PAIR(1));
        mvprintw(Yborder / 2 - 2, Xborder / 2 - 3, "%s", cat.sprite[2].c_str());
        attroff(COLOR_PAIR(1));

        mvprintw(Yborder / 2, Xborder / 2 - 5, "SCORE: %d", s);
        mvprintw(Yborder / 2 + 1, Xborder / 2 - 5, "WAVE: %d", wave);

        if ((gotick / 16) % 2 == 0){
            mvprintw(Yborder / 2 + 4, Xborder / 2 - 9, "PRESS SPACE TO EXIT");
        }

        wnoutrefresh(stdscr);
        doupdate();
        napms(32);
    }

    return 0;
}