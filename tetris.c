#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
 /*
 게임판 배열의 정보를 확인하고 배열에 있는 정보를 gotoxy를 사용해서 원하는 위치에 출력
 */
static struct termios initial_settings, new_settings;
 
static int peek_character = -1;
 
void init_keyboard()
{
    tcgetattr(0,&initial_settings);
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
}
 
void close_keyboard()
{
    tcsetattr(0, TCSANOW, &initial_settings);
}
 
int _kbhit()
{
    unsigned char ch;
    int nread;
 
    if (peek_character != -1) return 1;
    new_settings.c_cc[VMIN]=0;
    tcsetattr(0, TCSANOW, &new_settings);
    nread = read(0,&ch,1);
    new_settings.c_cc[VMIN]=1;
    tcsetattr(0, TCSANOW, &new_settings);
    if(nread == 1)
    {
        peek_character = ch;
        return 1;
    }
    return 0;
}
 
int _getch()
{
    char ch;
 
    if(peek_character != -1)
    {
        ch = peek_character;
        peek_character = -1;
        return ch;
    }
    read(0,&ch,1);
    return ch;
}
 
int _putch(int c) {
    putchar(c);
    fflush(stdout);
    return c;
}
#define BW 10   //가로
#define BH 20   //세로
#define UP 119
#define DOWN 115
#define LEFT 97
#define RIGHT 100
#define SPACE 32
#define ROT 13
#define hide() printf("\e[?25l");

enum {Empty,Brick,Wall};    //빈공간 0, 벽돌1, 벽2
int gameBoard[BH+2][BW+2];  //게임내부 +2 해서 벽 생성
int brick_pos[7][4][8]={
    {{0,-1,0,0,0,1,1,-1},{0,0,-1,0,1,0,1,1},{0,-1,0,0,0,1,-1,1},{0,0,-1,0,-1,-1,1,0}},
    {{0,-1,0,0,0,1,0,2},{0,0,1,0,-1,0,-2,0},{0,-1,0,0,0,1,0,2},{0,0,1,0,-1,0,-2,0}},
    {{0,-1,0,0,0,1,1,0},{0,0,1,0,-1,0,0,1},{0,-1,0,0,0,1,-1,0},{0,0,0,-1,1,0,-1,0}},
    {{0,0,0,1,1,-1,1,0},{0,0,-1,0,0,1,1,1},{0,0,0,1,1,-1,1,0},{0,0,-1,0,0,1,1,1}},
    {{0,-1,0,0,1,0,1,1},{0,0,0,1,-1,1,1,0},{0,-1,0,0,1,0,1,1},{0,0,0,1,-1,1,1,0}},
    {{0,0,0,1,1,0,1,1},{0,0,0,1,1,0,1,1},{0,0,0,1,1,0,1,1},{0,0,0,1,1,0,1,1}},
    {{0,-1,0,0,0,1,1,1},{0,0,-1,0,-1,1,1,0},{0,0,0,-1,-1,-1,0,1},{0,0,1,0,-1,0,1,-1}}
};    //블럭들의 상대좌표정보  블럭종류/각 블럭의 회전모양/상대좌표값
int brick[1000][8]; //각 블럭들의 현재 좌표를 보관
int prebrick[1000][9]={0,};
void gotoxy(int x, int y);
void print_Board();
void initBoard();
int create_brick(int brick_shape,int brick_num);
int down_brick(int brick_num, int brick_shape);
int key(int brick_num,int *p_rot,int brick_shape);
int rot_check(int brick_num,int rot,int brick_shape);
int line_max();
void gameover();
void nextBrick(int brick_shape);
void brick_preview(int brick_num);


int main(){
    system("clear");
    hide()
    init_keyboard();
    srand(time(NULL));
    initBoard();
    int brick_num=0;
    int brick_shape_1=rand()%8;
    if(brick_shape_1==7) brick_shape_1=6;
    while(1){
        int brick_shape_2=rand()%8;
        if(brick_shape_2==7) brick_shape_2=6;
        print_Board();
        nextBrick(brick_shape_2);
        if(create_brick(brick_shape_1,brick_num)){
            gameover();
            break;
        }
        down_brick(brick_num,brick_shape_1);
        while(line_max());   //여러개의 줄이 한번에 완성될수도 있으니 완성된 모든 줄이 사라질때까지 반복
        brick_num++;
        brick_shape_1=brick_shape_2;  
    }

}


void gotoxy(int x, int y){
    printf("\033[%d;%df",y,x);
    fflush(stdout);
}
void initBoard(){   //좌표를 돌면서 게임판배열의 초기정보를 채움
    for(int y=0; y<BH+2; y++){
        for(int x=0; x<BW+2; x++){
            if(x==0 || x==BW+1 || y==BH+1){
                gameBoard[y][x]=Wall;
            }
            else{
                gameBoard[y][x]=Empty;
            }
        }
    }
}
void print_Board(){ //배열에 입력되어있는 정보를 화면에 출력
    for(int y=0; y<BH+2; y++){
        for(int x=0; x<BW+2; x++){
            if(gameBoard[y][x]==Wall){
                gotoxy(30+x*2,y);
                printf("⬜️");
            }
            else if(gameBoard[y][x]==Brick){
                gotoxy(30+x*2,y);
                printf("🟥");
            }
            else if(gameBoard[y][x]==Empty){
                gotoxy(30+x*2,y);
                printf("  ");
            }
        }
    }
}

int key(int brick_num,int *p_rot,int brick_shape){ //사용자의 키 입력을 받는 함수
    int nkey=_getch();
    int rot=*p_rot;
    switch(nkey){
        case UP:
        switch(*p_rot){
            case 0:
            if(rot_check(brick_num,rot,brick_shape)){
                return 0;
            }
            for(int i=0; i<8; i+=2){
                if(gameBoard[brick[brick_num][i]][brick[brick_num][i+1]]==Wall || gameBoard[brick[brick_num][i]][brick[brick_num][i+1]]==Brick){    // 벽에 닿거나 블럭에 닿으면 함수종료
                    return 0;
                }
            }
            for(int i=0; i<8; i+=2){    //회전하기 전 블럭이 있던 자리를 빈칸으로 바꿈
                gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
                printf("  ");
                gotoxy(100,100);
            }
            for(int i=0; i<8; i++){ //블럭의 좌표에서 돌리기 전의 상대좌표를 빼고 돌린 후의 좌표를 더해줌
                brick[brick_num][i]-=brick_pos[brick_shape][0][i];
                brick[brick_num][i]+=brick_pos[brick_shape][1][i];
            }
            for(int i=0; i<8; i+=2){    //바뀐 위치에 블럭을 출력
                gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
                printf("🟥");
            }
            *p_rot=1;   //현재 블럭의 회전상태값
            break;
            case 1:
            if(rot_check(brick_num,rot,brick_shape)){
                return 0;
            }
            for(int i=0; i<8; i+=2){
                gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
                printf("  ");
                gotoxy(100,100);
            }
            for(int i=0; i<8; i++){
                brick[brick_num][i]-=brick_pos[brick_shape][1][i];
                brick[brick_num][i]+=brick_pos[brick_shape][2][i];
            }
            for(int i=0; i<8; i+=2){
                gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
                printf("🟥");
            }
            *p_rot=2;
            break;
            case 2:
            if(rot_check(brick_num,rot,brick_shape)){
                return 0;
            }
            for(int i=0; i<8; i+=2){
                gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
                printf("  ");
                gotoxy(100,100);
            }
            for(int i=0; i<8; i++){
                brick[brick_num][i]-=brick_pos[brick_shape][2][i];
                brick[brick_num][i]+=brick_pos[brick_shape][3][i];
            }
            for(int i=0; i<8; i+=2){
                gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
                printf("🟥");
            }
            *p_rot=3;
            break;
            case 3:
            if(rot_check(brick_num,rot,brick_shape)){
                return 0;
            }
            for(int i=0; i<8; i+=2){
                gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
                printf("  ");
                gotoxy(100,100);
            }
            for(int i=0; i<8; i++){
                brick[brick_num][i]-=brick_pos[brick_shape][3][i];
                brick[brick_num][i]+=brick_pos[brick_shape][0][i];
            }
            for(int i=0; i<8; i+=2){
                gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
                printf("🟥");
            }
            *p_rot=0;
            break;
        }
        break;
        case DOWN:
        for(int i=0; i<8; i+=2){
            if(gameBoard[brick[brick_num][i]+1][brick[brick_num][i+1]]==Wall || gameBoard[brick[brick_num][i]+1][brick[brick_num][i+1]]==Brick){
                return 0;
            }
        }
        for(int i=0; i<8; i+=2){
            gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
            printf("  ");
            gotoxy(100,100);
        }
        for(int i=0; i<8; i+=2){
            brick[brick_num][i]+=1;
        }
        break;
        case LEFT:
        for(int i=0; i<8; i+=2){
            if(gameBoard[brick[brick_num][i]][brick[brick_num][i+1]-1]==Wall || gameBoard[brick[brick_num][i]][brick[brick_num][i+1]-1]==Brick){   
                return 0;
            }
        }
        for(int i=0; i<8; i+=2){
            gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
            printf("  ");
            gotoxy(100,100);
        }
        for(int i=0; i<8; i+=2){
            brick[brick_num][i+1]-=1;
        }
        break;
        case RIGHT:
        for(int i=0; i<8; i+=2){
            if(gameBoard[brick[brick_num][i]][brick[brick_num][i+1]+1]==Wall || gameBoard[brick[brick_num][i]+1][brick[brick_num][i+1]+1]==Brick){
                return 0;
            }
        }
        for(int i=0; i<8; i+=2){
            gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
            printf("  ");
            gotoxy(100,100);
        }
        for(int i=0; i<8; i+=2){
            brick[brick_num][i+1]+=1;
        }
        break;
        case SPACE:
        while(1){
            for(int i=0; i<8; i+=2){
                if(gameBoard[brick[brick_num][i]+1][brick[brick_num][i+1]]==Wall || gameBoard[brick[brick_num][i]+1][brick[brick_num][i+1]]==Brick){
                    return 0;
                }
            }
            for(int i=0; i<8; i+=2){    //블럭의 y좌표에 시간이 지날수록 좌표값을 1씩 더해줌
                brick[brick_num][i]+=1;
            }
        }
        break;
    }
    return 0;
}
int create_brick(int brick_shape,int brick_num){    //불럭의 상대좌표값을 현재 생성되는 블럭에 하나씩 넣음
    for(int i=0; i<8; i++){
        if(i%2==1){ //x좌표
            brick[brick_num][i]=brick_pos[brick_shape][0][i]+5;
        }
        else{   //y좌표
            brick[brick_num][i]=brick_pos[brick_shape][0][i];
        }
    }
    for(int i=0; i<8; i+=2){
        if(gameBoard[brick[brick_num][i]][brick[brick_num][i+1]]==Brick){
            return 1;
        }
    }
    return 0;
}
int down_brick(int brick_num,int brick_shape){
    int rot=0;
    while(1){
        if(_kbhit()){
            if(!key(brick_num,&rot,brick_shape)) continue;
        }
        for(int i=0; i<8; i+=2){
            if(gameBoard[brick[brick_num][i]+1][brick[brick_num][i+1]]==Wall || gameBoard[brick[brick_num][i]+1][brick[brick_num][i+1]]==Brick){
                for(int j=0; j<8; j+=2){
                    gameBoard[brick[brick_num][j]][brick[brick_num][j+1]]=Brick;
                }
                return 1;   //더 이상 이동할 수 없는 경우 게임판 배열에 brick 값을 넣음
            }
        }                                        
        brick_preview(brick_num);
        for(int i=0; i<8; i+=2){
            gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);                     
            printf("  ");
            gotoxy(100,100);
        }
        for(int i=0; i<8; i+=2){    //블럭의 y좌표에 시간이 지날수록 좌표값을 1씩 더해줌
            brick[brick_num][i]+=1;
        }
        for(int i=0; i<8; i+=2){
            gotoxy(30+2*brick[brick_num][i+1],brick[brick_num][i]);
            printf("🟥");
            gotoxy(100,100);
        }
        usleep(500000);
    }
    return 0;
}
int rot_check(int brick_num,int rot,int brick_shape){
    if(rot==3){
        for(int i=0; i<8; i++){ //블럭의 좌표에서 돌리기 전의 상대좌표를 빼고 돌린 후의 좌표를 더해줌
            brick[brick_num][i]-=brick_pos[brick_shape][3][i];
            brick[brick_num][i]+=brick_pos[brick_shape][0][i];
        }
        for(int i=0; i<8; i+=2){
            if(gameBoard[brick[brick_num][i]][brick[brick_num][i+1]]==Wall || gameBoard[brick[brick_num][i]][brick[brick_num][i+1]]==Brick){
                for(int j=0; j<8; j++){ 
                    brick[brick_num][j]-=brick_pos[brick_shape][0][j];
                    brick[brick_num][j]+=brick_pos[brick_shape][3][j];
                }
                return 1;
            }
        }
        for(int i=0; i<8; i++){ //블럭의 좌표에서 돌리기 전의 상대좌표를 빼고 돌린 후의 좌표를 더해줌
            brick[brick_num][i]-=brick_pos[brick_shape][0][i];
            brick[brick_num][i]+=brick_pos[brick_shape][3][i];
        }
        return 0;
    }
    else{
        for(int i=0; i<8; i++){ //블럭의 좌표에서 돌리기 전의 상대좌표를 빼고 돌린 후의 좌표를 더해줌
            brick[brick_num][i]-=brick_pos[brick_shape][rot][i];
            brick[brick_num][i]+=brick_pos[brick_shape][rot+1][i];
        }
        for(int i=0; i<8; i+=2){
            if(gameBoard[brick[brick_num][i]][brick[brick_num][i+1]]==Wall || gameBoard[brick[brick_num][i]][brick[brick_num][i+1]]==Brick){
                for(int j=0; j<8; j++){ //블럭의 좌표에서 돌리기 전의 상대좌표를 빼고 돌린 후의 좌표를 더해줌
                    brick[brick_num][j]-=brick_pos[brick_shape][rot+1][j];
                    brick[brick_num][j]+=brick_pos[brick_shape][rot][j];
                }
                return 1;
            }    
        }
        for(int i=0; i<8; i++){ //블럭의 좌표에서 돌리기 전의 상대좌표를 빼고 돌린 후의 좌표를 더해줌
            brick[brick_num][i]-=brick_pos[brick_shape][rot+1][i];
            brick[brick_num][i]+=brick_pos[brick_shape][rot][i];
        }
        return 0;
    }
}
int line_max(){     //게임판 전체를 돌면서 각 x축에 들어가있는 블럭의 수를 확인 
    for(int y=BH; y>0; y--){
        int num=0;  //x축에 있는 블럭의 개수
        for(int x=1; x<=BW; x++){
            if(gameBoard[y][x]==Brick){
                num++;
            }   
            if(num==10){    //블럭이 꽉차있을 경우
                for(int x2=1; x2<=BW; x2++){
                    gameBoard[y][x2]=Empty;     //꽉 차있던 줄은 빈공간으로
                }
                for(int y2=y-1; y2>0; y2--){
                    for(int x2=1; x2<=BW; x2++){
                        if(gameBoard[y2][x2]==Brick){       //나머지 블럭들이 있던 칸은 y좌표를 1씩 증가
                            gameBoard[y2][x2]=Empty;
                            gameBoard[y2+1][x2]=Brick;
                        }
                    }
                }
                return 1;
            }
        }
    }
    return 0;   //그 외에는 0반환
}
void gameover(){
    for(int y=1; y<=BH; y++){
        for(int x=1; x<=BW; x++){
            if(gameBoard[y][x]==Brick){
                gotoxy(30+2*x,y);
                printf("🔳");
                gameBoard[y][x]=Empty;
            }
            usleep(5000);
        }
    }
    sleep(1);
    print_Board();
    gotoxy(38,10);
    printf("Game Over");
    gotoxy(100,100);
}
void nextBrick(int brick_shape){
    for(int y=0; y<7; y++){
        for(int x=0; x<6; x++){
            if(y==1 || y==6 || x==5){
                gotoxy(54+2*x,y);
                printf("⬜️");
            }
            else{
                gotoxy(54+2*x,y);
                printf("  ");
            }
        }
    }
    if(brick_shape==5 || brick_shape==1){
        for(int i=0; i<8; i+=2){
            gotoxy(57+2*brick_pos[brick_shape][0][i+1],3.5+brick_pos[brick_shape][0][i]);
            printf("🟥");
        }
    }
    else{
        for(int i=0; i<8; i+=2){
            gotoxy(58+2*brick_pos[brick_shape][0][i+1],3+brick_pos[brick_shape][0][i]);
            printf("🟥");
        }
    }
}
void brick_preview(int brick_num){   //블럭이 어떻게 바닥에 닿을지 미리 보여주는 함수
    int check=1;
    if(prebrick[brick_num][8]==1){
        for(int i=0; i<8; i+=2){
            gotoxy(30+2*prebrick[brick_num][i+1],prebrick[brick_num][i]);
            printf("  ");
        }
    }
    for(int i=0; i<8; i++){
        prebrick[brick_num][i]=brick[brick_num][i];
    }
    while(check){
        for(int i=0; i<8; i+=2){
            prebrick[brick_num][i]++;   //미리보기 블럭 배열의 y좌표를 1씩 늘림
        }
        for(int i=0; i<8; i+=2){
            if(gameBoard[prebrick[brick_num][i]+1][prebrick[brick_num][i+1]]==Wall || gameBoard[prebrick[brick_num][i]+1][prebrick[brick_num][i+1]]==Brick){
                for(int j=0; j<8; j+=2){
                    gotoxy(30+2*prebrick[brick_num][j+1],prebrick[brick_num][j]);
                    printf("🔲");
                }
                prebrick[brick_num][8]=1;   //prebrick배열을 건드린적이 있는지 없는지 확인(블럭이 움직이거나 회전했을때 그전에 있던 미리보기를 지우기 위해서)
                check=0;    //미리보기 배열을 화면에 출력하고 while문 탈출
                break;
            }
        }
    }
}