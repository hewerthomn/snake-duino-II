/*
* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* <phk@FreeBSD.ORG> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in return Poul-Henning Kamp
* ----------------------------------------------------------------------------
*/

/**
* Snake Duino v2
*
* Nokia 5110 LCD attached to pins 7, 6, 5, 4, 3
* Active Buzzer attached to pin 8
* Accelerometer MMA7361 attached to pins 13, 12, 11, 10, A0, A1 and A2
*
*
* Libs
* 
* Adafruit GFX     https://github.com/adafruit/Adafruit-GFX-Library
* Adafruit PCD8544 https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
* mma7361-library  https://code.google.com/p/mma7361-library/
*
* Inspirated in Snake v1.0, Ouarrak Ayoub
* http://pastebin.com/iAVt9AGJ
*/

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <AcceleroMMA7361.h>

/* pins */
#define SPEAKER_PIN 8

/* constants */
#define UP    1
#define RIGHT 2
#define DOWN  3
#define LEFT  4

/* frame size */
#define MAX_WIDTH  84
#define MAX_HEIGHT 48

/* defaults */
#define SNAKE_LEN   10
#define SNAKE_SPEED 30


/* lcd display */
Adafruit_PCD8544 lcd = Adafruit_PCD8544(7, 6, 5, 4, 3);

/* snake lenght */
int snakeLen = SNAKE_LEN;
int point    = 0, points = 10;
int level    = 0, time   = SNAKE_SPEED;

int xSnake,    ySnake; /* snake face */
int xFood = 0, yFood  = 0;

/* directions */
int dr = 0, dc = 1, i;
boolean left = false, right = true, up = false, down = false;

// vetor containing the coordinates of the individual parts
// of the snake {cols[0], row[0]}, corresponding to the head
int snakeCols[260];

// Vector containing the coordinates of the individual parts 
// of the snake {cols [snake_lenght], row [snake_lenght]} correspond to the tail
int snakeRow[260];

// Accelerometer
AcceleroMMA7361 accelero;
int x, y, z; /* coords */
int acceleroSensi = HIGH; /* sensitivity */

void(* reset)(void) = 0;

/*
 * setup
 */
void setup()
{
  Serial.begin(9600);
  randomSeed(analogRead(1));
  
  lcd.begin();
  
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, LOW);
  
  accelero.begin(13, 12, 11, 10, A0, A1, A2);
  accelero.setARefVoltage(3.3);
  accelero.setSensitivity(acceleroSensi);
  
  xFood = lcd.width()  / 2;
  yFood = lcd.height() / 2;
  
  intro();
  
  calibrateAccelerometer();
}

/*
 *  loop
 */
void loop()
{
  coords();
  
  snake();
}

void coords()
{
  lcd.clearDisplay();  
  
  accelero.getAccelXYZ(&x, &y, &z);
  
  if(false) // for debug
  {
    lcd.setTextSize(2);
    lcd.print("X  ");
    lcd.println(x);
  
    lcd.print("Y  ");
    lcd.println(y);
  
    lcd.print("Z  ");
    lcd.println(z);

    lcd.display();
    delay(500);
  }
}

/*
 * snake
 */
void snake()
{
  xSnake = snakeCols[0];
  ySnake = snakeRow[0];
  
  if(point == 0 || point == points)
  {
    level++;
    printLevel();
  }  
  lcd.clearDisplay();
  
  moveSnake();
  
  // the snake has eaten the food (right or left)
  for(int i=0; i < 3; i++)
  {
    // control the snake's head (x) with x-coordinates of the food
    if((xSnake+1 == xFood) or (xSnake == xFood+1))
    {
      // control the snake's head (y) with y-coordinates of the food
      if((ySnake == yFood) or (ySnake+1 == yFood) or (ySnake == yFood+1))
      {
        eatFood();
      }
    }
    
    // the snake has eaten the food (from above or from bellow)
    if((ySnake == yFood) or (ySnake == yFood+i))
    {
      if((xSnake == xFood) or (xSnake+i == xFood) or (xSnake == xFood+i))
      {
        eatFood();
      }
    }    
  }
  
  /* LEFT */
  if(left == true)
  {
    // snake touches the left wall
    if(xSnake == 1) gameover();
    if(xSnake  > 1) drawSnake();
  }
  
  /* RIGHT */
  if(right == true)
  {
    // snake touches the top wall
    if(xSnake == MAX_WIDTH-1) gameover();
    if(xSnake  < MAX_WIDTH-1) drawSnake();
  }
  
  /* UP */
  if(up == true)
  {
    // snake touches the above wall
    if(ySnake == 1) gameover();
    if(ySnake  > 1) drawSnake();
  }
  
  /* DOWN */
  if(down == true)
  {
    // snake touches the ground
    if(ySnake == MAX_HEIGHT-1) gameover();
    if(ySnake  < MAX_HEIGHT-1) drawSnake();
  }
  
  delay(time);
}

/*
 * eatFood
 */
void eatFood()
{
  beep(2000, 10);
  
  // increase the point and snake lenght
  point++;
  snakeLen += 2;
  
  // new coordinates food randonly
  xFood = random(1, 80);
  yFood = random(1, 46);
  
  drawSnake();  
}

/*
 * drawSnake
 */
void drawSnake()
{
  lcd.drawRect(0, 0, MAX_WIDTH, MAX_HEIGHT, BLACK);
  
  for(int i = snakeLen; i > 0; i--)
  {
    lcd.drawCircle(snakeCols[i], snakeRow[i], 1, BLACK);
  }
  
  lcd.fillRect(xFood, yFood, 3, 3, BLACK);
  lcd.display();
  
  for(int i = snakeLen; i > 0; i--)
  {
    snakeRow[i]  = snakeRow[i - 1];
    snakeCols[i] = snakeCols[i - 1];
  }
  
  snakeRow[0]  += dr;
  snakeCols[0] += dc;
}

/*
 * moveSnake
 */
void moveSnake()
{
  int Xratio = 10;
  int Yratio = 15;
  
  /* LEFT */
  if(x < -(Xratio) and right == false)
  {
    if((xSnake > 0 or xSnake <= lcd.width() - xSnake))
      direc(LEFT);
    return;
  }
  
  /* RIGHT */
  if(x > Xratio and left == false)
  {
    if((xSnake > 0 or xSnake <= lcd.width() - xSnake))
      direc(RIGHT);
    return;
  }
  
  /* UP */
  if(y > Yratio and down == false)
  {
    if((ySnake > 0 or ySnake <= lcd.height() - ySnake))
      direc(UP);
    return;
  }
  
  /* DOWN */
  if(y < (Yratio * -1) and up == false)
  {
    if((ySnake > 0 or ySnake <= lcd.height() - ySnake));
      direc(DOWN);
    return;
  }
  
  if(z < -50)
  {
    showPause();
  }
}

/*
 * showPause
 */
void showPause()
{
  lcd.clearDisplay();
  lcd.setTextSize(2);
  lcd.setTextColor(BLACK);
  lcd.println("Pause!");
  lcd.println("");
  lcd.setTextSize(1);
  lcd.print("Level:  ");
  lcd.println(level);
  lcd.print("Points: ");
  lcd.println(point - 1);
  lcd.display();
  
  delay(5000);
}

/*
 * printLevel
 */
void printLevel()
{
  upLevel();
  
  point   = 1;
  points += 10;
}

/*
*  upLevel
*/
void upLevel()
{
  if(level > 1)
  {
    time -= 4;
  }
}

/*
 * direc
 */
void direc(int d)
{
  switch(d)
  {
    case UP:    { left=false; right=false; up=true ; down=false; dr = -1; dc =  0;} break;
    case RIGHT: { left=false; right=true ; up=false; down=false; dr =  0; dc =  1;} break;
    case DOWN:  { left=false; right=false; up=false; down=true ; dr =  1; dc =  0;} break;
    case LEFT:  { left=true ; right=false; up=false; down=false; dr =  0; dc = -1;} break;
  }
}

/*
 * gameover
 */
void gameover()
{
  beep(1000, 100);
  
  lcd.clearDisplay();
  lcd.setTextSize(2);
  lcd.println("Game");
  lcd.print("   Over");
  lcd.setTextSize(1);
  lcd.print("Level:  ");
  lcd.println(level);
  lcd.print("Points: ");
  lcd.println(point -1);
  lcd.display();
  delay(2500);
  
  lcd.clearDisplay();
  lcd.setTextSize(2);
  lcd.println("  Try");
  lcd.println(" Again!");
  lcd.display();  
  delay(1500);

  resetGame();  
}

/*
 * resetGame
 */
void resetGame()
{
 snakeLen = SNAKE_LEN;
 
 for(int i=0; i < (snakeLen-1); i++)
 {
   snakeCols[i] = i;
   snakeRow[i]  = (MAX_HEIGHT / 2);
 }
 
 level  = 0;  
 point  = 0;
 points = SNAKE_LEN;
 time   = SNAKE_SPEED;
 
 up    = false; 
 right = true;
 down  = false;
 left  = false;
 dr    = 0;
 dc    = 1;
}

/*
* intro
*/
void intro()
{
  lcd.clearDisplay();

  lcd.setTextColor(WHITE, BLACK);
  lcd.setTextSize(2);
  lcd.print(" Snake ");
  lcd.setTextColor(BLACK);
  lcd.print("Duino");
  lcd.setTextColor(WHITE, BLACK);
  lcd.print("II");
  lcd.setTextSize(1);
  lcd.setTextColor(BLACK);
  lcd.println("");
  lcd.print("by hewerthomn ");
  lcd.display();
  delay(2500);
}

void calibrateAccelerometer()
{
  double var = 5000;
  double sumX = 0;
  double sumY = 0;
  double sumZ = 0;
  
  lcd.clearDisplay();
  lcd.print("Calibrando");
  lcd.display();
  for (int i = 0;i<var;i++)
  {
    sumX = sumX + accelero.getXVolt();
    sumY = sumY + accelero.getYVolt();
    sumZ = sumZ + accelero.getZVolt();
    if (i%100 == 0)
    {
      lcd.print(".");
      lcd.display();
    }
  }
  
  if (acceleroSensi == false)
  {
    accelero.setOffSets(1672 - sumX / var,1671 - sumY / var, + 1876 - sumZ / var);
  }
  else
  {
    accelero.setOffSets(1650 - sumX / var,1650 - sumY / var, + 2450 - sumZ / var);
  }
  if (abs(accelero.getOrientation())!=3)
  {
    lcd.setTextColor(WHITE, BLACK);
    lcd.println("Falha!");
    accelero.setOffSets(0,0,0);
  }
  else
  {
    lcd.println("Pronto!");
  }
  lcd.display();
  
  delay(2000);  
  lcd.clearDisplay();
}

/*
 * beep
 */
void beep(int frequencyInHertz, long timeInMilliseconds)
{
  int x;
  long delayAmount = (long)(1000000/frequencyInHertz);
  long loopTime    = (long)((timeInMilliseconds*1000)/(delayAmount*2));
  for(x=0; x<loopTime; x++)
  {
    digitalWrite(SPEAKER_PIN, HIGH);
    delayMicroseconds(delayAmount);
    digitalWrite(SPEAKER_PIN, LOW);
    delayMicroseconds(delayAmount);
  }
  
  delay(2);
  // a little delay to make all notes sound separate
}
