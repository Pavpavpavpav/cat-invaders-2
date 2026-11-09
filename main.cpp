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
    int hp;
};

struct powerup {
    int x, y;
    int type;

    // 1 -> spreadshot
    // 2 -> frenzy
    // 3 -> double points (wip)
    // 4 -> bomb
};

struct popup {
    int x, y;
    int dx;
    int value;
    int timer;
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

struct bossi {
    bool alive = false;
    int x, y;
    int hp, maxhp;
    int dir = 1;
    int mtick = 0;
    int ftick = 0;
    bool uability = false;
    int hface = 0;
    int death = 0;
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
          "=^$ $^=",
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
    int hurtface = 0;

    bool r = true;
    int blinktimer = 160;

    float cooldown = 0;
    int maxcd = 32;
    int maxbullets = 1; //If ur changing it just remember that the max bullet size is this + 1 cos stuff is counted from 0 
    int pwrbulmult[] = {1, 3, 2, 1, 1, 1};

    std::vector<bullet> bullets;
    int btick = 0;
    int fireface = 0;

    std::vector<powerup> powerups;
    int ptick = 0;
    int pface = 0;
    int poweruptime = 0;
    int powertbonus = 0;
    int powerup = 0;

    std::vector<explosion> boom;
    int bombblink = 31;
    int bobmtype = 0;

    const char* powername[] { "none", "Spreadshot", "Frenzy", "Double points", "", "Confusion"};

    std::vector<mouse> mice;
    bossi boss;
    int dir = 1;
    int mtick = 0;
    int s = 0;

    std::vector<popup> popup;
    int combo = 1; //Combo stuff and mice hit stuff here V
    int combot = 0;
    int cpop = 0;

    auto ppoint = [&](int pts, int mx, int my){
        int pcombo = combo;
        s += (combo * pts);
        popup.push_back({mx, my, (rand() % 3) - 1, combo * pts, 20});
        if (combo < 5){
            combo++;
        }
        if (combo == 5 && pcombo < 5){
            cpop = 40;
            beep();
        }
        combot = 62;
    };



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

        if (wave % 5 != 0){
            for (int row = 0; row < 4; row++){
                for (int col = 0; col < 8; col++){
                    mice.push_back({10 + col * 5, -3, true, 3 + row * 2, col * 3 + row * 8, (row > 2 - wave / 5) ? 2 : 1});
                }
            }
        }
        else {
            boss = bossi();
            boss.alive = true;
            boss.x = Xborder / 2 - 4;
            boss.y = -3;
            boss.maxhp = 15 + wave * 2;
            boss.hp = boss.maxhp;
            boss.ftick = 90;
        }
        wstart = 62;
    };


    std::string typch;
    bool gmode = false;
    bool evilcheat = false;
    bool nocd = false;
    bool infbul = false;
    int cheatsactive = 0;

    auto Cheatc = [&](const std::string& code){ //recognize cheat codes if the player is evil to type them, idk if u really can modify this
        if (typch.size () >= code.size() && typch.compare(typch.size() - code.size(), code.size(), code) == 0){
            typch.clear();
            evilcheat = true;
            return true;
        }
        return false;
    };

    std::vector<bgstar> stars;
    for (int i = 0; i < 40; i++){
        stars.push_back({rand() % Xborder, rand() % Yborder, 3 + rand() % 4});
    }
    int stick = 0;

    int boughtam[4] = {0, 0, 0, 0};
    const char* upname[4] = {"+ Life", "+ Pwr Up time", "- Cooldown", "+ Bullet Limit"};
    int price[4] = {4700, 2800, 3600, 3200};

    auto Shop = [&](){   //name should be explanory, tweak from here if u wanna mess with the shop code i wrote
        std::vector<bullet> shb;
        int shx = Xborder / 2;

        int shbtick = 0;
        int shcd = 0;
        int shface = 0;
        int shtick = 0;
        int msgtick = 0;
        int shsprite = 0;

        std::string msg;
        bool sh = true;

        int signw[5] = {3, 3, 3, 3, 6};
        int signx[5];
        
        for (int i = 0; i < 5; i++) {
            signx[i] = Xborder * (i + 1) / 6 - signw[i] / 2;
        }
        int signy = 11;

        flushinp();

        while(sh){
            shtick++;

            int shch = getch(); 

            if (shch == 'q'){ // shop key inputs and what the stuffs they do
                sh = false;
            }
            if ((shch == 'a' || shch == KEY_LEFT) && shx > 3){
                shx--;
            }
            if ((shch == 'd' || shch == KEY_RIGHT) && shx < Xborder - 4){
                shx++;
            }
            if (shch == ' ' && shcd == 0 && shb.size() < 2){
                shb.push_back({shx, Ypl - 1, 0});
                shcd = 12;
                shface = 62;
            }
            
            if (shcd > 0){
                shcd--;
            }

            shsprite = 0;

            if (shface > 0){
                shface--;
                shsprite = 5;
            }

            if (shtick % 100 > 92){
                shsprite = 1;
            }

            shbtick++;
            if (shbtick > 2){
                shbtick = 0;
                for (int i = 0; i < shb.size(); i++){
                    shb[i].y--;
                }
                for (int i = shb.size() - 1; i >= 0; i--){
                    if (shb[i].y < 3){
                        shb.erase(shb.begin() + i);
                        continue;
                    }
                    for (int sign = 0; sign < 5; sign++){
                        if (abs(shb[i].y - signy) <= 1 && shb[i].x >= signx[sign] && shb[i].x <= signx[sign] + signw[sign] - 1){
                            if (sign == 4){
                                sh = false;
                            }
                            else {
                                int cost = price[sign] * (1 + boughtam[sign]);
                                bool max = (sign == 0) ? (lives >= 9) : (boughtam[sign] >= 4);

                                if (max){
                                    msg = "Maximum already owned!";
                                }
                                else if (s < cost){
                                    msg = "Not enough score!";
                                }
                                else {
                                    s -= cost;
                                    boughtam[sign]++;
                                    switch(sign){
                                        case 0:
                                            lives++;
                                            break;
                                        case 1: 
                                            powertbonus += 62;
                                            break;
                                        case 2:
                                            maxcd -= 4;
                                            break;
                                        case 3:
                                            maxbullets++;
                                            break;
                                    }
                                    msg = std::string("Bought: ") + upname[sign];
                                }
                                msgtick = 62;
                            }
                            shb.erase(shb.begin() + i);
                            break;
                        }
                    }
                }
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

            attron(COLOR_PAIR(1) | A_BOLD); // shop title print
            mvprintw(1, Xborder / 2 - 8, "== UPGRADE SHOP ==");
            attroff(COLOR_PAIR(1) | A_BOLD);
            mvprintw(2, (Xborder - (int)snprintf(NULL, 0, "SCORE: %d", s)) / 2, "SCORE: %d", s);

            for (int i = 0; i < 4; i++){    // shop options print
                int cost = price[i] * (1 + boughtam[i]);
                int max = (i == 0) ? (lives >= 9) : (boughtam[i] >= 4);

                if (max){
                    attron(COLOR_PAIR(2));
                }
                else if (s >= cost){
                    attron(COLOR_PAIR(4) | A_BOLD);
                }
                else {
                    attron(A_DIM);
                }
                if (max){
                    mvprintw(4 + i, Xborder / 2 - 15, "[%d] %-16s MAX", i + 1, upname[i]);
                }
                else {
                    mvprintw(4 + i, Xborder /2 - 15, "[%d] %-16s %d pts", i + 1, upname[i], cost);
                }
                if (max){
                    attroff(COLOR_PAIR(2));
                }
                else if (s >= cost){
                    attroff(COLOR_PAIR(4) | A_BOLD);
                }
                else {
                    attroff(A_DIM);
                }
            }

            mvprintw(9, Xborder / 2 - 19, "shoot to buy - shoot exit to well exit");

            for (int sign = 0; sign < 4; sign++){ // shop shoot options print
                mvprintw(signy, signx[sign], "[%d]", sign + 1);
            }

            attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(signy, signx[4], "[EXIT]");
            attroff(COLOR_PAIR(2) | A_BOLD);

            if (msgtick > 0){ //shop msg (if any) printy
                msgtick--;
                mvprintw(13, Xborder / 2 - (int)msg.size() / 2, "%s", msg.c_str());
            }

            for (int i = 0; i < shb.size(); i++){ //bullet print
                attron(A_BOLD);
                mvprintw(shb[i].y, shb[i].x, "$");
                attroff(A_BOLD);
            }

            attron(COLOR_PAIR(1));
            mvprintw(Ypl, shx - 3, "%s", cat.sprite[shsprite].c_str());
            attroff(COLOR_PAIR(1));

            wnoutrefresh(stdscr);
            doupdate();
            napms(32);
        }

        flushinp();
    };

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

        int ch = getch();

        if (ch >= 32 && ch < 127){
            typch += (char)ch;
            if (typch.size() > 32){
                typch.erase(0, typch.size() - 32);
            }
        }

        //Cheat codes if u wanna be evil VVVV
        if (Cheatc("life") || Cheatc("healme")){
            lives++;
        }
        if (Cheatc("god") || Cheatc("adadadadddaadadadaaaadadad")){
            gmode = !gmode;
            if (gmode == true){
                cheatsactive++;
            }
            else {
                cheatsactive--;
            }
        }
        if (Cheatc("bored") || Cheatc("cooldownbegone") || Cheatc("nocd")){
            nocd = !nocd;
            if (nocd == true){
                cheatsactive++;
            }
            else {
                cheatsactive--;
            }
        }
        if (Cheatc("infinity") || Cheatc("nolimit")){
            infbul = !infbul;
            if (infbul == true){
                cheatsactive++;
            }
            else {
                cheatsactive--;
            }
        }
        if (Cheatc("ratpoison")){
            for (int i = 0; i < mice.size(); i++){
                mice[i].alive = false;
            }
        }
        if (Cheatc("bomb") || Cheatc("bobm")){
            powerups.push_back({Xpl, 1, 4});
        }
        if (Cheatc("money") || Cheatc("$$$$$") || Cheatc("rich")){
            s += 15000;
        }
        if (Cheatc("boring")){
            wave = 4;
            for (int i = 0; i < mice.size(); i++){
                mice[i].alive = false;
            }
            boss.alive = false;  
        }

        if (combot > 0){ //combo and the popup logic here
            combot--;
            if (combot == 0){
                combo = 1;
            }
        }

        for (int i = popup.size() - 1; i >= 0; i--){
            popup[i].timer--;
            if (popup[i].timer % 5 == 0){
                popup[i].y--;
                if (popup[i].timer % 10 == 0){
                    popup[i].x += popup[i].dx;
                }
            }
            if (popup[i].timer <= 0){
                popup.erase(popup.begin() + i);
            }
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



        //keypresses and what they do belows VVV
        if (ch == '\t'){
            nodelay(stdscr, FALSE);
            attron(COLOR_PAIR(1) | A_BOLD);
            mvprintw(Yborder / 2 - 1, Xborder / 2 - 3, "PAUSED");
            attroff(COLOR_PAIR(1) | A_BOLD);
            mvprintw(Yborder / 2 + 1, Xborder / 2 - 9, "TAB resume  Q quit");
            refresh();

            int pch = 0;
            while (pch != '\t' && pch != 'q'){
                pch = getch();
            }

            nodelay(stdscr, TRUE);
            if (pch == 'q'){
                r = false;
            }
            flushinp();
            ch = ERR;
        }
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
            int shot = (powerup == 1) ? 3 : 1;
            int mbul = (maxbullets + 1)* pwrbulmult[powerup];
            if (infbul == true || (int)bullets.size() + shot <= mbul){
                if (powerup == 1){
                    bullets.push_back({Xpl, Ypl - 1, -1});
                    bullets.push_back({Xpl, Ypl - 1, 0});
                    bullets.push_back({Xpl, Ypl - 1, 1});
                }
                else if (powerup == 5){ 
                    if (rand() % 2 == 0){
                        bullets.push_back({Xpl, Ypl - 1, rand() % 4});
                    }
                    else {
                        bullets.push_back({Xpl, Ypl - 1, -(rand() % 4)});
                    }
                }
                else {
                    bullets.push_back({Xpl, Ypl - 1, 0});
                }
                fireface = 10;
                beep();
                if (nocd == false){
                    cooldown = (powerup == 2) ? maxcd / 4 : maxcd;
                }
            }
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
                        case 5:
                            beep();
                            powerup = powerups[i].type; 
                            hurtface = 10;
                            poweruptime = 340;
                            break;
                        default:
                            beep();
                            powerup = powerups[i].type;
                            pface = 10;
                            poweruptime = 170 + powertbonus;
                            ppoint(powerup == 3 ? 200 : 100, powerups[i].x, powerups[i].y);
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
        if (btick > 3){
            btick = 0;
            for (int i = 0; i < bullets.size(); i++){
                bullets[i].y--;
                bullets[i].x += bullets[i].dx;
            }

            for (int i = bullets.size() - 1; i >= 0; i--){
                if (bullets[i].y < 0 || bullets[i].x < 0 || bullets[i].x >= Xborder){
                    bullets.erase(bullets.begin() + i);
                    continue;
                }

                for (int p = powerups.size() - 1; p >= 0; p--){
                    if (powerups[p].type == 4 && powerups[p].x == bullets[i].x && abs(powerups[p].y - bullets[i].y) <= 1){
                        boom.push_back({powerups[p].x, powerups[p].y, 16, 0});
                        ppoint(powerup == 3 ? 30 : 15, powerups[p].x, powerups[p].y);
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
                        mice[m].hp--;
                        if (mice[m].hp <= 0){
                            mice[m].alive = false;
                            beep();
                            ppoint(powerup == 3 ? 100 : 50, mice[m].x, mice[m].y);
                            if (rand() % 5 == 0){
                                powerups.push_back({mice[m].x + 1, mice[m].y + 1, 1 + (rand() % 3)});
                            }
                        }
                        else {
                            ppoint(powerup == 3 ? 50 : 25, mice[m].x, mice[m].y);
                        }
                        bullets.erase(bullets.begin() + i);
                        break;
                    }
                }
            }

            if (boss.alive == true && boss.hp > 0 && boss.y >= 3){
                for (int i = bullets.size() - 1; i >= 0; i--){
                    if (bullets[i].y >= boss.y && bullets[i].y <= boss.y + 2 && bullets[i].x >= boss.x && bullets[i].x <= boss.x + 8){
                        boss.hp--;
                        boss.hface = 8;
                        ppoint(powerup == 3 ? 50 : 25, bullets[i].x, bullets[i].y);
                        bullets.erase(bullets.begin() + i);
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
                        ppoint(powerup == 3 ? 10 : 5, powerups[i].x, powerups[i].y);
                    }
                    powerups.erase(powerups.begin() + i);
                }
            }

            if (abs(y - Ypl) <= 2 && Xpl + 3 >= x - 2 && Xpl - 3 <= x + 2){
                if (gmode == false){
                    lives--;
                    combo = 1;
                    s -= 250;
                    hurtface = 15;
                }
            }

            for (int i = 0; i < mice.size(); i++){
               if (mice[i].alive == false){
                    continue;
               } 
               if (abs(y - mice[i].y) <= 2 && mice[i].x + 2 >= x - 2 && mice[i].x <= x + 2){
                    mice[i].alive = false;
                    ppoint(powerup == 3 ? 100 : 50, mice[i].x, mice[i].y);
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
                    if (rand () % 2 == 0){
                        powerups.push_back({mice[alive[i]].x + 1, mice[alive[i]].y + 1, 4});
                    }
                    else {
                        powerups.push_back({mice[alive[i]].x + 1, mice[alive[i]].y + 1, 5});
                    }
                }
                if (rand() % bobmodd == 1 && mice[alive[i]].hp > 1){
                    if (rand () % 2 == 0){
                        powerups.push_back({mice[alive[i]].x + 1, mice[alive[i]].y + 1, 4});
                    }
                    else {
                        powerups.push_back({mice[alive[i]].x + 1, mice[alive[i]].y + 1, 5});
                    }
                }
            }
        }


        if (alive.empty() && boss.alive == false){ //if all dead make new wave basically
            wave++;
            Shop();
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

        if (s < 0){
            s = 0;
        }

        if (boss.alive == true){
            if (boss.hface > 0){
                boss.hface--;
            }

            if (boss.hp <= 0){
                if (boss.death == 0){
                    boss.death = 90;
                    ppoint(1000, boss.x + 4, boss.y);
                    if (lives < 9){
                        lives++;
                    }
                    pface = 40;
                }
                boss.death--;
                if (boss.death <= 0){
                    boss.alive = false;
                }
            }
            else if (boss.y < 5){
                if (stick % 12 == 0){
                    boss.y++;
                }
            }
            else {
                boss.mtick++;

                if (boss.mtick >= (boss.hp <= boss.maxhp / 2 ? 2 : 3)){
                    boss.mtick = 0; 
                    if (rand() % 14 != 0){
                        boss.x += boss.dir; 
                    }
                    else {
                        boss.x -= boss.dir; 
                    }
                    if (boss.x <= 1 || boss.x + 9 >= Xborder - 1){
                        boss.dir = -boss.dir;
                    }
                }

                boss.ftick--;

                if (boss.ftick <= 0){
                    if (rand() % 6 != 5){
                        if (boss.hp <= boss.maxhp / 2){
                            powerups.push_back({boss.x + 1, boss.y + 3, 4});
                            powerups.push_back({boss.x + 4, boss.y + 3, 4});
                            powerups.push_back({boss.x + 7, boss.y + 3, 4});
                            boss.ftick = 70;
                        }
                    }
                    else {
                        powerups.push_back({boss.x + 4, boss.y + 3, (rand() % 4 == 0) ? 5 : 4});
                        boss.ftick = 50;
                    }
                }
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
                case 5:
                    attron(COLOR_PAIR(2));
                    mvprintw(powerups[i].y, powerups[i].x, "#");
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
                if (mice[i].hp >=2 ){
                    attron(A_BOLD);
                    mvprintw(mice[i].y, mice[i].x, dir == 1 ? "~0>" : "<0~");
                    attroff(A_BOLD);   
                }
                else {
                    mvprintw(mice[i].y, mice[i].x, dir == 1 ? "~o>" : "<o~");
                }
            }
        }

        if (boss.alive == true){
            const char* bsprite[3] = { "|\\_____/|", "( 0   0 )", " \\__v__/ "};
            if (boss.hface > 0){
                if (rand() % 2 == 1){
                    bsprite[1] = "( x   X )";
                }
                else {
                    bsprite[1] = "( X   x )";
                }
            }

            int attr = COLOR_PAIR(1) | A_BOLD;
            if (boss.hp <= 0 && (stick / 6) % 2 == 0){
                attr = COLOR_PAIR(2) | A_BOLD;
            }
            attron(attr);
            for (int r = 0; r < 3; r++){
                mvprintw(boss.y + r, boss.x, "%s", bsprite[r]);
            }
            attroff(attr);

            if (boss.hp > 0){
                int bbar = boss.hp * 20 / boss.maxhp; 
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(3, Xborder / 2 - 14, "ALIEN [");
                for (int bar = 0; bar < 20; bar++){
                    addch(bar < bbar ? '#' : '-');
                }
                addch(']');
                attroff(COLOR_PAIR(2) | A_BOLD);
            }
        }

        std::string inf = "inf";

        attroff(COLOR_PAIR(3)); //HUD PRINT & CHEATS ON THE RIGHT

        if (cheatsactive > 0){
            int cheatsdisp[] = {0, 0, 0};
            for (int i = 0; i < cheatsactive; i++){
                if (cheatsdisp[0] == 0 && gmode == true){
                    mvprintw(i, Xborder - 8, "GODMODE");
                    cheatsdisp[0] = 1;
                }
                else if (cheatsdisp[1] == 0 && nocd == true){
                    mvprintw(i, Xborder - 12, "NO COOLDOWN");
                    cheatsdisp[1] = 1;
                }
                else if (cheatsdisp[2] == 0 && infbul == true){
                    mvprintw(i, Xborder - 12, "INF BULLETS");
                    cheatsdisp[2] = 1;
                }
            }
        }
        if (infbul == false){
            mvprintw(0, 0, "bullets=%d/%d cooldown=%.1f powertime=%d power=%s", (int)bullets.size(), (maxbullets + 1) * pwrbulmult[powerup], cooldown/31, poweruptime/31, powername[powerup]);
        }
        else {
            mvprintw(0, 0, "bullets=%d/%s cooldown=%.1f powertime=%d power=%s", (int)bullets.size(), inf.c_str(), cooldown/31, poweruptime/31, powername[powerup]);
        }
        mvprintw(1, 0, "lives=");
        attron(COLOR_PAIR(2));
        printw("%d", lives);
        attroff(COLOR_PAIR(2));
        printw(" SCORE %07d", s);
        attroff(COLOR_PAIR(2));
        mvprintw(2, 0, "wave = %02d", wave);

        if (cheatsactive == 0 && combo > 1){ //combo printing
            int comboatt[] = {0, 0, 3, 4, 1, 2};
            const char* combotitl[] = {"", "", "OK", "WOW", "PAWSOME", "MEOW"};
            int attr = COLOR_PAIR(comboatt[combo]) | A_BOLD;

            if (combo == 5 && (stick / 32) % 2 == 0){
                attr |= A_REVERSE;
            }
            int cbar = combot * 10 / 62;
            attron(attr);

            mvprintw(1, Xborder - 24, "%-7s x%d [", combotitl[combo], combo);
            for (int bar = 0; bar < 10; bar++){
                addch(bar < cbar ? '=' : ' ');
            }
            addch(']');
            attroff(attr);
        }

        if (cpop > 0){
            cpop--;
            if ((cpop / 16) % 2 == 0){
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(Yborder / 2 - 3, Xborder / 2 - 7, "MEOW ACHIEVED!");
                attroff(COLOR_PAIR(2) | A_BOLD);
            }
        }

        for (int i = 0; i < popup.size(); i ++){ //popup print 
            if (popup[i].y >= 3){
                attron(A_BOLD);
                mvprintw(popup[i].y, popup[i].x, "+%d", popup[i].value);
                attroff(A_BOLD);
            }
        }

        if (wstart > 0){ //The giant WAVE: # thing that appears
            wstart--;
            mvprintw(Yborder / 2, Xborder / 2 - 5, "WAVE %d", wave);
        }
        wnoutrefresh(stdscr);
        doupdate();

        napms(32);
    }

    int gotick = 0;
    bool goscreen = true;

    while (goscreen == true){ //Game over screen
        gotick++;

        int inp = getch();
        if (gotick > 100 && ( inp == ' ' || inp == 'q') ){
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

        if (evilcheat == true){
            attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(Yborder / 2 + 2, Xborder / 2 - 4, "CHEATER!");
            attroff(COLOR_PAIR(2) | A_BOLD);
        }

        if ((gotick / 16) % 2 == 0){
            mvprintw(Yborder / 2 + 4, Xborder / 2 - 9, "PRESS SPACE TO EXIT");
        }

        wnoutrefresh(stdscr);
        doupdate();
        napms(32);
    }

    endwin();
    return 0;
}