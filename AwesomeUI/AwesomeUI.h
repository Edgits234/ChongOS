#include <Arduino.h>
#define WOKWI_SIM false
#ifndef AwesomeUI_h
#define AwesomeUI_h

#ifndef UI_SETTINGS
  #define UI_SETTINGS 0b00000000
#endif
// At the top of AwesomeUI.h, add:
extern char *__brkval;
extern char __heap_start;

int freeMemory() {
  char top;
  return &top - (__brkval ? __brkval : &__heap_start);
}

int stackDepth = 0;
int maxStackDepth = 0;

// Pins for the DFR0665 Display
#define TFT_CS 10
#define TFT_DC 8
#define TFT_RST 9

// Pins for the XPT2046 Touch Controller (THIS IS FOR THE SCREEN IRL)(Verify these with DFRobot documentation)
#define T_CLK 52 // Shares SCK pin
#define T_DO 50  // Shares MISO pin
#define T_DIN 51 // Shares MOSI pin
#define T_CS 3   // This MUST be a unique digital pin (e.g., Pin 3)

// These are typical starting values for many 2.8"/3.2" screens with the XPT2046 chip
// Use these as placeholders until you run the calibration sketch on your hardware:
#define TS_MINX 200
#define TS_MAXX 3840
#define TS_MINY 400
#define TS_MAXY 3880
//#define MIN_PRESSURE 3

// #include <Adafruit_GFX.h>
// #include <Adafruit_ILI9341.h>
// #include <TFT_eSPI.h>
// #define DISABLE_WARNINGS
#include "CustomGraphics.h"

// #include <SD.h>

// struct Point
// {
//   int16_t x;
//   int16_t y;
// };

struct RectangleParams
{
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
  // int16_t lx;
  // int16_t ly;
  // uint16_t lw;
  // uint16_t lh;
};

// struct DoubleRectangleParams
// {
//   int16_t x;
//   int16_t y;
//   int16_t w;
//   int16_t h;
//   int16_t lx;
//   int16_t ly;
//   int16_t lw;
//   int16_t lh;
// };

struct RectangleParamsInt
{
  int x;
  int y;
  int w;
  int h;
};

struct DisplacePointUI
{
  int16_t x;
  int16_t y;
  // int16_t lx;
  // int16_t ly;
};

// struct Color
// {
//   byte red;
//   byte green;
//   byte blue;
// };

#if WOKWI_SIM
  #include <Wire.h> // Required for I2C communication
  #include <Adafruit_FT6206.h>
  Adafruit_FT6206 ctp = Adafruit_FT6206(); // Initialize capacitive touch object
  SPISettings spiSettings(80000000, MSBFIRST, SPI_MODE0);

  //make an empty skeleton of the KeyboardManager class so that nothing breaks
  class KeyboardManager
  {
    public:
    String buffer = "";
    uint8_t prevKeys[6] = {0,0,0,0,0,0};
    uint8_t lastKey = 0;
    uint8_t currKey = 0;
    char currChar = 0;
    size_t Available = false;
    HID_KEYBD_Info_TypeDef keys;
    long long lastPress = __LONG_LONG_MAX__;
    long long lastRepeat = 0;
    bool enableBuffer = false;

    void begin(){}
    size_t available(){return 0;}
    void testUpdate(){}
    void testKeys(HID_KEYBD_Info_TypeDef key){}
    bool checkForKey(uint8_t key){return false;}
    void update(){}
  };

  KeyboardManager kbd;

#else

  // #include <USBHostGiga.h>//this is for the keyboard
  // #include <XPT2046_Touchscreen.h>

  // Create touchscreen object
  // XPT2046 ALSO uses SPI1 (shares MOSI, MISO, SCK with display)
  // XPT2046_Touchscreen ts(T_CS);

  // The hero (correct): IN homage to the GOAT, I would have never figured this out by myself

  #include <CustomKeyboard.h>
#endif

// template <typename T> void print(T input){Serial.print(input);} template<typename T, typename... Args> void print(T input, Args... other){Serial.print(input); print(other...);}void println(){Serial.println();}template <typename T>void println(T input) {Serial.println(input);}template <typename T, typename... Args> void println(T input, Args... other){Serial.print(input);println(other...);}
// template <typename T> void print1(T input){Serial.print(input);} template<typename T, typename... Args> void print1(T input, Args... other){Serial.print(input); Serial.print(", "); print1(other...);}void println1(){Serial.println();}template <typename T>void println1(T input) {Serial.println(input);}template <typename T, typename... Args> void println1(T input, Args... other){Serial.print(input);Serial.print(", ");println1(other...);}

// #if enableTestPrints
// void testprintln(){Serial.println();}template <typename T>void testprintln(T input) {Serial.println(input);}template <typename T, typename... Args> void testprintln(T input, Args... other){Serial.print(input);println(other...);}
// #else
// void testprintln(){}template <typename T>void testprintln(T input) {}template <typename T, typename... Args> void testprintln(T input, Args... other){}
// #endif

// Point addPoints(Point p1, Point p2)
// {
//   return {p1.x + p2.x, p1.y + p2.y};
// }


template<typename T>
class Arraya
{
  public:
  T* array = nullptr;
  unsigned int size = 0;

  void add(T input)
  {
    // we allocate the size for the new array
    T* newArray = new T[size + 1];

    if(newArray == nullptr)
    {
      println("WELL WE'VE JUST FOUND THE ERROR, THIS STUPID ASS ARRAY TRIED GETTING MEMORY BUT IT FAILED AND NOW WE BOUT TO BUST THE WHOLE FUCKING THING");
      while(1);// bug finding
    }else
    {
      for (unsigned int i = 0; i < size; i++)
      {
        newArray[i] = array[i]; // we set the new array to the previous array
      }

      // we set the new value to add at the end
      newArray[size] = input;

      // we get rid of the previous array
      delete[] array;
      array = nullptr; // idk, just to be sure

      // set the array pointer to the new array pointer
      array = newArray;

      // we up the size by one
      size++;
    }
  }

  void remove()
  {
    // we allocate the size for the new array
    T* newArray = new T[size - 1];

    for (unsigned int i = 0; i < size - 1; i++)
    {
      newArray[i] = array[i]; // we set the new array to the previous array
    }

    // we get rid of the previous array
    delete[] array;
    array = nullptr; // idk, just to be sure

    // set the array to the new array
    array = newArray;

    // we up the size by one
    size--;
  }

  void remove(unsigned int index)
  {
    // we allocate the size for the new array
    T* newArray = new T[size - 1];

    for (unsigned int i = 0; i < index; i++)
    {
      newArray[i] = array[i]; // we set the new array to the previous array
    }

    for (unsigned int i = index; i < size - 1; i++)
    {
      newArray[i] = array[i + 1]; // we set the new array to the previous array
    }

    // we get rid of the previous array
    delete[] array;
    array = nullptr; // idk, just to be sure

    // set the array to the new array
    array = newArray;

    // we up the size by one
    size--;
  }

  T at(unsigned int index)
  {
    // if the index is in bound
    if (0 <= index && index <= size - 1)
    {
      return array[index];
    }
    else
    {
      println("ERROR, invalid index (the index you gave doesn't point to an element in the array)");
      while(1);// bug finding
      return 0;
    }
  }

  Arraya(){};
  Arraya(const Arraya&) = delete;
  Arraya& operator=(const Arraya&) = delete;

  ~Arraya()
  {
    delete[] array; // remove array from the memory
  }
};


int setPos(int pos, uint16_t len, int screenSize, bool vertical){return pos;}
int setPos(const char* pos, uint16_t len, int screenSize, bool vertical)
{
  // //make an array for the signs (+, -, *, /)
  // byte signs[9] = {0,0,0,0,0,0,0,0,0};
  // double numbers[10] = {0,0,0,0,0,0,0,0,0,0};//and another one for the numbers that we are going to have

  // //we start by separating the whole thing in an array and fuck, should I use a dynamic size array? Nah, we can use a fixed size array, its not like we are going to be doing some complex as balls thing
  // unsigned int index = 0;
  // while(true)
  // {
    

  //   //if we found the break char (the character that announces the end of the string)
  //   if(pos[findingSpaces] == '\0')
  //   {
  //     break;//we break to get out of the loop
  //   }

  //   index++;
  // }  
  if((strcmp(pos,"left") == 0 && vertical == 0) || ((strcmp(pos,"top") == 0 || strcmp(pos,"up") == 0 || strcmp(pos,"upper") == 0) && vertical == 1))
  {
    return 0;
  }else if(strcmp(pos,"middle") == 0 || strcmp(pos,"centered") == 0 || strcmp(pos,"center") == 0)
  {
    return screenSize / 2 - len / 2;
  }else if((strcmp(pos,"right") == 0 && vertical == 0) || ((strcmp(pos,"bottom") == 0 || strcmp(pos,"down") == 0 || strcmp(pos,"lower") == 0) && vertical == 1))
  {
    return screenSize - len;
  }

  println("ERROR, '",pos,"' is not correct keyword (tip: your make sure your 'top'/'bottom' and 'right'/'left' are not inverted)");
  return 0;
}

int16_t globalx = 0;
int16_t globaly = 0;
bool cursorClicky = false;
bool touched()
{
  #if WOKWI_SIM
    return ctp.touched();
  #else

    //if the keys got changed, we check if we pressed spacebar
    // if(kbd.available())
    // {

      //if we haven't pressed ctrl, alt and have not pressed a previous key
      if(kbd.keys.lctrl == 0 && kbd.keys.lalt == 0)
      {
        //if we just pressed the up arrow
        if(kbd.checkForKey(KEY_UPARROW))
        {
          globaly -= 5;
        }
        
        //if we just pressed the down arrow
        if(kbd.checkForKey(KEY_DOWNARROW))
        {
          globaly += 5;        
        }
        
        //if we just pressed the left arrow
        if(kbd.checkForKey(KEY_LEFTARROW))
        {
          globalx -= 5;
        }
        
        //if we just pressed the right arrow
        if(kbd.checkForKey(KEY_RIGHTARROW))
        {
          globalx += 5;          
        }
        
        if(kbd.checkForKey(KEY_SPACEBAR))
        {
          cursorClicky = true;
          return true;
        }else
        {
          cursorClicky = false;    
        }
      }
    // }

    return false;

  #endif
}

Point getPoint()
{
  #if WOKWI_SIM
    TS_Point p = ctp.getPoint(); // Get the touch point

    // fix the points, bcs they reverse for some reason
    p.x = SCREEN_WIDTH  - p.x;
    p.y = SCREEN_HEIGHT - p.y;

    return {p.x, p.y};
  #else

    return {globalx, globaly};
  #endif
}

int16_t gvx = 0;
int16_t gvy = 0;
int16_t gvw = SCREEN_WIDTH;
int16_t gvh = SCREEN_HEIGHT;
void setTestViewport(int16_t x, int16_t y, int16_t w, int16_t h)
{
  gvx = tft.vx;
  gvy = tft.vy;
  gvw = tft.vw;
  gvh = tft.vh;

  tft.setViewport(x, y, w, h);
}

void cancelTestViewport()
{
  tft.setViewport(gvx, gvy, gvw, gvh);
}


// input two rectangles and get if they touch or not
bool collide(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
  //returns false if any of the length of the rectangles are negative
  if(w1 < 0 || h1 < 0 || w2 < 0 || h2 < 0)
  {
    return false;
  }

  return !((x2 > x1 + w1 && x2 + w2 > x1 + w1) || (x2 < x1 && x2 + w2 < x1)) && !((y2 > y1 + h1 && y2 + h2 > y1 + h1) || (y2 < y1 && y2 + h2 < y1)); // && !((y2 > y1 + h1 && y2 + h2 > y1 + h1) || (y2 > y1 && y2 + h2 > y1));
}

// uint16_t color(byte red, byte green, byte blue)
// {
//   // maping the colors from 255 to  their respective map
//   red = red * 32 / 256;
//   green = green * 64 / 256;
//   blue = blue * 32 / 256;
//
//   return ((uint16_t)(red) << 11) + ((uint16_t)(green) << 5) + (uint16_t)(blue);
// }
//
// uint16_t color(Color input)
// {
//   // maping the colors from 255 to  their respective map
//   // println(input.red," * ",31," = ",input.red * 32);
//   // println(input.red * 31," / 255 = ",input.red * 32 / 255);
//   input.red = input.red * 32 / 256;
//   input.green = input.green * 64 / 256;
//   input.blue = input.blue * 32 / 256;
//
//   // println();
//   // println("Red:",input.red);
//   // Serial.println(input.red,BIN);
//   // println("Green:",input.green);
//   // Serial.println(input.green,BIN);
//   // println("Blue:",input.blue);
//   // Serial.println(input.blue,BIN);
//
//   return ((uint16_t)(input.red) << 11) + ((uint16_t)(input.green) << 5) + (uint16_t)(input.blue);
// }
//
// Color toColor(uint16_t input)
// {
//   Color variable;
//
//   variable.red   = ((input & 0xF800) >> 11) * 256 / 32;
//   variable.green = ((input & 0x7E0)  >>  5) * 256 / 62;
//   variable.blue  = ((input & 0x1F)   >>  0) * 256 / 32;
//
//   return variable;
// }

//takes in pairs of 2 rectangles. VERY CURSED gonna regret that but meh whatever
RectangleParams combineRectangles16(RectangleParams r1, RectangleParams r2)
{
  int16_t x1 = max(r1.x, r2.x);
  int16_t y1 = max(r1.y, r2.y);
  int16_t x2 = min(r1.x + r1.w, r2.x + r2.w);
  int16_t y2 = min(r1.y + r1.h, r2.y + r2.h);
  
  int16_t  x = x1;
  int16_t  y = y1;
  int16_t w = max(0, x2 - x1);
  int16_t h = max(0, y2 - y1);

  return {x, y, w, h};

  //test this function with teh following : 
  // tft.drawRect(20, 20, 100, 50, color(0,255,0));
  // tft.drawRect(30, 200, 50, 60, color(0,0,255));
  // RectangleParams r = combineRectangles({30, 200, 50, 60}, {20, 20, 100, 50});
  // tft.drawRect(r.x, r.y, r.w, r.h, color(255,255,255));
}

//we might not need this function anymroe ------------------------------------------------------------------------------------------------------------------------------------------------------------------
// RectangleParams combine2Rectangles16(RectangleParams r1, RectangleParams r2)
// {
//   RectangleParams r3 = combineRectangles16({r1.x, r1.y, r1.w, r1.h}, {r2.x, r2.y, r2.w, r2.h});
//   // RectangleParams r4 = combineRectangles16({r1.lx, r1.ly, r1.lw, r1.lh}, {r2.lx, r2.ly, r2.lw, r2.lh});

//   return {r3.x, r3.y, r3.w, r3.h};
// }

// RectangleParams combineRectangles(RectangleParamsInt r1, RectangleParams r2)
// {
//   return combineRectangles16((RectangleParams){(int16_t)r1.x, (int16_t)r1.y, (uint16_t)r1.w, (uint16_t)r1.h}, (RectangleParams){(int16_t)r2.x, (int16_t)r2.y, (uint16_t)r2.w, (uint16_t)r2.h});// typecast that BITCH
// }

//global::UIelement
class UIelement
{
  public:
  const char* id = nullptr;
  int16_t x = 0;
  int16_t y = 0;
  int16_t w = 0;
  int16_t h = 0;
  int8_t z_index = 0;
  uint8_t selected = false;
  uint8_t selectable = true;

  /*0b00000000
    ##^^^^^^
    ##87654321

  1 -> show selected outline
  2 -> ?
  3 -> ?
  4 -> ?
  5 -> ?
  6 -> ?
  7 -> ?
  8 -> ?*/
  uint8_t settings = 0b00000001;

  virtual void update(){}
  virtual void handleInput(Point p, bool holding, bool lostfocus){}

  #if !WOKWI_SIM
  virtual void handleKeyboardInput(){}
  #endif

  virtual void tick(){}
  virtual void draw(){}
  virtual ~UIelement(){}
};

// forward definition of the Text, button, and terminal class
class Text;
class Button;
class Window;
class Terminal;
class UI;

// In AwesomeUI.h, add this BEFORE the UI class definition (around line 300):

UI* g_pathBuffer[10];

//global::UI 
//class to manage the UI array, (updating ui, input handling for ui, acts as the middle man between the ui elements and the inputs of the user)
uint8_t enabletouch = 1;
class UI 
{
  public:
  Array<UIelement*> uiPointerArray; // we are simply creating an array of pointers to UIelement object with a custom class (this class does some very basic memory managements for you and is kind of like vector exept all of the features are missing exept "push_back" which is now "add" and "remove" just removes at last index or the specified index)
  unsigned long lastTouch = 0;
  bool holding = false;
  bool focus = false;
  uint16_t basecolor = 0;
  UI* prevUI = nullptr;
  bool isManager = false;
  uint8_t selected = false;
  UIelement* parent = nullptr;
  uint8_t kbdShortcuts = 0;
  UIelement* selectedElement = nullptr;

  #if WOKWI_SIM
    int pointRadius = 7;
  #else
    int pointRadius = 5;
  #endif

  //UI::printarray
  void printarray()
  {
    println(" this function should not be playing (line ",__LINE__,")");
    // test print println("printing array");
    // test print println("the size is : ",uiPointerArray.size);

    // test print
    //println("(size : ",uiPointerArray.size,")");
    for(unsigned int i = 0; i < uiPointerArray.size; i++)
    {
      println("[",i,"] = ",uiPointerArray.at(i)->id);
    }
  }

  //is that too many variables? meh probably not thats like uhhhhhhhhhh 128 bytes.. dayum 128 bytes, ok thats maybe a bit much... we have 8 MB right...
  int16_t x = 0;
  int16_t y = 0;
  int16_t w = 240;
  int16_t h = 320;

  //check if the element exists based on its pointer
  bool exists(UIelement* element)
  {
    for(unsigned int i = 0; i < uiPointerArray.size; i++)
    {
      if(element == uiPointerArray.at(i))
      {
        return true;
      }
    }

    return false;
  }

  //check if an element exists based on its id
  bool exists(const char* id)
  {
    for(unsigned int i = 0; i < uiPointerArray.size; i++)
    {
      UIelement* elem = uiPointerArray.at(i);
      
      //we check if elem is a nullptr JUST IN CASE
      if(elem == nullptr)
      {
        println("ERROR line ",__LINE__,", detected a nullptr element");
        continue;//continue just to be sure we don't use the elem
      }else
      {
        //if the strings are the same
        if(strcmp(id, elem->id) == 0)
        {
          return true;
        }
      }
    }
  }

  //UIManager::touchCollide
  bool touchCollide(Point p, int16_t x, int16_t y, int16_t w, int16_t h)
  {
    return collide(x, y, w, h, p.x - pointRadius, p.y - pointRadius, pointRadius*2, pointRadius*2);
  }

  //UI::findElementWithId
  UIelement* findElementWithId(const char* id)
  {
    // println("this function should not be called (line ",__LINE__,")");
    // println("in file '",__FILE__,"', in function '",__func__,", at line '",__LINE__,"'");
    for(unsigned int i = 0; i < uiPointerArray.size; i++)
    {
      if(strcmp(uiPointerArray.at(i)->id, id) == 0)
      {
        return uiPointerArray.at(i);
      }
    }

    println("ERROR, COULD NOT FIND THE UI ELEMENT WITH ID:'",id,"' in the following list of UIs : ");
    println("Array size (",uiPointerArray.size,")");
    printarray();
    println("");
    return nullptr;
  }

  //UI::getTotalDisplacement
  DisplacePointUI getTotalDisplacement()
  {
    UI* currentUI = this;
    DisplacePointUI t = {0,0};
    int i = 0; 
    while(true)
    {
      t.x += currentUI->x;
      t.y += currentUI->y;

      //if we itreated for too long (stuck in some kind of loop) we just print some error stuff and get out of the loop
      if(i > 30)
      {
        println("ERROR, went down too many indexs (you might have too many elements in elements);");
        break;
      }

      //if the previous ui is a nullptr then we break;
      if(currentUI->prevUI == nullptr)
      {
        break;
      }

      currentUI = currentUI->prevUI;

      i++;
    }

    return t;
  }

  //UI::getCombinedRectangle
  RectangleParams getCombinedRectangle()
  {
      int pathLen = 0;
      
      UI* temp = this;
      while(temp != nullptr && pathLen < 10)
      {
          // CHECK FOR LOOPS!
          for(int i = 0; i < pathLen; i++) {
              if (g_pathBuffer[i] == temp) {
                  Serial.println(F("!!! CIRCULAR REFERENCE DETECTED !!!"));
                  Serial.print(F("Loop at depth: ")); Serial.println(pathLen);
                  while(1);
              }
          }
          
          g_pathBuffer[pathLen++] = temp;
          temp = temp->prevUI;
      }
      
      if (pathLen >= 10) {
          Serial.println(F("!!! PATH TOO DEEP !!!"));
          while(1);
      }
      
      // Now calculate absolute positions with ONE pass
      int16_t absX = 0, absY = 0;
      
      for(int i = pathLen - 1; i >= 0; i--)  // Root to leaf
      {
          absX += g_pathBuffer[i]->x;
          absY += g_pathBuffer[i]->y;
      }
      
      // Start with this UI's rectangle
      RectangleParams r = {absX, absY, w, h};
      
      // Intersect with each parent
      int16_t cumX = 0, cumY = 0;
      for(int i = pathLen - 1; i > 0; i--)  // Don't include 'this' (index 0)
      {
          cumX += g_pathBuffer[i]->x;
          cumY += g_pathBuffer[i]->y;
          
          r = combineRectangles16(r, {cumX, cumY, g_pathBuffer[i]->w, g_pathBuffer[i]->h});
      }
      
      return r;
  }

  template <typename T1, typename T2>
  Text& addText(const char* id_input, String text, int fontsize, uint16_t color, T1 posX, T2 posY);//UI::addText 
  Text& getText(const char* id);//UI::getText

  /**
   * @brief adds a button
   * @param id_input the id of the ui element (cannot be changed once set)
   * @param text the text that the button is displaying
   * @param fontsize the size of the text
   * @param colour the colour of the text (use "color()" function to input color with rgb (0 to 255 for each color))
   * @param textOffsetX the position in x axis of the text, can be inputed as number (Ex:1, 2, 10, 20, etc...) or as string (Ex:"middle", "left", "right", etc...)
   * @param textOffsetY the position in y axis of the text, can be inputed as number (Ex:1, 2, 10, 10, etc...) or as string (Ex:"middle", "top", "down", etc...)
   * @param background is there a background? (in the case that there is no backgorund, the position of the text will take over and won't simply be the offset position based on the background position)
   * @param backgroundColor the color of the background (if there is no background, the color you input doesn't matter)
   * @param posX the position in the x axis of the background
   * @param posY the position in the y axis of the background
   * @param width the width of the background
   * @param height the height of the background
   * @param function1 a function that plays whenever the button changes states
   * @param toggle the state of the button. Is it a toggle or a simple, push down to activate, stop pushing to deactivate
   * @retval returns a button reference, (you can globaly initialize a button and set its variable to the button)
   */
  template <typename T1, typename T2, typename T3, typename T4>
  Button& addButton(const char* id_input, String text, int fontsize, uint16_t colour, T3 textOffsetX, T4 textOffsetY, bool background, uint16_t backgroundcolor, T1 posX, T2 posY, uint16_t width, uint16_t height, void (*function1)(bool input), bool toggle);//UI::addButton
  Button& getButton(const char* id);//UI::getButton

  template <typename T1, typename T2>
  Window& addWindow(const char* id_input, uint16_t colour, T1 xpos, T2 ypos, uint16_t width, uint16_t height);//UI::addWindow
  Window& getWindow(const char* id);//UI::getWindow

  template <typename T1, typename T2>
  Terminal& addTerminal(const char* id_input, T1 window_x, T2 window_y, int16_t window_w, int16_t window_h, uint16_t backgroundColor, uint16_t textColor);//UI::addTerminal
  Terminal& getTerminal(const char* id);//UI::getTerminal

  //UI::remove
  void remove(const char* id)
  {
    println("SHOULD NOT BE REMOVING ANY UI ELEMENT WHAT THE FUCK");
    // we go through all of the ui elements
    for (unsigned int i = 0; i < uiPointerArray.size; i++)
    {
      // if the ui element has the specific id we are looking for
      if (strcmp(uiPointerArray.at(i)->id, id) == 0)
      {
        delete uiPointerArray.at(i);
        uiPointerArray.remove(i); // don't forget to remove it from the array too
        
        return;
      }
    }
    
    // if we didn't find a corresponding id
    println("ERROR, couldn't find '", id, "' in the list of ui elements");
  }
  
  //UI::remove
  void remove(UIelement* ptr)
  {
    // we go through all of the ui elements
    for (unsigned int i = 0; i < uiPointerArray.size; i++)
    {
      //if the ui element has the specific pointer we are looking for
      if(uiPointerArray.at(i) == ptr)
      {
        delete uiPointerArray.at(i);
        uiPointerArray.remove(i); // don't forget to remove it from the array too
      }
    }
    
    // // if we didn't find a corresponding id
    println("ERROR line ",__LINE__," couldn't find the pointer in the list of ui elements");
  }

  //UI::select
  /**
   @param p the point where a touch even happent
   @brief this function gets the UIelement that is touching the point provided
   @retval returns the pointer to that UIelement
  */
  UIelement* select(Point p)
  {
    //if the point that has been pressed is not inside of the current window, then we know nothing is going to be selected
    {
      RectangleParams r = getCombinedRectangle();
      if(!touchCollide(p, r.x, r.y, r.w, r.h)) return nullptr;
    }

    //globaly set the total displacement (we know its never going to change)
    DisplacePointUI d = getTotalDisplacement();

    UIelement* bm = nullptr;

    for(int j = INT8_MIN; j <= INT8_MAX; j++)
    {
      //bm -> bestMatch (its the element that is colliding with the point that has the highest z)
      
      for(int i = uiPointerArray.size - 1; i >= 0 ; i--)
      {
        UIelement* elem = uiPointerArray.at(i);

        if(elem != nullptr)
        {
          if(elem->z_index == j)
          {
            if(bm == nullptr)
            {
              //bm isn't set to anything yet, so the current element becomes the best element IF ITS BEING TOUCHED

              //if the pixel is not inside of the window
              bool collided = touchCollide(p, d.x + elem->x, d.y + elem->y, elem->w, elem->h);

              //if it is being touched
              if(collided)
              {
                //the current element becomes the best element
                bm = elem;
              }
            }else
            {

              //if the pixel is not inside of the window
              bool collided = touchCollide(p, d.x + elem->x, d.y + elem->y, elem->w, elem->h);

              //if the z_index of the current element is bigger than the best element
              if(collided && (bm->z_index < elem->z_index))
              {
                //if it is, the current element becomes the best element
                bm = elem;
              }
            }
          }
        }else
        {
          println("ERROR line ",__LINE__,", detected a nullptr in the uiPointerArray at index ",i);
        }
      }
    }

    //return best match
    return bm;
  }

  //UI::handleInput
  void handleInput(Point p, bool holding, bool focus)
  {
    //has a new touch started
    if(holding == 0 && focus == 1)
    {
      println("started a new touch");

      if(selectedElement != nullptr)
      {
        println("making an element DESELECTED");

        //deselect the previously selected element
        if(selectedElement->selectable == 1)
        {
          selectedElement->selected = 0;
        }
      }

      //update the selected element
      selectedElement = select(p);

      if(selectedElement != nullptr)
      {
        //set selected to 2 because selectedElement not being nullptr means that the ui class has an element that is selected within it 
        if(parent != nullptr)
        {
          parent->selected = 2;
        }else if(isManager == 1)
        {
          selected = 2;
        }

        println("making an element selected");

        //make the new selected element selected
        if(selectedElement->selectable == 1)
        {
          selectedElement->selected = 1;
        }

      //else if its the main manager then we should be selecting it when we can't select anything else
      }else if(isManager == 1)
      {
        selected = 1;
      }

    //if we stopped holding the element
    }//else if(holding == 0 && focus == 0)
    // {
    //   //deselect whatever element we were holding
    //   if(selectedElement != nullptr)
    //   {
    //     //deselect the previously selected element
    //     selectedElement->selected = 0;
    //   }
    // }

    //go through all of the uiPointerArray and handleInput for them
    for(unsigned int i = 0; i < uiPointerArray.size; i++)
    {
      if(uiPointerArray.at(i) == nullptr)
      {
        println("UH OH, NOT GOOD, WE HAVE A NULLPTR!!!---------------------------------------------notice");
      }else
      {
        uiPointerArray.at(i)->handleInput(p, holding, focus);// play update functioon for all YOU NEED TO ADD THE Z_INDEX THINGY DON'T FORGET ABOUT IT PLEASE--------------------------------------
      }
    }
  }
  
  //UI::handleKeyboardInput
  void handleKeyboardInput()
  {

    // println("WHAT THE FUCK, IT IS NOT SUPPOSED TO DO THIS FUNCTION ---------------------------------------notice");
    for(unsigned int i = 0; i < uiPointerArray.size; i++)
    {
      if(uiPointerArray.at(i) == nullptr)
      {
        println("UH OH, NOT GOOD, WE HAVE A NULLPTR!!!---------------------------------------------notice");
      }else
      {
        uiPointerArray.at(i)->handleKeyboardInput();// play update functioon for all YOU NEED TO ADD THE Z_INDEX THINGY DON'T FORGET ABOUT IT PLEASE--------------------------------------
      }
    }
  }

  //UI::addUIpointer
  void addUIpointer(UIelement *pointer)
  {
    uiPointerArray.add(pointer); // add the pointer at the end of the array
  }
  
  //UI::removeUIpointer
  void removeUIpointer(UIelement *pointer)
  {
    println("this function should not playing right now (line 991)");
    // we go through all of the UIelement pointers in the array
    for(unsigned int i = 0; i < uiPointerArray.size; i++)
    {
      // we find the corresponding pointer (the same pointer as in the function input)
      if(uiPointerArray.at(i) == pointer)
      {
        uiPointerArray.remove(i); // we remove the pointer from the list
      }
    }
  }
  
  //UI::tick
  void tick()
  {
    println("this function should not be called (line : 1009)");
    // maybe change the order idk
    for (unsigned int i = 0; i < uiPointerArray.size; i++)
    {
      uiPointerArray.at(i)->tick();
    }
  }

  //UI::clear
  void clear()
  {
    unsigned int staticSize = uiPointerArray.size;

    // we go through all of the ui elements
    for (unsigned int i = 0; i < staticSize; i++)
    {
      // then we delete
      delete uiPointerArray.at(0);
      uiPointerArray.remove(0); // don't forget to remove from the array too

      return;
    }
  }

  //UI::touchUpdateAddition
  virtual void touchUpdateAddition(){}

  //UI::update
  // FIX 4: Add bounds checking to update() loop
  void update()
  {
    //testprintln("touchUpdateAddition");
    
    touchUpdateAddition();

    //testprintln("handling input based on recieved point, now updating elements");

    //go through all of the possible z_indexs
    for(int j = INT8_MIN; j <= INT8_MAX; j++)
    {
      //go through all ofthe list of UIelements
      for (unsigned int i = 0; i < uiPointerArray.size; i++)
      {
        UIelement* ptr = uiPointerArray.at(i);//get ui element

        //if its a nullptr, print error
        if(ptr == nullptr)
        {
          println("ERROR, nullptr detected in uiPtrArray (go to line ",__LINE__," to see where this print statement was put)");
          continue;
        }

        //if its not then update it and draw bound box if its selected
        if(ptr->z_index == j)
        {
          ptr->update();

          //is the ui selected AND selectable in the first place
          if(ptr->selected == 1 && ptr->settings & 0b1)
          {
            //get total displacement because positions are local (based on the parent's positiond)
            DisplacePointUI d = getTotalDisplacement();

            // println("drawing selected bounding box");
            tft.drawRect(ptr->x + d.x - 5, ptr->y + d.y - 5, ptr->w + 10, ptr->h + 10, 0xFFFF);//draw bounding box if its selected
          }
        }
      }
    }  
  
    //testprintln("finnished handling elements, displaying the frame buffer");

    //show cursor on GIGA (because we are using the keyboard)
    #if !WOKWI_SIM
      if(isManager && enabletouch)
      {
        uint16_t cursorColor = color(0, 0, 255);

        if(cursorClicky)
        {
          cursorColor = color(255, 0, 0);
        }

        //upper part of the cursor
        tft.fillRect(globalx, globaly - 6, 1, 5, cursorColor);

        //lower part of the cursor
        tft.fillRect(globalx, globaly + 1, 1, 5, cursorColor);

        //lefter part of the cursor
        tft.fillRect(globalx - 6, globaly, 5, 1, cursorColor);

        //righter part of the cursor
        tft.fillRect(globalx + 1, globaly, 5, 1, cursorColor);
      }
    #endif


    if(isManager && !(UI_SETTINGS & 0b100))
    {
      //testprintln("displaying the frame buffer");
      displayFrameBuffer();//display the buffer on the screen
    }
  }

};

//global::UIManager
class UIManager : public UI
{
  public:
  void (*touchInput)(Point p, bool holding, bool lostfocus);
  int number = 0;//what the fuck is that ??? ----------------------------------------------------------------------
  // bool selected = false;

  //UIManager::begin
  void begin()
  {
    Serial.begin(9600);
    delay(3000);
    Serial.println("Starting...");
    Serial.println("DO NOT FUCKING FORGET ABOUT SAVING CHANGES TO THE ONLINE VERSION OF WOKWI !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    tft.begin();
    kbd.begin();

    tft.fillScreen(basecolor);

    #if WOKWI_SIM
      if (!ctp.begin(40))
      { // Initialize the capacitive touch screen
        Serial.println("Couldn't start FT6206 touchscreen controller");
        while (1);
      }
      Serial.println("Capacitive touchscreen started");
    #else
      // Initialize your resistive touchscreen here
      
      // Start SPI1 (shared with display)
      // SPI1.begin();

      // if (!ts.begin(SPI1)) {
      //   Serial.println("ERROR: Touch controller not found!");
      //   Serial.println("Check your T_CS wire (should be pin 3)");
      //   while(1);
      // }

      // DON'T initialize XPT2046 library - we're doing it manually
      // pinMode(T_CS, OUTPUT);
      // digitalWrite(T_CS, HIGH);
    #endif
  }

  //UIManager::touchUpdateAddition
  //add the touch updating to the UIManager UI only because we don't want to update the whole touch thingy when we simply want to update UIs
  void touchUpdateAddition()
  {

    //first bit fo UI_SETTINGS is supposed to be "are we not clearing the screen?"
    if(!(UI_SETTINGS & 1))
    {
      tft.fillScreen(basecolor);
    }

    //update the keyboard (fetch the keys that have been pressed or unpressed)
    kbd.update();

    if(enabletouch == 1)
    {
      if(touched())
      {
        Point p = getPoint();

        //testprintln("god the point, now handling for input");

        if(p.x != 240 && p.y != 320)
        {
          focus = true;

          // check if we are holding
          #if WOKWI_SIM
          if (millis() - lastTouch <= 75)
          #else
          if (millis() - lastTouch <= 200)
          #endif
          {
            holding = true;
          }
          else
          {
            holding = false;
          }

          if(!(UI_SETTINGS & 0b10))
          {
            // println("DRAWING PIXEL -> X:",globalx," Y:",globaly);
            tft.drawPixel(p.x,p.y,color(0,255,0));
          }

          handleInput(p, holding, focus);
          
          if(touchInput != nullptr)
          {
            (*touchInput)(p, holding, focus);
          }

          lastTouch = millis();
        }
      }

      if(millis() - lastTouch > 75 && focus == 1)
      {
        // testprintln("calling handle input for all uis");
        //println("handling input");
        handleInput({0,0}, false, 0);

        // testprintln("success calling handle input for all uis ending function");
        // disable all of the button thingy idk
        holding = 0;
        focus = 0;//undo the focus
      }

    }else if(enabletouch == 0)
    {
      //just tell everything, HEY, WE STOPPED THE TOUCH
      handleInput({0,0}, false, 0);

      // disable all of the button thingy idk
      holding = 0;
      focus = 0;//undo the focus
    }

    if(selected == 1)
    {
      if(kbd.keys.lctrl == 1 && kbd.keys.lshift == 0 && kbd.keys.lalt == 0 && kbd.lastKey == 0 && kbd.currKey == KEY_T && kbdShortcuts != 1)
      {
        kbdShortcuts = 1;//set to one so we ignore repeats
        addTerminal("terminal1", "middle", "middle", 200, 200, color(0,0,0), color(0, 255, 0));
      }

      if(kbd.keysClear())
      {
        kbdShortcuts = 0;//reset kbd shortcuts back to zero so we can use keyboard shortcuts again
      }
    }

    handleKeyboardInput();
  }

  UIManager()
  {
    isManager = true;//set is manager to true because... its the manager

    // x = 0;
    // y = 0;
    // w = SCREEN_WIDTH;
    // h = SCREEN_HEIGHT;
  }
};

UIManager ui;

//global::Text
class Text : public UIelement
{
  public:
  UI* ui;
  bool mode = false;
  String fixedText = "";
  String text = "";
  int8_t fontsize = 1;
  uint16_t colour = 0x0000;

  //for when we want the text to be editable by the user
  uint8_t editable = 0;
  uint8_t lastSelected = 0;
  uint8_t updateTextSize = 1;
  unsigned int lastBufferLength = 0;

  template<typename T>
  void setx(T input)
  {
    Size info = getTextBounds(text, fontsize);
    x = setPos(input, info.w, ui->w, 0);
  }

  template<typename T>
  void sety(T input)
  {
    Size info = getTextBounds(text, fontsize);
    // x = setPos(input, w1, ui->w, 0);
    y = setPos(input, info.h, ui->h, 1);
  }

  //Text::handleKeyboardInput
  void handleKeyboardInput()
  {
    //make sure that we don't accidentally lock editable inside of one state
    if(editable == 1)
    {
      //if the text recieved a new click
      if(selected == 1 && lastSelected == 0)
      {
        lastSelected = 2;

        //update teh keyboard input
        kbd.buffer = text;//the text should have been set to the kbd buffer so it should be fine
        kbd.enableBuffer = false; //If we just clicked on the window, we don't ant to register keyboard for the new presses so disable that bitch

        //we disable the touch (makes the cursor dissapear so we can use the arrows to select text, escape to get out)
        enabletouch = 0;

      //if the text isn't in focus anymore (shouldn't happen) or if the escape key was pressed
      }else if((selected == 0 && lastSelected == 1) || (kbd.checkForKey(KEY_ESCAPE)))
      {
        text = kbd.buffer;//update text for last time 
        lastSelected = 0;//update the lastSelected (its used to tell us if selection has changed)
        selected = 0;//force element to be unselected
        kbd.enableBuffer = false;//disable the buffer (to go back to normal)

        //re-enable the touch
        enabletouch = 1;

        //re-enable the rapid fire delay
        kbd.delayLimiter = 0;
      }

      //if the text is currently being selected
      if(lastSelected == 1)
      {
        text = kbd.buffer;
      }

      //when we actually stop clicking on space
      if(lastSelected == 2 && !kbd.checkForKey(KEY_SPACEBAR))
      {
        lastSelected = 1;
        kbd.enableBuffer = true;
        kbd.buffer = text;
        
        //enable normal key repeat timing for writting on keyboard
        kbd.delayLimiter = 1;
      }
    }
  }

  //Text::draw
  void draw()
  {
    DisplacePointUI d = ui->getTotalDisplacement();
    // tft.drawText(fixedText + text, (x + d.x), (y + d.y), fontsize, colour);
    //combine both the fixed text and the text (for cursor and selector reasons
    // String combined = fixedText + text;
    tft.setCursor((x + d.x), (y + d.y), fontsize, colour, (x + d.x));

    //print the fixed text first
    tft.print(fixedText);

    //if the cursor is inside of the text (bound check)
    if(0 <= kbd.cursorIndex && kbd.cursorIndex <= (int)text.length())
    {
      if(kbd.cursorIndex == 0)
      {
        if(selected == 1)
        {
          //print the cursor
          tft.fillRect(tft.cursor_x, tft.cursor_y - fontsize, 1, (FONT_HEIGHT + 2) * fontsize, color(100, 100, 255));
        }
        
        //print the rest of the text
        tft.print(text.substring(kbd.cursorIndex, text.length()));

      //else if we are at the start of any line draw the cursor one to the right (so we can actually see it)
      }else if(kbd.buffer[kbd.cursorIndex - 1] == '\n')
      {
        //print the first part of the text
        tft.print(text.substring(0, kbd.cursorIndex));

        if(selected == 1)
        {
          //print the cursor
          tft.fillRect(tft.cursor_x, tft.cursor_y - fontsize, 1, (FONT_HEIGHT + 2) * fontsize, color(100, 100, 255));
        }

        //print the rest of the text
        tft.print(text.substring(kbd.cursorIndex, text.length()));


      //if the cursor is at the very tippy left we do smth different
      }else if(kbd.cursorIndex == (int)text.length())
      {
        //print the whole text
        tft.print(text);

        if(selected == 1)
        {
          //print the cursor
          tft.fillRect(tft.cursor_x - 1, tft.cursor_y - fontsize, 1, (FONT_HEIGHT + 2) * fontsize, color(100, 100, 255));
        }

      //if the cursor isn't at some weird place, do standart to display cursor
      }else if(0 <= kbd.cursorIndex && kbd.cursorIndex < (int)text.length())
      {
        //print the first part of the text
        tft.print(text.substring(0, kbd.cursorIndex));

        if(selected == 1)
        {
          //print the cursor
          tft.fillRect(tft.cursor_x - 1, tft.cursor_y - fontsize, 1, (FONT_HEIGHT + 2) * fontsize, color(100, 100, 255));
        }
        
        //print the rest of the text
        tft.print(text.substring(kbd.cursorIndex, text.length()));
      }

    //else if the cursor is, draw the text like a normal person.. without the cursor
    }else
    {

      //print the whole text
      tft.print(text);
    }
  }

  //Text::update
  void update()
  {
    if(updateTextSize == 1)
    {
      Size info = getTextBounds(fixedText + text, fontsize);//(text, 0, 0, &x1, &y1, &w1, &h1);
      w = info.w;
      h = info.h;
    }

    println(text);
    draw();
  }

  //Text::Text
  template <typename T1, typename T2>
  Text(UI* ui1, bool mode, const char* id_input, String text, int fontsize, uint16_t colour, T1 xpos, T2 ypos, uint16_t modew, uint16_t modeh)
  {
    this->ui = ui1;
    // we replicate all of the values above

    id = id_input;//set the pointer of the id to the pointer of the id_input because its a const char pointer most likely

    this->mode = mode;
    this->text = text;
    this->fontsize = fontsize;
    this->colour = colour;
    
    Size info = getTextBounds(text, fontsize);//(text, 0, 0, &x1, &y1, &w1, &h1);

    x = setPos(xpos, info.w, ui->w, 0);
    y = setPos(ypos, info.h, ui->h, 1);

    // ui->addUIpointer(this); // add to the ui manager list
  }

  //Text::~Text
  ~Text()
  {
    delete[] id;//remove the id that was alocated in memory for this ui element
    println("Text destructor for ID : ",id," <- NOTICE ------------------------------------------------------------");
  }
};

//global::Button
class Button : public UIelement
{
  public:
  UI* ui;
  int16_t tx;
  int16_t ty;
  String text = "";
  int8_t fontsize = 1;
  uint16_t colour = 0x0000;
  uint16_t backgroundcolor = 0;
  bool background = false;
  bool usingTextPos;
  bool lastButtonState = false;
  bool buttonState = false;
  bool toggleable = false;
  void (*function)(bool buttonState);

  //Button::setx
  template<typename T>
  void setx(T input)
  {
    x = setPos(input, w, ui->w, 0);
    // y = setPos(input, h, ui->h, 1);
  }

  //Button::sety
  template<typename T>
  void sety(T input)
  {
    // x = setPos(input, w, ui->w, 0);
    y = setPos(input, h, ui->h, 1);
  }

  //Button::setTextx
  template<typename T>
  void setTextx(T input)
  {
    Size info = getTextBounds(text, fontsize);
    tx = setPos(input, info.w, w, 0);
  }

  //Button::setTexty
  template<typename T>
  void setTexty(T input)
  {
    Size info = getTextBounds(text, fontsize);
    // tx = setPos(input, info.w, w, 0);
    ty = setPos(input, info.h, h, 1);
  }

  //Button::handleinput
  void handleInput(Point p, bool holding, bool focus)
  {
    DisplacePointUI d = ui->getTotalDisplacement();

    if(background == false)
    {
      Size info = getTextBounds(text, fontsize);
      //probably not good ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

      bool collided = ::ui.touchCollide(p, (tx + d.x),(ty + d.y),info.w,info.h);

      if(toggleable)
      {
        //if a new touch started 
        if(selected == 1 && holding == 0 && focus == 1 && collided == 1)
        {
          //invert the buttons state
          buttonState = !buttonState;
        }
      }else
      {
        //if a new touch started on the button
        if(selected == 1 && holding == 0 && focus == 1 && collided == 1) 
        {
          //activate the button
          buttonState = 1;
        }else if(selected == 1 && holding == 0 && focus == 0)
        {
          //disactivate the button
          buttonState = 0;
        }
      }
    }else
    {
      bool collided = ::ui.touchCollide(p, (x + d.x),(y + d.y),w,h);

      if(toggleable)
      {
        //if a new touch started 
        if(selected == 1 && holding == 0 && focus == 1 && collided == 1)
        {
          //invert the buttons state
          buttonState = !buttonState;
        }
      }else
      {
        //if a new touch started on the button
        if(selected == 1 && holding == 0 && focus == 1 && collided == 1) 
        {
          //activate the button
          buttonState = 1;
        }else if(selected == 1 && holding == 0 && focus == 0)
        {
          //disactivate the button
          buttonState = 0;
        }
      }

    }
  }

  //Button::draw
  void draw()
  {
    DisplacePointUI d = ui->getTotalDisplacement();

    if(background)
    {
      tft.fillRect((x + d.x),(y + d.y),w,h,backgroundcolor);
    }

    tft.drawText(text, (tx + x + d.x), (ty + y + d.y), fontsize, colour);
  }

  //Button::update
  void update()
  {

    //if the state of the button changed
    if(buttonState != lastButtonState)
    {
      if(function != nullptr)
      {
        (*function)(buttonState);//play the custom function
      }
    }

    draw();

    lastButtonState = buttonState;
  }

  //Button::Button
  template <typename T1, typename T2, typename T3, typename T4>
  Button(UI* ui1, const char* id_input, String text, int fontsize, uint16_t colour, T3 textOffsetX, T4 textOffsetY, bool background, uint16_t backgroundcolor, T1 posX, T2 posY, uint16_t width, uint16_t height, void (*function1)(bool buttonState), bool toggle)
  {
    this->ui = ui1;
    // we replicate all of the values above
    toggleable = toggle;
    
    id = id_input;

    this->text = text;
    this->fontsize = fontsize;
    this->colour = colour;
    
    Size info = getTextBounds(text, fontsize);//(text, 0, 0, &x1, &y1, &w1, &h1);

    
    //if we don't have a background
    if(width == 0 || height == 0 || background == false)
    {
      tx = setPos(textOffsetX, info.w, ui->w, 0);
      ty = setPos(textOffsetY, info.h, ui->h, 1);
    }else
    {
      tx = setPos(textOffsetX, info.w, width, 0);
      ty = setPos(textOffsetY, info.h, height, 1);
    }

    function = function1;
    this->background = background;
    this->backgroundcolor = backgroundcolor;
    this->w = width;
    this->h = height;

    x = setPos(posX, w, ui->w, 0);
    y = setPos(posY, h, ui->h, 1);
  }

  //Button::~Button
  ~Button()
  {
    delete[] id;//de alocation of the id (cuz we alocated that right)
    println("Button destructor for ID : ",id," <- NOTICE ------------------------------------------------------------");
  }
};

//global::Canvas
class Canvas : public UIelement
{
  //Nothing for the moment
};

//global::Window
int globalUpdateDepth = 0;
class Window : public UIelement
{
  public:
  UI* ui1;
  UI ui;//add the ui (this is the middle man between the ui and the user. when user asks for ui.addButton, it adds a button and communicates to the outside via this object)
  uint16_t colour = 0;// had a realisation, to change the fuckkk in teh redrawAffectedUIS thing. we need to put the changed condition outside, like have it in a function to make the function optimized when two elements are in each other, get me?-------------------------------------------------------------------------------------------------------------
  bool startSelect = false;
  Point startPoint;
  void* deathCallbackInput = nullptr;//this is what is going to get inputed to the deathCallBack function
  void (*deathCallback)(void*) = nullptr;//this function gets called during the destructo rof the Window element (this is for some use that I needed, don't ask)

  //Window::updateChildUIVariables
  void updateChildUIVariables()
  {
    ui.x = x;
    ui.y = y + 10;
    ui.w = w;
    ui.h = h - 10;
    ui.basecolor = colour;
  }

  //Window::setx
  void setx(const char* input)
  {
    x = setPos(input, w, ui1->w, 0);
    ui.x = x;
  }

  void setx(int16_t input)
  {
    x = input;
    ui.x = x;
  }

  //Window::sety
  void sety(const char* input)
  {
    y = setPos(input, h, ui1->h, 1);
    ui.y = y + 10;
  }

  void sety(int16_t input)
  {
    y = input;
    ui.y = y + 10;
  }

  //Window::handleInput
  void handleInput(Point p, bool holding, bool focus)
  {
    updateChildUIVariables();

    DisplacePointUI d = ui1->getTotalDisplacement();
    RectangleParams r = ui1->getCombinedRectangle();

    bool windowCollided = ::ui.touchCollide(p, r.x, r.y, r.w, r.h);//this is the previous window bellow. if its outside of the previous window, then we shouldn't be able to click it
    bool movecollided  = ::ui.touchCollide(p, x + d.x, y + d.y, w, 10);//collide(x + d.x, y + d.y, w, 10, p.x - ui1->pointRadius, p.y - ui1->pointRadius, ui1->pointRadius*2, ui1->pointRadius*2);
    bool deleteCollided = ::ui.touchCollide(p, (x + d.x) + w - 15, (y + d.y), 15, 10);

    //the Alt R to resize the windows + a key and the key makes it makes it bigger by like 10 pixels in that direction

    if(focus == 1 && holding == 0 && deleteCollided == 1 && windowCollided == 1) 
    {
      ui1->remove(this);//remove self
    }

    //if we are starting a new click and its on the window
    if(focus == 1 && holding == 0 && movecollided == 1 && windowCollided == 1)
    {
      startSelect = true;
      startPoint = p;
    }else if(focus == 0)
    {
      startSelect = 0;
    }

    if(startSelect)
    {
      // println("COLLIDED, AND MOVING THE WINDOW");

      //get point difference
      int diffx = startPoint.x - p.x;
      int diffy = startPoint.y - p.y;

      //move the window around
      x -= diffx;
      y -= diffy;
      //also update all of the ui resting upon the window
      ui.x -= diffx;
      ui.y -= diffy;

      //set the reference point to the current point
      startPoint = p;
    }

    ui.handleInput(p, holding, focus);
  }
  
  //Window::handleKeyboardInput
  void handleKeyboardInput()
  {
    //if the window is currently selected
    if(selected == 1)
    {
      //if we are only pressing alt
      if(kbd.keys.lctrl == 1 && kbd.keys.lalt == 0 && kbd.keys.lshift == 0)
      {
        //if we are pressing the R key
        if(kbd.checkForKey(KEY_R))
        {
          //if we just pressed the up arrow
          if(kbd.checkForKey(KEY_UPARROW))
          {
            //update the size of the window
            y -= 5;
            h += 5;
          }
          
          //if we just pressed the down arrow
          if(kbd.checkForKey(KEY_DOWNARROW))
          {
            //update the size of the window
            h += 5;       
          }
          
          //if we just pressed the left arrow
          if(kbd.checkForKey(KEY_LEFTARROW))
          {
            //update the size of the window
            x -= 5;
            w += 5;
          }
          
          //if we just pressed the right arrow
          if(kbd.checkForKey(KEY_RIGHTARROW))
          {
            //update the size of the window
            w += 5;       
          }
        }
      }else if(kbd.keys.lctrl == 1 && kbd.keys.lalt == 0 && kbd.keys.lshift == 1)
      {
        //if we are pressing the R key
        if(kbd.checkForKey(KEY_R))
        {
          //if we just pressed the up arrow
          if(kbd.checkForKey(KEY_UPARROW))
          {
            //update the size of the window
            h -= 5;       
          }
          
          //if we just pressed the down arrow
          if(kbd.checkForKey(KEY_DOWNARROW))
          {
            //update the size of the window
            y += 5;
            h -= 5;
          }
          
          //if we just pressed the left arrow
          if(kbd.checkForKey(KEY_LEFTARROW))
          {
            //update the size of the window
            w -= 5;     
          }
          
          //if we just pressed the right arrow
          if(kbd.checkForKey(KEY_RIGHTARROW))
          {
            //update the size of the window
            x += 5;
            w -= 5;  
          }
        }
      }else if(kbd.keys.lctrl == 0 && kbd.keys.lalt == 0 && kbd.keys.lshift == 0)
      {
        //go full screen
        if(kbd.checkForKey(KEY_F11))
        {
          x = 0;
          y = -10;
          w = SCREEN_WIDTH;
          h = SCREEN_HEIGHT + 10;
        }
      }
    }

    ui.handleKeyboardInput();
  }

  //Window::draw
  void draw()
  {
    updateChildUIVariables();
    DisplacePointUI d = ui1->getTotalDisplacement();
    
    // Draw window background
    tft.fillRect(d.x + ui.x, d.y + ui.y, ui.w, ui.h, colour);

    // Store viewport
    int16_t vx = tft.vx;
    int16_t vy = tft.vy;
    int16_t vw = tft.vw;
    int16_t vh = tft.vh;
    
    // NOW get comed rectangle for child UI
    {
      RectangleParams r = ui.getCombinedRectangle();

      tft.setViewport(r.x, r.y, r.w, r.h);
      
      ui.update();
      
      tft.setViewport(vx, vy, vw, vh);
    }
    
    // Draw window decorations
    tft.fillRect((x + d.x), (y + d.y), w, 10, color(100,100,100));
    tft.fillRect((x + d.x) + w - 15, (y + d.y), 15, 10, color(255,0,0));//if you ever change this, change the handle input collision too
    
    tft.drawLine(((x + d.x) + w - 15)+5, ((y + d.y))+2, ((x + d.x) + w)-5, ((y + d.y) + 10)-3, color(0,0,0));
    tft.drawLine(((x + d.x) + w - 15)+5, ((y + d.y) + 10)-3, ((x + d.x) + w)-5, ((y + d.y))+2, color(0,0,0));
  }

  void update()
  {
      updateChildUIVariables();
      draw();
  }

  //Window::Window
  template <typename T1, typename T2>
  Window(UI* ui1, const char* id_input, uint16_t colour, T1 xpos, T2 ypos, uint16_t width, uint16_t height)
  {
    // we replicate all of the values above
    ui.prevUI = ui1;//link up the two UI manager classes together to fix the broken displacement
    ui.parent = this;//link up this uielement to the UI manager class so they can work in tandem
    this->ui1 = ui1;
    
    // if(id_input != nullptr)
    // {
    //   id = new char[strlen(id_input) + 1]; // tell c++ to give us some storage and give the pointer to that storage
    //   strcpy(id, id_input);                // copy the id_input string to the location in storage
    // }
    id = id_input;

    this->colour = colour;
    this->w = width;
    this->h = height;

    //set the position based on the thing idk
    x = setPos(xpos, w, ui1->w, 0);
    y = setPos(ypos, h, ui1->h, 1);

    updateChildUIVariables();
  }

  //Window::~Window
  ~Window()
  {
    println("Windows destructor for ID : ",id," <- NOTICE ------------------------------------------------------------");

    //play the mistery deathCallback function if it is not a nullptr
    if(deathCallback != nullptr)
    {
      (*deathCallback)(deathCallbackInput);//play the function
    }

    //clear all of the elements in the Array
    ui.clear();
  }
};

template <typename T1, typename T2>
Text& UI::addText(const char *id_input, String text, int fontsize, uint16_t colour, T1 posX, T2 posY)
{
  Text* element = new Text(this, false, id_input, text, fontsize, colour, posX, posY, 0, 0);
  uiPointerArray.add((UIelement*)element);
  return *element;
}

// get the desired ui
Text& UI::getText(const char *id)
{
  // we go through all of the ui elements
  return *((Text*)(findElementWithId(id)));

  // if we didn't find a corresponding id
  println("ERROR, couldn't find '", id, "' in the list of ui elements");
}

template <typename T1, typename T2, typename T3, typename T4>
Button& UI::addButton(const char* id_input, String text, int fontsize, uint16_t colour, T3 textOffsetX, T4 textOffsetY, bool background, uint16_t backgroundcolor, T1 posX, T2 posY, uint16_t width, uint16_t height, void (*function1)(bool input), bool toggle)
{
  Button* element = new Button(this, id_input, text, fontsize, colour, textOffsetX, textOffsetY, background, backgroundcolor, posX, posY, width, height, function1);
  uiPointerArray.add((UIelement*)element);
  return *element;
}

// get the desired ui
Button& UI::getButton(const char* id)
{
  return *((Button*)findElementWithId(id));

  // if we didn't find a corresponding id
  println("ERROR, couldn't find '", id, "' in the list of ui elements");
}

template <typename T1, typename T2>
Window& UI::addWindow(const char* id_input, uint16_t colour, T1 xpos, T2 ypos, uint16_t width, uint16_t height)
{
  Window* element = new Window(this, id_input, colour, xpos, ypos, width, height);
  uiPointerArray.add((UIelement*)element);
  return *element;
}

Window& UI::getWindow(const char* id)
{
  return *((Window*)findElementWithId(id));

  // if we didn't find a corresponding id
  println("ERROR, couldn't find '", id, "' in the list of ui elements");
}

//global::Terminal
class Terminal : public UIelement
{
  public:
  UI* ui;//the current ui that is holding this ui element
  Window& window = ::ui.addWindow("Terminal_Window", color(50, 50, 50), 0, 0, 50, 50);//hostage window
  Text& text = window.ui.addText("Terminal_Text", "", 1, color(0, 255, 0), 0, 0);//hostage text
  uint8_t terminalSelect = 0;//helper variable to see if teh window just got selected
  uint8_t pressedEnter = 0;//helper variable to not repeat Enter when held
  uint8_t windowDied = 0;//helper variable to check if the window died, and if it did, we should delete the whole Terminal object as well
  uint8_t barCollision = 0;//stop code from registering  

  //if the window ever dies then its going to call this function (make it static so complier don't go KABOOM)
  static void deathCallbackTerminal(void* input)
  {
    if(input != nullptr)
    {
      uint8_t& var = *((uint8_t*)(input));//transform mistery input into our variable
      var = 1;//set windowDied to true
    }else
    {
      println("ERROR line ",__LINE__,", the input was a nullptr");
    }
  }

  //Terminal::handleInput
  void handleInput(Point p, bool holding, bool focus)
  {
    //if we started a new click
    if(holding == 0 && focus == 1)
    {
      DisplacePointUI d = ::ui.getTotalDisplacement();

      //did the touch touch the top bar of the window
      barCollision = ::ui.touchCollide(p, d.x + window.x, d.y + window.y, window.w, 10);
    }
  }

  //Terminal::handleKeyboardInput
  void handleKeyboardInput()
  {
    //is the child window gone
    if(windowDied)
    {
      ui->remove(id);//self destruct if child window is gone
    }

    // DEBUG(window.selected);

    //when we click the window (select it) it should also select the text
    if((window.selected == 1 || window.selected == 2) && terminalSelect == 0 && !barCollision)
    {
      terminalSelect = 1;//update terminalSelect
      text.selected = 1;
    }

    //when we deselect the text, it should also deselect the window
    if(text.selected == 0 && !barCollision)
    {
      terminalSelect = 0;//update terminalSelect
      window.selected = 0;
    }

    //if the text is being selected
    if(text.selected == 1)
    {  
      //did we press enter
      if(kbd.keys.lctrl == 0 && kbd.keys.lalt == 0 && kbd.keys.lshift == 0 && kbd.currKey == KEY_ENTER && pressedEnter == 0)
      {
        //find the last '\n' char
        int index = kbd.buffer.length() - 2;
        while(index > 0 && kbd.buffer[index] != '\n')
        {
          index -= 1;
        }

        String textCommand = "";
        if((kbd.buffer.length() - 2) - index <= 0)
        {
          println("ERROR line ",__LINE__,", zero or bellow zero length command");
        }else
        {
          //get the command based on the last '\n' character index
          textCommand = kbd.buffer.substring(index, kbd.buffer.length() - 1); //get the specific command typed in the chat
        }

        //throw whatever we just wrote into the fixed size (so we can't backspace it away)
        text.fixedText += kbd.buffer + "unknown command : '" + textCommand + "' \n";

        //clear both buffers and text Just to be sure
        text.text = "";
        kbd.buffer = "";
        
        pressedEnter = 1;//update pressedEnter to prevent spam

        kbd.enableBuffer = false;//disable the buffer until we unpress enter .. just to be sure

      //if its literally any other key other then enter
      }else if(!kbd.checkForKey(KEY_ENTER) && pressedEnter == 1)
      {
        //then we can put back pressedEnter
        pressedEnter = 0;

        //and also re-enable the buffer since we disabled it last command
        kbd.enableBuffer = true;
      }
        
    }

  }

  //Terminal::update
  void update()
  {
    //is the child window gone?
    if(windowDied)
    {
      ui->remove(id);//self destruct if child window is gone
    }
  }

  //Terminal::Terminal
  template <typename T1, typename T2>
  Terminal(UI* ui1, const char* id_input, T1 window_x, T2 window_y, int16_t window_w, int16_t window_h, uint16_t backgroundColor, uint16_t textColor)
  {
    //link up the ui that manages this element to this 
    ui = ui1;

    //set the id
    // id = new char[strlen(id_input) + 1]; // tell c++ to give us some storage and give the pointer to that storage
    // strcpy(id, id_input);                // copy the id_input string to the location in storage
    id = id_input;

    //update window to match
    window.colour = backgroundColor;
    window.w = window_w;
    window.h = window_h;
    window.setx(window_x);
    window.sety(window_y);

    //make sure that the index is in the right order so the terminal's update function always gets called first
    window.z_index = -100;
    text.z_index = -99;
    this->z_index = -98;

    //update text to match
    text.colour = textColor;
    text.editable = 1; //make the text editable
    text.settings &= ~0b1; //toggle OFF the settings that erases selection lines (so we don't see ugly rectangle arround the text box inside the termianl)
    text.updateTextSize = 0;// toggle OFF update text size to stop the text from ever updating the width and height so we can modify it ourselfs

    //change text dimentions to negative so that it doesn't overlap with anything (it'll mess with our logic otherwise)
    text.w = -1;
    text.h = -1; 

    //make sure position isn't a problem (because this is kind of like an invisible ui element)
    x = -100;
    y = -100;
    w = 0;
    h = 0;

    //add a death callback function to auto delete the Terminal class if the window was ever to be deleted
    window.deathCallback = Terminal::deathCallbackTerminal;
    window.deathCallbackInput = &windowDied;
  }

  ~Terminal()
  {
    delete[] id;//remove the id that was alocated in memory for this ui element
    println("Terminal destructor for ID : ",id," <- NOTICE ------------------------------------------------------------");
  }
};

template <typename T1, typename T2>
Terminal& UI::addTerminal(const char* id_input, T1 window_x, T2 window_y, int16_t window_w, int16_t window_h, uint16_t backgroundColor, uint16_t textColor)
{
  Terminal* element = new Terminal(this, id_input, window_x, window_y, window_w, window_h, backgroundColor, textColor);
  uiPointerArray.add((UIelement*)element);
  return *element;
}

Terminal& UI::getTerminal(const char* id)
{
  return *((Terminal*)findElementWithId(id));

  // if we didn't find a corresponding id
  println("ERROR, couldn't find '", id, "' in the list of ui elements");
}


void nullfunc(bool nothing){}

/*
// void drawScreen()
// {
  //   tft.fillScreen(ui.basecolor);
  //   tft.setCursor(10, 10);
  //   tft.setTextColor(0x0000);
  //   tft.setTextSize(2);
  //   tft.println("OS BOOTING...");
  
  //   tft.fillRect(10, 245, 50, 20, color(128, 120, 128));
  //   tft.setCursor(17, 251);
  //   tft.setTextColor(0x0000);
  //   tft.setTextSize(1);
  //   tft.println("Button");
  
  //   tft.fillRect(10, 305, 220, 5, color(128, 120, 128)); // uhm this is like...a slider
// }


// void buttonPressed(bool input)
// {
  //   println("THE BUTTON WAS PRESSED : ",input);
  // }
  
  
  // void setup()
  // {
    //   ui.begin();
    //   //ui.addText("Terminal", "terminal", 1, color(255,255,255), "left", "top");
    
    //   ui.uiPointerArray.add(&window);
    
    //   ui.update();
    
    //   //delay(1000);
    
    //   window.x += 10;
    //   window.y += 10;
    //   window.w -= 50;
    //   window.h -= 50;
    
    //   //delay(100000);
    
    //   //delay(10000);
    // }
    
    
    // bool holding = false;
    // bool holdingSlider = false; // 0 : nothing or not holding slider, 1 : holding slider
    // bool holdingButton = false;
    // bool buttonState = false;
    // bool lastButtonState = false;
    // unsigned long lastTouch = 0;
    // int sliderValue = 0; // this is the slider value out of a hundread
    // int lastSliderValue = -1;
    // bool focus = false;
    void loop()
{
  // tft.fillScreen(ui.basecolor);
  ui.update();
  
  //delay(100);
  if(touched())
  {
    Point p = getPoint();
    
    focus = true;
    
    // check if we are holding
    #if WOKWI_SIM
    if (millis() - lastTouch <= 75)
    #else
    if (millis() - lastTouch <= 75)
    #endif
    {
      holding = true;
    }
    else
    {
      holdingSlider = 0; // we reset the holding slider to nothing
      
      holdingButton = 0;
      buttonState = 0;

      holding = false;
    }
    
    //println("handling input");
    ui.handleInput(p, holding, focus);
    
    // println("is holding ? : ",holding);
    
    // println("Touch detected at: X=",p.x," Y=",p.y);
    
    if (!(p.x == 240 && p.y == 320))
    {
      // Map coordinates as needed (capacitive often requires less mapping than resistive)
      
      tft.drawPixel(p.x, p.y, color(0, 0, 255));
      
      // Add UI interaction logic here
      if ((collide(10, 305, 220, 5, p.x - 4, p.y - 4, 8, 8) && (holding == 0 && holdingSlider == 0)) || (holdingSlider == 1))
      {
        // if its the first time we are clicking on the screen (like after letting go)
        if (holding == false)
        {
          println("holding slider = 1");
          holdingSlider = 1;
        }
        
        // get what the slider value would have to be
        
        //  - distance : (p.x - 10)
        //  - percentage : (p.x - 10) / 220 * 100
        
        sliderValue = (p.x - 10) * 100 / 220;
        
        // limit slider value
        sliderValue = min(max(sliderValue, 0), 100);
      }
      
      if (lastSliderValue != sliderValue)
      {
        // get rid of last slider point
        tft.fillCircle(10 + lastSliderValue * 220 / 100, 305 + 5 / 2, 7, ui.basecolor);
        tft.fillRect(10, 305, 220, 5, color(128, 120, 128));
        
        // set little dot on the slider
        tft.fillCircle(10 + sliderValue * 220 / 100, 305 + 5 / 2, 7, color(100, 94, 100));
        
        // get rid of last text
        tft.setTextSize(2);
        tft.setCursor(10, 305 - 30);
        tft.setTextColor(ui.basecolor);
        tft.print(lastSliderValue);
        tft.print("%");
        
        // set new text
        tft.setCursor(10, 305 - 30);
        tft.setTextColor(color(50, 50, 50));
        tft.print(sliderValue);
        tft.print("%");
      }
      
      // UI interaction logic for the button
      if (collide(10, 245, 50, 20, p.x - 4, p.y - 4, 8, 8))
      {
        // if its the first time we are clicking on the screen (like after letting go)
        if (holding == false)
        {
          holdingButton = 1;
        }
        
        if (holdingButton == true)
        {
          buttonState = true;
        }
      }
      else if (holding == 1)
      {
        buttonState = false;
      }
      
      if (lastButtonState != buttonState)
      {
        if (buttonState == 1)
        {
          tft.fillRect(10, 245, 50, 20, color(160, 145, 170));
          tft.setTextColor(color(100, 100, 100));
        }
        else
        {
          tft.fillRect(10, 245, 50, 20, color(128, 120, 128));
          tft.setTextColor(0x0000);
        }
        
        tft.setCursor(17, 251);
        tft.setTextSize(1);
        tft.println("Button");
      }
      
      lastTouch = millis();
      lastSliderValue = sliderValue;
      lastButtonState = buttonState;
    }
  }
  
  if(millis() - lastTouch > 75 && focus == 1)
  {
    //println("handling input");
    ui.handleInput({0,0}, false, 0);
    // disable all of the button thingy idk
    holding = 0;
    buttonState = 0;
    holdingButton = 0;
    focus = 0;//undo the focus
    
    // we display the button state (its the same as the one in the touched thingy)
    if (lastButtonState != buttonState)
    {
      if (buttonState == 1)
      {
        tft.fillRect(10, 245, 50, 20, color(160, 145, 170));
        tft.setTextColor(color(100, 100, 100));
      }
      else
      {
        tft.fillRect(10, 245, 50, 20, color(128, 120, 128));
        tft.setTextColor(0x0000);
      }
      
      tft.setCursor(17, 251);
      tft.setTextSize(1);
      tft.println("Button");
    }
    
    lastButtonState = 0; // don't forget to set the lastButtonState variable
  }
}

// void loop()
// {
  //   #if true
  
  //     ui.update();
  //     // window.x += 1;
  //     // window.y += 1;
  
  
  //     //delay(10);
  
  //     // Text& terminal = ui.getText("Terminal");
  
  //     // terminal.text = number;
  
  //     // ui.update();
  
  //     // delay(1000);
  
  //     // number++;
  //   #else
  //     kbd.update();
  
  //     Text& terminal = ui.getText("Terminal");
  
  //     if(kbd.available())
  //     {
    //       terminal.text = kbd.getBuffer();
    //     }
    
    //     ui.update();
    //   #endif
    // }
*/
    
#endif
