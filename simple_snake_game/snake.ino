#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include <IRremote.h>

// ST7735
// define not needed for all pins; reference for ESP32 physical pins connections to VSPI:
// SDA  GPIO23 aka VSPI MOSI
// SCLK GPIO18 aka SCK aka VSPI SCK
// D/C  GPIO21 aka A0 (also I2C SDA)
// RST  GPIO22 aka RESET (also I2C SCL)
// CS   GPIO5  aka chip select
// LED  3.3V
// VCC  5V
// GND - GND

#define TFT_CS        10
#define TFT_RST        8
#define TFT_DC         9

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

uint16_t snake_image[12][12] = {
{0 ,19437 ,25872 ,25872 ,25872 ,25872 ,25872 ,25872 ,25872 ,19437 ,0 ,0 },
{0 ,19437 ,25872 ,25872 ,25872 ,25872 ,25872 ,25872 ,25872 ,19437 ,0 ,0 },
{0 ,19437 ,19437 ,19437 ,19437 ,19437 ,19437 ,19437 ,19437 ,19437 ,0 ,0 },
{0 ,19437 ,19437 ,19437 ,19437 ,19437 ,19437 ,19437 ,19437 ,19437 ,0 ,0 },
{0 ,19437 ,25872 ,25872 ,25872 ,25872 ,25872 ,25872 ,25873 ,19437 ,0 ,0 },
{0 ,19437 ,25872 ,25872 ,25872 ,25872 ,25872 ,25872 ,25872 ,19437 ,0 ,0 },
{0 ,19437 ,25872 ,63422 ,23792 ,25872 ,27921 ,65471 ,25872 ,19437 ,0 ,0 },
{0 ,19437 ,25872 ,25388 ,25872 ,25872 ,25840 ,27436 ,25872 ,19437 ,0 ,0 },
{0 ,19437 ,23695 ,21421 ,25873 ,25872 ,25840 ,21420 ,23695 ,19437 ,0 ,0 },
{0 ,21517 ,19437 ,25840 ,25840 ,25840 ,25840 ,25840 ,19437 ,21517 ,0 ,0 },
{0 ,0 ,21518 ,21518 ,17421 ,17421 ,17421 ,21518 ,21550 ,0 ,0 ,0 },
{0 ,0 ,0 ,0 ,41448 ,41448 ,41448 ,0 ,0 ,0 ,0 ,0 }
};

const int RECV_PIN = 7;
IRrecv irrecv(RECV_PIN);
decode_results results;

#define OFFSET_SCORE 18
#define MAX_X 126
#define MIN_X 2
#define MAX_Y 127
#define MIN_Y 19
#define BLOCK_SIZE 4

#define GRID_Y 27 //change based on 108/BLOCK_SIZE formula
#define GRID_X 31

enum dir {Right, Down, Left, Up};

#define START_X 62//change based on MIN_X + (GRID_X/2)*BLOCK_SIZE
#define START_Y 71 // MIN_Y + (GRID_Y/2)*BLOCK_SIZE formula
#define START_DIRECTION Right
#define START_TAIL_SIZE 3

bool GAME_RUNNING = false;
unsigned long key_value = 0;
uint8_t HighScore=0;
bool NewHighScore = false;

typedef struct _Point {
  uint8_t x,y;
} Point;
typedef struct _Snake {
  Point pos; //head of the snake
  uint8_t tail_length;
  Point* tail;
  uint8_t score;
  enum dir dir;
  enum dir last_dir;
  Point last_tail;
} Snake;

Snake SnakePlayer;
Point Apple;
float GameTime;
float DeltaTime;

void init_snake(Snake* snake)
{
   snake->pos.x = START_X;
   snake->pos.y = START_Y;
   snake->dir = START_DIRECTION;
   snake->last_dir = snake->dir;
   snake->tail_length = START_TAIL_SIZE;
   snake->tail = (Point *)realloc(snake->tail, snake->tail_length*sizeof(Point));
   snake->last_tail.x = -1;
   snake->last_tail.y = -1;

   for (uint8_t i=0;i<snake->tail_length;i++)
   {
      snake->tail[i].y = snake->pos.y;
      snake->tail[i].x = snake->pos.x-i-BLOCK_SIZE;
   }
   
   snake->score = 0;
}
void init_apple(Point* apple, Snake* snake)
{
  uint8_t rand_x;
  uint8_t rand_y;
  while(true) {
    rand_x = MIN_X + BLOCK_SIZE*random(0, GRID_X);

    if (snake->pos.x == rand_x) continue;
    for (uint8_t i=0;i<snake->tail_length;i++)
    {
      if (snake->tail[i].x==rand_x) continue;
    } 
    break;
  }

  while(true) {
    rand_y = MIN_Y + BLOCK_SIZE*random(0, GRID_Y);

    if (snake->pos.y == rand_y) continue;
    for (uint8_t i=0;i<snake->tail_length;i++)
    {
      if (snake->tail[i].y==rand_y) continue;
    } 
    break;
  }

  apple->x = rand_x;
  apple->y = rand_y;
}

void start_game()
{
  tft.fillScreen(ST77XX_BLACK);
  init_snake(&SnakePlayer);
  init_apple(&Apple, &SnakePlayer);
  draw_score();
  draw_game_bg(ST77XX_WHITE);
  draw_snake(&SnakePlayer);
  draw_apple(&Apple);
  GameTime=millis();
  DeltaTime = 0;
  GAME_RUNNING = true;
}

void draw_game_bg(uint16_t color)
{
  tft.drawLine(tft.width()-2, OFFSET_SCORE, 0, OFFSET_SCORE, color);
  tft.drawLine(tft.width()-2, tft.height()-1, 0, tft.height()-1, color);
  tft.drawLine(1, OFFSET_SCORE, 1, tft.height()-1, color);
  tft.drawLine(tft.width()-2, OFFSET_SCORE, tft.width()-2, tft.height()-1, color);
}

void draw_score()
{
  tft.setCursor(2,5);
  tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
  tft.print("Score: ");
  tft.print(SnakePlayer.score);
  tft.print("/");
  tft.print(HighScore);
}

void draw_snake(Snake* snake)
{
  for (int j=0;j<BLOCK_SIZE;j++)
      for (int k=0;k<BLOCK_SIZE;k++)
      {
        tft.drawPixel(snake->pos.x+j, snake->pos.y+k, ST77XX_GREEN);
        if (snake->last_tail.x != -1 && snake->last_tail.y != -1)
        {
              tft.drawPixel(snake->last_tail.x+j, snake->last_tail.y+k, ST77XX_BLACK);
        }
      }

  for (uint8_t i=0;i<snake->tail_length;i++)
  {
    for (int j=0;j<BLOCK_SIZE;j++)
      for (int k=0;k<BLOCK_SIZE;k++)
        tft.drawPixel(snake->tail[i].x+j, snake->tail[i].y+k, ST77XX_GREEN);
  }
}

void draw_apple(Point* apple)
{
  for (int j=0;j<BLOCK_SIZE;j++)
      for (int k=0;k<BLOCK_SIZE;k++)
        tft.drawPixel(apple->x+j, apple->y+k, ST77XX_RED);
}

void update_game(Snake* snake, Point* apple)
{
    if (snake->pos.x < MIN_X || (snake->pos.x+BLOCK_SIZE) > MAX_X) {
    stop_game();
    return;
   }
   if (snake->pos.y < MIN_Y || (snake->pos.y+BLOCK_SIZE) > MAX_Y) {
    stop_game();
    return;
   }
   for (uint8_t i=snake->tail_length;i>0;i--)
   {
      if (snake->tail[i].x == snake->pos.x && snake->tail[i].y == snake->pos.y)
      {
        stop_game();
        return;
      }
   }
   if (snake->pos.x == apple->x && snake->pos.y == apple->y)
   {
     init_apple(apple, snake);
     snake->last_tail.x = -1;
     snake->last_tail.y = -1;
     snake->tail_length++;
     snake->tail = (Point *)realloc(snake->tail, snake->tail_length*sizeof(Point));
     snake->score++;
     if (snake->score > HighScore)
     {
      HighScore = snake->score;
      NewHighScore=true;
     }
     draw_score();
   }
   else {
    snake->last_tail.x = snake->tail[snake->tail_length-1].x;
    snake->last_tail.y = snake->tail[snake->tail_length-1].y;
   }

   if (snake->dir == Right && snake->last_dir==Left) snake->dir = snake->last_dir;
   if (snake->dir == Left && snake->last_dir==Right) snake->dir = snake->last_dir;
   if (snake->dir == Up && snake->last_dir==Down) snake->dir = snake->last_dir;
   if (snake->dir == Down && snake->last_dir==Up) snake->dir = snake->last_dir;
   
   if (snake->dir == Right)
   {
      for (uint8_t i=snake->tail_length;i>0;i--)
      {
        snake->tail[i] = snake->tail[i-1];
      }
      snake->tail[0] = snake->pos;
      snake->pos.x += BLOCK_SIZE;
   }
   if (snake->dir == Left)
   {
    for (uint8_t i=snake->tail_length;i>0;i--)
      {
        snake->tail[i] = snake->tail[i-1];
      }
      snake->tail[0] = snake->pos;
      snake->pos.x -= BLOCK_SIZE;
   }
   if (snake->dir == Up)
   {
    for (uint8_t i=snake->tail_length;i>0;i--)
      {
        snake->tail[i] = snake->tail[i-1];
      }
      snake->tail[0] = snake->pos;
      snake->pos.y -= BLOCK_SIZE;
   }
   if (snake->dir == Down)
   {
    for (uint8_t i=snake->tail_length;i>0;i--)
      {
        snake->tail[i] = snake->tail[i-1];
      }
      snake->tail[0] = snake->pos;
      snake->pos.y += BLOCK_SIZE;
   }
   snake->last_dir = snake->dir;
}

void stop_game()
{
  draw_menu();
  GAME_RUNNING = false;
}

void setup(void) {
  Serial.begin(9600);
  irrecv.enableIRIn();
  irrecv.blink13(true);
  tft.initR(INITR_144GREENTAB); //  1.44" TFT Init ST7735R chip, green tab
  tft.fillScreen(ST77XX_BLACK);

  // draw menu
  stop_game();

  // a single pixel
  // tft.drawPixel(tft.width()/2, tft.height()/2, ST77XX_GREEN);
  // delay(500);

  // line draw test
  // testlines(ST77XX_YELLOW);
  // delay(500);

  // optimized lines
  // testfastlines(ST77XX_RED, ST77XX_BLUE);
  // delay(500);
}

void loop() {
  if (GAME_RUNNING==true)
  {
    if (irrecv.decode(&results)){
        if (results.value == 0XFFFFFFFF)
          results.value = key_value;

          switch(results.value){
          case 0xFF22DD:
            stop_game();
            irrecv.resume();
            return;
          break;
          case 0xFF18E7:
            SnakePlayer.dir = Up;
          break ;
          case 0xFF10EF:
            SnakePlayer.dir = Left;
          break ;
          case 0xFF4AB5:
          case 0xFF38C7:
            SnakePlayer.dir = Down;
          break ;
          case 0xFF5AA5:
            SnakePlayer.dir = Right;
          break ;
        }
        key_value = results.value;
        irrecv.resume(); 
    }
    DeltaTime += millis() - GameTime;
    GameTime = millis();
    if (DeltaTime > 50) {
      draw_snake(&SnakePlayer);
      draw_apple(&Apple);
      update_game(&SnakePlayer, &Apple);
      DeltaTime=0;
    }
  }
  else {
    if (irrecv.decode(&results)){
        if (results.value != 0XFFFFFFFF)
        {
          results.value = key_value;
          key_value = results.value;
          start_game();
          irrecv.resume(); 
        }
    } 
  }
}

void draw_menu()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(0);
  tft.setCursor(2,0);
  tft.print("- Simple Snake Game -");  
  uint8_t offset = 13;
  for (uint8_t i=0;i<96;i++)
    for (uint8_t j=0;j<96;j++)
      tft.drawPixel(j+16, i+offset, snake_image[i/8][j/8]);
  tft.setCursor(5,113);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Press ANY key");  
  tft.setCursor(5,113);
  if (NewHighScore==true)
  {    
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(10,35);
    tft.print("- NEW Highscore! -");  
    NewHighScore=false;
  }
}
