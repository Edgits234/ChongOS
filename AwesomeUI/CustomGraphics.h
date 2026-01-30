#ifndef CustomGraphics_h
#define CustomGraphics_h
 
#define LINE_DEBUG println("line : ",__LINE__);

// #define SPI_HW ((SPI_TypeDef *)0x40013000)
// #define SPI_HW 0x40013000//((SPI_TypeDef *)0x40015000)// 0x40015000 //SPI5//((SPI_TypeDef *) SPI1_BASE) 
#define SPI_HW SPI5
#define SPI_SETTINGS 40000000//8000000
#include <SPI.h>

#define TAB_WIDTH 8//max size that is going to be done

// I STOLE THIS FROM TFT_eSPI *MAKING THIS KNOWN OUT THERE*
// Default color definitions
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xD69A      /* 211, 211, 211 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFE19      /* 255, 192, 203 */ //Lighter pink, was 0xFC9F
#define TFT_BROWN       0x9A60      /* 150,  75,   0 */
#define TFT_GOLD        0xFEA0      /* 255, 215,   0 */
#define TFT_SILVER      0xC618      /* 192, 192, 192 */
#define TFT_SKYBLUE     0x867D      /* 135, 206, 235 */
#define TFT_VIOLET      0x915C      /* 180,  46, 226 */
#define BLOOD_IS_THE_LAW 1 
// 1. Helper macros to turn whatever is in WOKWI_SIM into a "string"
#define STRINGIZE_INTERNAL(x) #x
#define STRINGIZE(x) STRINGIZE_INTERNAL(x)

// 2. A simple C++11 constexpr function to compare strings at compile-time
constexpr bool strings_equal(char const* a, char const* b) {
    return *a == *b && (*a == '\0' || strings_equal(a + 1, b + 1));
}

// 3. The check
// This will throw a compiler error if SOMETHING is anything else.
static_assert(!strings_equal(STRINGIZE(WOKWI_SIM), "[0 or 1]"), "haha very funny. you did  \"#define WOKWI_SIM [0 or 1]\" ... but seriously, do something like '#define WOKWI_SIM 1'\n\n\n");

// Are we running Wokwi simulation? (automatically checks based on the current board (if its a mega its wokwiSIM))
#ifndef WOKWI_SIM
  #if defined(__AVR_ATmega2560__) || defined(ARDUINO_AVR_MEGA2560)
    #define WOKWI_SIM 1
  #elif defined(ARDUINO_GIGA)
    #define WOKWI_SIM 0
  #else
    #error "ERROR, if you don't have a GIGA or a MEGA board, this library most likely wont work, if you want to try anyways do '#define WOKWI_SIM [0 or 1]' just before including the library"
  #endif
#endif

#if WOKWI_SIM
    #define SPI_BUS SPI
#else
    #include "SDRAM.h"

    //no longer needed  
    // #define SPI_BUS SPI1
    #define SPI_BUS println("YOU FORGOT TO GET RID OF THE SPI_BUS AT LINE ",__LINE__);SPI1

    //buffer instead : 
    // Point directly to SDRAM base + offset to avoid conflicts

    #define SCREEN_BUFFER_SIZE SCREEN_WIDTH*2*SCREEN_HEIGHT
    inline uint8_t* screenBuffer = nullptr;

    // Hardware pointer to SPI5 (the actual peripheral pins 11,12,13 use)
    #define SPI5_HW ((SPI_TypeDef *)SPI5_BASE)
#endif

struct Color
{
  byte r;
  byte g;
  byte b;
};

struct Point
{
  int16_t x;
  int16_t y;
};

struct Size
{
    int16_t w;
    int16_t h;
};

struct BoundingBox
{
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

//custom nice to have quick prints : 
template <typename T> void print(T input){Serial.print(input);} template<typename T, typename... Args> void print(T input, Args... other){Serial.print(input); print(other...);}inline void println(){Serial.println();}template <typename T>void println(T input) {Serial.println(input);}template <typename T, typename... Args> void println(T input, Args... other){Serial.print(input);println(other...);}
template <typename T> void print1(T input){Serial.print(input);} template<typename T, typename... Args> void print1(T input, Args... other){Serial.print(input); Serial.print(", "); print1(other...);}inline void println1(){Serial.println();}template <typename T>void println1(T input) {Serial.println(input);}template <typename T, typename... Args> void println1(T input, Args... other){Serial.print(input);Serial.print(", ");println1(other...);}
#if enableTestPrints
inline void testprintln(){Serial.println();}template <typename T>void testprintln(T input) {Serial.println(input);}template <typename T, typename... Args> void testprintln(T input, Args... other){Serial.print(input);println(other...);}
#else
inline void testprintln(){}template <typename T>void testprintln(T input) {}template <typename T, typename... Args> void testprintln(T input, Args... other){}
#endif

//the holy grail of printing functions
template <typename T>
void debugHelper(int index, const char* funcInput, T input)
{
    //find the next comma (or the end char)
    while(funcInput[index] != ',' && funcInput[index] != '\0')
    {
        if(funcInput[index] == ' ')
        {
            index++;
            continue;
        }
        print(funcInput[index]);
        index++;
	}

    print(" = ");
    println(input);

    index++;
}

template <typename T, typename... Args>
void debugHelper(int index, const char* funcInput, T input, Args... other)
{
    //find the next comma (or the end char)
    while(funcInput[index] != ',' && funcInput[index] != '\0')
    {
        if(funcInput[index] == ' ')
        {
            index++;
            continue;
        }
        print(funcInput[index]);
        index++;
	}

    print(" = ",input,", ");

    //skip the comma for next variable
    index++;

    debugHelper(index, funcInput, other...);
}

template <typename T>
void printArrayHelper(T* input, size_t size)
{
    for(int i = 0; i < size; i++)
    {
        print(input[i]);
        if(i != size - 1)
        {
            print(", ");
        }
    }
}

//very fucking cool function that helps you print an array without having to input the size
#define printArray(x) printArrayHelper(x, sizeof(x) / sizeof(x[0]))
#define printlnArray(x) printArrayHelper(x, sizeof(x) / sizeof(x[0])); println()

#define DEBUG(...) debugHelper(0, #__VA_ARGS__, __VA_ARGS__)

#define DC_PIN 8
#define CS_PIN 10
#define RST_PIN 9

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

#define FRAMEBUFFER_SIZE SCREEN_WIDTH*SCREEN_HEIGHT*2

#define FONT_WIDTH  5
#define FONT_HEIGHT 6

#define ILI9341_CASET 0x2A
#define ILI9341_PASET 0x2B
#define ILI9341_RAMWR 0x2C

//for indexing the right character
inline const char* fontChar = "\x1A""aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ1234567890+-.,!?=:/*()'_$<>[]~";

//the font
const uint8_t font[] = {0b00000001,0b00010101,0b00010101,0b00010000,0b00000000,0b01110100,0b10100100,0b11110111,0b01000111,0b11110001,0b10001100,0b01100001,0b00001110,0b01001010,0b01001100,0b11110100,0b01111101,0b00011000,0b11111000,0b00000000,0b01110100,0b00100000,0b11100111,0b01000110,0b00010000,0b10001011,0b10000010,0b00010111,0b11000110,0b00101111,0b11110100,0b01100011,0b00011000,0b11111000,0b00001110,0b10001111,0b10100000,0b11101111,0b11000011,0b10010000,0b10000111,0b11000100,0b01000111,0b00010000,0b10000100,0b11111100,0b00111001,0b00001000,0b01000000,0b00001110,0b10001011,0b11000010,0b11100111,0b01000010,0b00010011,0b10001011,0b11100001,0b00001111,0b01000110,0b00110001,0b10001100,0b01111111,0b00011000,0b11000100,0b00000100,0b00000001,0b00001000,0b01000111,0b00010000,0b10000100,0b00100011,0b10001000,0b00000010,0b00010000,0b10001000,0b11111000,0b10000100,0b00101001,0b00110010,0b00010000,0b10100110,0b00101001,0b00101000,0b11001011,0b00010100,0b10010100,0b01001000,0b01000010,0b00010000,0b10000010,0b10000100,0b00100001,0b00001000,0b01111100,0b00000000,0b01010101,0b01101011,0b00011101,0b11010110,0b10110101,0b10101101,0b01000000,0b00001111,0b01000110,0b00110001,0b10001110,0b01101011,0b01011001,0b11000100,0b00000000,0b01110100,0b01100010,0b11100111,0b01000110,0b00110001,0b10001011,0b10000001,0b11101000,0b11111010,0b00010000,0b11110100,0b01100011,0b11101000,0b01000000,0b00001111,0b10001011,0b11000010,0b00010111,0b01000110,0b00110001,0b10011011,0b11000000,0b00000100,0b00111101,0b00001000,0b11110100,0b01111101,0b00011000,0b11000100,0b00000110,0b01000001,0b00000100,0b11000111,0b01000001,0b10000011,0b10001011,0b10001000,0b11100010,0b00010000,0b10000100,0b11111001,0b00001000,0b01000010,0b00010000,0b00000000,0b10001100,0b01100010,0b11111000,0b11000110,0b00110001,0b10001011,0b10000000,0b00001000,0b11000101,0b01000100,0b10001100,0b01100010,0b10100101,0b00010000,0b00000000,0b10101101,0b01101010,0b10101010,0b11010110,0b10110101,0b10101010,0b10000001,0b00011000,0b10111010,0b00110001,0b10001100,0b01011101,0b00011000,0b11000100,0b00010001,0b10001011,0b11000010,0b11101000,0b11000101,0b11000100,0b00100001,0b00000000,0b00001111,0b10001001,0b00011111,0b11111000,0b10001000,0b10001000,0b01111100,0b10001100,0b00100001,0b00001000,0b11100111,0b01000100,0b01000100,0b01000111,0b11011101,0b00010011,0b00000110,0b00101110,0b10010100,0b10111110,0b00100001,0b00001011,0b11110000,0b11110000,0b01100010,0b11100111,0b11000011,0b11010001,0b10001011,0b10111110,0b00010001,0b00010001,0b00001000,0b01110100,0b01011101,0b00011000,0b10111001,0b11010001,0b01111000,0b01000010,0b00010111,0b01000110,0b10110101,0b10001011,0b10000000,0b01000010,0b01111100,0b10000100,0b00000000,0b00000001,0b11110000,0b00000000,0b00000000,0b00000000,0b00000000,0b01000000,0b00000000,0b00000000,0b00100001,0b00001000,0b01000010,0b00010000,0b00000100,0b01110100,0b01000100,0b01000000,0b00010000,0b00000000,0b11111000,0b00111110,0b00000000,0b00010000,0b00000000,0b00100000,0b00000000,0b00010001,0b00010001,0b00010000,0b01010001,0b00010100,0b00000000,0b00000000,0b10001000,0b01000010,0b00010000,0b01000010,0b00001000,0b01000010,0b00010001,0b00001000,0b01000010,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b01111100,0b10001110,0b10100011,0b10001010,0b11100000,0b00001000,0b10001000,0b00100000,0b10000000,0b10000010,0b00001000,0b10001000,0b01110010,0b00010000,0b10000100,0b00111001,0b11000010,0b00010000,0b10000100,0b11100000,0b00000001,0b00010101,0b00010000,0b00000000};

//global::Array
template<typename T>
class Array
{
  public:
  T array[10];
  unsigned int size = 0;

  //Array::add
  void add(T input)
  {
    //If the size is bellow the total size of the array
    if(size < (sizeof(array) / sizeof(T)))
    {
      //we add an item at the end of the list
      array[size] = input;
      size += 1;//update the size
    }else 
    {
      println("ERROR, function 'add()' wasn't able to add an element to the end of the list because the array is full (max array size is : ",(sizeof(array) / sizeof(T)),", while current size is : ",size,")");
    }
  }

  //Array::remove
  void remove()
  {
    //if the size is bigger than 0 (cuz if size 0 than can't remove shit)
    if(size > 0)
    {
      size -= 1;//just make size smaller (we gonna overide values later)
    }else
    {
      println("ERROR, function 'remove()' wasn't able to remove the last element because the array size was ",size);
    }
  }

  //Array::remove
  void remove(unsigned int index)
  {
    //if the index is between 0 and the current size of the array (if its not, than can't delete)
    if(0 <= index && index <= min((size - 1), (sizeof(array) / sizeof(T) - 1)))
    {
      //go through all of the items after the index and move them one to the left
      for(unsigned int i = index; i < (size - 1); i++)
      {
        array[i] = array[i + 1];
      }

      //reduce the size
      size -= 1;
    }else
    {
      println("ERROR, tried accessing index : ",index," but the range of possible is : ",0," to ",size - 1);
    }
  }

  //Array::at
  T& at(unsigned int index)
  {
    //check if in the array
    if(0 <= index && index <= min((size - 1), (sizeof(array) / sizeof(T) - 1)))
    {
      return array[index];

    //if its not, give error
    }else
    {
      println("ERROR, tried accessing index : ",index," but the range of possible indexs are : ",0," to ",min((size - 1), (sizeof(array) / sizeof(T) - 1)));
    }

    println("WARNING : control reaches end of non-void function [-Wreturn-type] at line ",__LINE__);
  }

  T& operator[](unsigned int index)
  {
    return array[index];
  }

  //Array::Array
  Array(){};
  Array(const Array&) = delete;
  Array& operator=(const Array&) = delete;

  //Array::~Array
  ~Array()
  {
    
  }
};

inline int16_t clamp16(int input)
{
    int clamped = min(INT16_MAX, max(INT16_MIN, input));
    if(clamped != input) 
    {
        #ifndef DISABLE_WARNINGS 
            Serial.print(F("WARNING, clamped number '")); Serial.print(input); Serial.print(F("' to '")); Serial.print(clamped); Serial.println(F("'"));
        #endif
    }
    return (int16_t)(clamped);
}

inline int16_t clamp16(double input)
{
    return clamp16((int)input);
}

inline uint16_t color(byte red, byte green, byte blue)
{    
    #if true //WOKWI_SIM
        // maping the colors from 255 to  their respective map
        red = red * 32 / 256;
        green = green * 64 / 256;
        blue = blue * 32 / 256;

        return ((uint16_t)(red) << 11) + ((uint16_t)(green) << 5) + (uint16_t)(blue);
    #else
        // maping the colors from 255 to  their respective map
        red = red * 64 / 256;
        green = green * 32 / 256;
        blue = blue * 32 / 256;

        return ((uint16_t)(blue) << 11) + ((uint16_t)(red) << 5) + (uint16_t)(green);
    #endif
}

inline uint16_t color(Color input)
{
  // maping the colors from 255 to  their respective map
  input.r = input.r * 32 / 256;
  input.g = input.g * 64 / 256;
  input.b = input.b * 32 / 256;
  
  return ((uint16_t)(input.r) << 11) + ((uint16_t)(input.g) << 5) + (uint16_t)(input.b);
}

inline Color toColor(uint16_t input)
{
  Color variable;

  variable.r   = ((input & 0xF800) >> 11) * 256 / 32;
  variable.g = ((input & 0x7E0)  >>  5) * 256 / 62;
  variable.b  = ((input & 0x1F)   >>  0) * 256 / 32;

  return variable;
}

inline uint16_t color565(int r, int g, int b)
{
    r = max(0, min(31, r));
    g = max(0, min(63, g));
    b = max(0, min(31, b));

    return ((uint16_t)(r) << 11) + ((uint16_t)(g) << 5) + (uint16_t)(b);
}

inline Color toColor565(uint16_t input)
{
    Color c = {0, 0, 0};
    c.r = ((input & 0xF800) >> 11);
    c.g = ((input & 0x7E0)  >>  5);
    c.b = ((input & 0x1F)   >>  0);

    return c;
}

inline long long powi(long base, long exp) {
    long long res = 1;
    for (int i = 0; i < exp; i++) res *= base;
    return res;
}

inline unsigned long long upowi(long base, long exp){
    unsigned long long res = 1;
    for (int i = 0; i < exp; i++) res *= base;
    return res;
}

inline int getDigits(long long input, int i1, int i2 = -1)
{
    if(i2 == -1)
    {
        i2 = i1;
    }

    return (input / powi(10, i1 - 1) * powi(10, i1 - 1) - input / powi(10, i2) * powi(10, i2)) / powi(10, i1 - 1);
}

inline int getDigits(unsigned long long input, int i1, int i2 = -1)
{
    if(i2 == -1)
    {
        i2 = i1;
    }

    return (input / upowi(10, i1 - 1) * upowi(10, i1 - 1) - input / upowi(10, i2) * upowi(10, i2)) / upowi(10, i1 - 1);
}


/*this is a very dumb function to let me see differences between stuff that have a different index so I can see if my shit is optimal, it probably doesn't do what you think it does*/
inline uint16_t convertNumberToColor(uint8_t number)
{
    number %= 10;   

    switch (number)
    {
        case 0: return TFT_RED;
        case 1: return TFT_ORANGE;
        case 2: return TFT_YELLOW;
        case 3: return TFT_GREENYELLOW;
        case 4: return TFT_GREEN;
        case 5: return TFT_CYAN;
        case 6: return TFT_BLUE;
        case 7: return TFT_PURPLE;
        case 8: return TFT_MAGENTA;
        case 9: return TFT_PINK;
    }

    return 0;
}

inline bool fontUnpacker(int i)
{
  int index = i / 8;
  bool output = (font[index] & (1 << (7 - (i % 8)))) != 0;
  return output;
}

inline void writeCommand(uint8_t cmd) {
    #if WOKWI_SIM
        digitalWrite(DC_PIN, LOW);
        digitalWrite(CS_PIN, LOW);
        SPI_BUS.transfer(cmd);
        digitalWrite(CS_PIN, HIGH);
    #else
        digitalWrite(DC_PIN, LOW);
        digitalWrite(CS_PIN, LOW);
        
        // Manual register transfer
        SPI5_HW->IFCR = 0xFFFFFFFF;
        SPI5_HW->CR2 = 1; // 1 byte
        SPI5_HW->CR1 |= SPI_CR1_CSTART;
        
        while (!(SPI5_HW->SR & SPI_SR_TXP));
        *((volatile uint8_t*)&SPI5_HW->TXDR) = cmd;
        
        while (!(SPI5_HW->SR & SPI_SR_RXP));
        volatile uint8_t dummy = *((volatile uint8_t*)&SPI5_HW->RXDR);
        (void)dummy;
        
        while (!(SPI5_HW->SR & SPI_SR_EOT));
        SPI5_HW->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;
        
        digitalWrite(CS_PIN, HIGH);
    #endif
}

inline void writeData(uint8_t data) {
    #if WOKWI_SIM
        digitalWrite(DC_PIN, HIGH);
        digitalWrite(CS_PIN, LOW);
        SPI_BUS.transfer(data);
        digitalWrite(CS_PIN, HIGH);
    #else
        digitalWrite(DC_PIN, HIGH);
        digitalWrite(CS_PIN, LOW);
        
        SPI5_HW->IFCR = 0xFFFFFFFF;
        SPI5_HW->CR2 = 1;
        SPI5_HW->CR1 |= SPI_CR1_CSTART;
        
        while (!(SPI5_HW->SR & SPI_SR_TXP));
        *((volatile uint8_t*)&SPI5_HW->TXDR) = data;
        
        while (!(SPI5_HW->SR & SPI_SR_RXP));
        volatile uint8_t dummy = *((volatile uint8_t*)&SPI5_HW->RXDR);
        (void)dummy;
        
        while (!(SPI5_HW->SR & SPI_SR_EOT));
        SPI5_HW->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;
        
        digitalWrite(CS_PIN, HIGH);
    #endif
}
// #define writeData println("LINE : ",__LINE__);writeData

#if WOKWI_SIM
    void setWindow(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
        

        int16_t cx1 = max(0, min(240, x1));
        int16_t cy1 = max(0, min(320, y1));
        int16_t cx2 = max(-1, min(239, x2));
        int16_t cy2 = max(-1, min(319, y2));

        #ifndef DISABLE_WARNINGS
            if(cx1 != x1) println(F("WARNING, setWindow clamped x1 ("),x1,F(") to "),cx1);
            if(cy1 != y1) println(F("WARNING, setWindow clamped y1 ("),y1,F(") to "),cy1);
            if(cx2 != x2) println(F("WARNING, setWindow clamped x2 ("),x2,F(") to "),cx2);
            if(cy2 != y2) println(F("WARNING, setWindow clamped y2 ("),y2,F(") to "),cy2);
        #endif

        x1 = cx1;
        y1 = cy1;
        x2 = cx2;
        y2 = cy2;

        if(y2 - y1 + 1 < 0 || x2 - x1 + 1 < 0)
        {
            #ifndef DISABLE_WARNINGS
                Serial.println("WARNING, 0 size setWindow");
            #endif
            
            return;
        }
        
        writeCommand(ILI9341_CASET);
        writeData((uint16_t)cx1 >> 8);
        writeData((uint16_t)cx1 & 0xFF);
        writeData((uint16_t)cx2 >> 8);
        writeData((uint16_t)cx2 & 0xFF);
        
        writeCommand(ILI9341_PASET);
        writeData((uint16_t)cy1 >> 8);
        writeData((uint16_t)cy1 & 0xFF);
        writeData((uint16_t)cy2 >> 8);
        writeData((uint16_t)cy2 & 0xFF);
        
        writeCommand(ILI9341_RAMWR);
    }
#else
    inline void displayFrameBuffer() {
        
        // **DISABLE INTERRUPTS - Critical section**
        // noInterrupts();

        // RESET SPI5 completely before each frame
        SPI5_HW->CR1 &= ~SPI_CR1_SPE; // Disable
        delayMicroseconds(10);
        SPI5_HW->IFCR = 0xFFFFFFFF; // Clear all flags
        SPI5_HW->CR1 |= SPI_CR1_SPE; // Re-enable
        delayMicroseconds(10);
        
        writeCommand(0x2A);
        writeData(0x00); writeData(0x00);
        writeData(0x00); writeData(0xEF);
        
        writeCommand(0x2B);
        writeData(0x00); writeData(0x00);
        writeData(0x01); writeData(0x3F);
        
        writeCommand(0x2C);
        
        digitalWrite(DC_PIN, HIGH);
        digitalWrite(CS_PIN, LOW);
        
        // for(uint32_t i = 0; i < SCREEN_BUFFER_SIZE; i++) {
        //     SPI5_HW->IFCR = 0xFFFFFFFF; // Clear flags EVERY byte
        //     SPI5_HW->CR2 = 1;
        //     SPI5_HW->CR1 |= SPI_CR1_CSTART;
            
        //     while (!(SPI5_HW->SR & SPI_SR_TXP));
        //     *((volatile uint8_t*)&SPI5_HW->TXDR) = screenBuffer[i];
            
        //     while (!(SPI5_HW->SR & SPI_SR_EOT));
        // }

        for(uint32_t i = 0; i < SCREEN_BUFFER_SIZE; i += 2) {
            // Send HIGH byte first, then LOW byte
            uint8_t hi = screenBuffer[i + 1];  // High byte
            uint8_t lo = screenBuffer[i];      // Low byte
            
            // Send HIGH byte
            SPI5_HW->IFCR = 0xFFFFFFFF;
            SPI5_HW->CR2 = 1;
            SPI5_HW->CR1 |= SPI_CR1_CSTART;
            while (!(SPI5_HW->SR & SPI_SR_TXP));
            *((volatile uint8_t*)&SPI5_HW->TXDR) = hi;
            while (!(SPI5_HW->SR & SPI_SR_EOT));
            
            // Send LOW byte
            SPI5_HW->IFCR = 0xFFFFFFFF;
            SPI5_HW->CR2 = 1;
            SPI5_HW->CR1 |= SPI_CR1_CSTART;
            while (!(SPI5_HW->SR & SPI_SR_TXP));
            *((volatile uint8_t*)&SPI5_HW->TXDR) = lo;
            while (!(SPI5_HW->SR & SPI_SR_EOT));
        }
        
        // // **NEW: CRITICAL CLEANUP FOR TOUCH TO WORK**
        // // Wait for the SPI to completely finish
        // // while (SPI5_HW->SR & SPI_SR_BSY);
        // while (SPI5_HW->SR & SPI_SR_EOT) {}  // Wait for End Of Transfer
        // while (!(SPI5_HW->SR & SPI_SR_TXC));  // Wait for TX Complete
                
        // // Clear ALL flags one more time
        // SPI5_HW->IFCR = 0xFFFFFFFF;
        
        // // Deselect display CS
        // digitalWrite(CS_PIN, HIGH);
        
        // // **Give the SPI bus a moment to settle**
        // delayMicroseconds(10);

        // Wait for the SPI to completely finish
        delayMicroseconds(50);  // Give it time to finish

        // Clear ALL flags one more time
        SPI5_HW->IFCR = 0xFFFFFFFF;

        // Deselect display CS
        digitalWrite(CS_PIN, HIGH);

        // Give the SPI bus a moment to settle
        delayMicroseconds(10);

        // **RE-ENABLE INTERRUPTS**
        // interrupts();
    }
#endif

inline Size getTextBounds(const char* text, int16_t fontsize)
{
  unsigned int i = 0;
  int lineLen = 0;
  int maxLineLen = 0;
  int nofnewlines = 1;
  while(text[i] != '\0')
  {
    if(text[i] == '\n')
    {
      nofnewlines += 1;
      lineLen = 0; //reset line length
    }else
    { 
      lineLen += (FONT_WIDTH + 1) * fontsize;//tft.textWidth((const char*)(&text[i]));
    }

    //we get the longest of all of the lines
    if(lineLen > maxLineLen)
    {
      maxLineLen = lineLen;
    }
    

    i++;
  }


  return {(int16_t)(maxLineLen - fontsize), (int16_t)((nofnewlines * (FONT_HEIGHT + 1) - 1) * fontsize)};
}

inline Size getTextBounds(String text, int16_t fontsize)
{
    return getTextBounds(text.c_str(), fontsize);
}

//global::TFT
class TFT 
{
    public:
    int16_t vx = 0;
    int16_t vy = 0;
    int16_t vw = SCREEN_WIDTH;
    int16_t vh = SCREEN_HEIGHT;
    int16_t gx = 0;
    int16_t gy = 0;
    int16_t cursor_x_start = 0;
    int16_t cursor_x = 0;
    int16_t cursor_y = 0;
    int8_t  fontSize = 1;
    uint16_t fontColor = 0xFFFF;


    //TFT::getTextBounds
    Size getTextBounds(const char* text, int16_t w, int16_t h, int16_t fontSize)
    {
        return getTextBounds(text, w, h, fontSize);
    }

    //TFT::begin
    // begin() stays mostly the same but ensure SPE stays on
    void begin()
    {
        #if !WOKWI_SIM
            if(!SDRAM.begin())
            {
                ::println("ERROR, could not initialize the SDRAM");
                while(true);
            }

            Serial.println("Testing SDRAM access...");
            
            screenBuffer = (uint8_t*)SDRAM.malloc(SCREEN_WIDTH * 2 * SCREEN_HEIGHT);

            if(screenBuffer == NULL)
            {
                ::println("ERROR, allocation to SDRAM failed");
                while(true);
            }else
            {
                ::println("SUCCESS, the SDRAM seems to work");
                for(unsigned int i = 0; i < SCREEN_BUFFER_SIZE; i++)
                {
                    screenBuffer[i] = 0;
                }
            }   
        #endif

        pinMode(DC_PIN, OUTPUT);
        pinMode(CS_PIN, OUTPUT);
        pinMode(RST_PIN, OUTPUT);
        
        digitalWrite(CS_PIN, HIGH);
        // digitalWrite(DC_PIN, HIGH);
        
        digitalWrite(RST_PIN, LOW);
        delay(20);
        digitalWrite(RST_PIN, HIGH);
        delay(150);
        
        #if WOKWI_SIM
            ::println("WOKWI_SIM");
            SPI_BUS.setClockDivider(SPI_CLOCK_DIV2);
            SPI_BUS.begin();
        #else
            
            // Let Arduino library init SPI5
            Serial.println("Initializing with SPI1.begin()...");
            SPI1.begin();
            SPI1.beginTransaction(SPISettings(80000000, MSBFIRST, SPI_MODE0)); // FAST!
            
            Serial.println("SPI1.begin() done, checking SPI5 registers...");
            Serial.print("SPI5->CR1:  0x"); Serial.println(SPI5_HW->CR1, HEX);
            Serial.print("SPI5->SR:   0x"); Serial.println(SPI5_HW->SR, HEX);
            Serial.print("SPI5->CFG1: 0x"); Serial.println(SPI5_HW->CFG1, HEX);
            Serial.print("SPI5->CFG2: 0x"); Serial.println(SPI5_HW->CFG2, HEX);
            
            if (!(SPI5_HW->CR1 & SPI_CR1_SPE)) {
                Serial.println("ERROR: SPI5 not enabled!");
                while(1);
            }
            
            Serial.println("SPI5 is ON! Testing register writes...");
        #endif
        
        writeCommand(0xC1);  // PWCTR2
        writeData(0x10);     // SAP[2:0];BT[3:0]
    
        // VCOM control
        writeCommand(0xC5);  // VMCTR1
        writeData(0x3e);
        writeData(0x28);
        
        writeCommand(0xC7);  // VMCTR2
        writeData(0x86);
        
        // Memory access control (rotation)
        writeCommand(0x36);  // MADCTL

        // #if WOKWI_SIM
            writeData(0x48);     // MX, BGR (adjust for your screen orientation)
        // #else
        //     ::println("doing thei thing please work for the fucking love of god ");   
        //     writeData(0x08);
        // #endif
        // Pixel format

        
        writeCommand(0x3A);  // PIXFMT
        writeData(0x55);     // 16-bit color (RGB565)
        
        // Frame rate
        writeCommand(0xB1);  // FRMCTR1
        writeData(0x00);
        writeData(0x18);     // 79Hz
        
        // Display function control
        writeCommand(0xB6);  // DFUNCTR
        writeData(0x08);
        writeData(0x82);
        writeData(0x27);
        
        // Gamma settings (optional but recommended)
        writeCommand(0xE0);  // GMCTRP1 (Positive Gamma)
        writeData(0x0F);
        writeData(0x31);
        writeData(0x2B);
        writeData(0x0C);
        writeData(0x0E);
        writeData(0x08);
        writeData(0x4E);
        writeData(0xF1);
        writeData(0x37);
        writeData(0x07);
        writeData(0x10);
        writeData(0x03);
        writeData(0x0E);
        writeData(0x09);
        writeData(0x00);
        
        writeCommand(0xE1);  // GMCTRN1 (Negative Gamma)
        writeData(0x00);
        writeData(0x0E);
        writeData(0x14);
        writeData(0x03);
        writeData(0x11);
        writeData(0x07);
        writeData(0x31);
        writeData(0xC1);
        writeData(0x48);
        writeData(0x08);
        writeData(0x0F);
        writeData(0x0C);
        writeData(0x31);
        writeData(0x36);
        writeData(0x0F);
        
        // Exit sleep and turn on display
        writeCommand(0x11);  // SLPOUT
        delay(120);
        
        writeCommand(0x29);  // DISPON (Display ON)
        delay(20);
        
        ::println("Display init complete!");
    }

    //TFT::setViewport
    void setViewport(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        w = w + x - 1;
        h = h + y - 1;
    
        x = max( 0, min(240, x));
        y = max( 0, min(320, y));
        w = max(-1, min(239, w));
        h = max(-1, min(319, h));

        w = w - x + 1;
        h = h - y + 1;

        vx = x;
        vy = y;
        vw = w;
        vh = h;
    }

    //TFT::drawPixel
    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        // //update position based on global
        // x += gx;
        // y += gy;

        //if the pixel is not inside of the viewport
        if (x < vx || vx + vw - 1 < x || y < vy || vy + vh - 1 < y) return;

        #if WOKWI_SIM            
            setWindow(x, y, x, y);
            writeData(color >> 8);
            writeData(color & 0xFF);
        #else
            *((uint16_t*)(screenBuffer + ((y) * SCREEN_WIDTH * 2) + ((x) * 2))) = color;
        #endif
    }

    void drawPixelNoCheck(int16_t x, int16_t y, uint16_t color) {
        // //update position to be based on global
        // x += gx;
        // y += gy;

        #if WOKWI_SIM            
            setWindow(x, y, x, y);
            writeData(color >> 8);
            writeData(color & 0xFF);
        #else
            *((uint16_t*)(screenBuffer + (y * SCREEN_WIDTH * 2) + (x * 2))) = color;
        #endif
    }

    //TFT::fillRect
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {

        #if WOKWI_SIM
            //update position to be based on global
            // x += gx;
            // y += gy;
        #endif

        w = w + x - 1;
        h = h + y - 1;
    
        x = max((vx), min((vx + vw), x)); // (vx), (vx + vw)
        y = max((vy), min((vy + vh), y)); // (vy), (vy + vh)
        w = max((vx - 1), min((vx + vw - 1), w)); // (vx - 1), (vx + vw - 1)
        h = max((vy - 1), min((vy + vh - 1), h)); // (vy - 1), (vy + vh - 1)

        w = w - x + 1;
        h = h - y + 1;
        
        //if on mega, then directly transfer the fillRect to the screen
        #if WOKWI_SIM   
            setWindow(x, y, x + w - 1, y + h - 1);
            
            uint32_t totalPixels = (uint32_t)w * (uint32_t)h;
            uint8_t hi = color >> 8;
            uint8_t lo = color & 0xFF;
            
            digitalWrite(CS_PIN, LOW);
            digitalWrite(DC_PIN, HIGH);

            for (uint32_t i = 0; i < totalPixels; i++) {

                SPI_BUS.transfer(hi);
                SPI_BUS.transfer(lo);
            }

            digitalWrite(CS_PIN, HIGH);

        //else, on giga, we have enough space for a screen buffer so write to the screen buffer instead
        #else

            //go through every possibilities of the rectangles
            for(int i = 0 ; i < w; i++)
            {
                for(int j = 0; j < h; j++)
                {

                    drawPixel(x + i, y + j, color);
                }
            }
        #endif
    }

    //TFT::fillScreen
    void fillScreen(uint16_t colour)
    {
        fillRect(vx, vy, vw, vh, colour);
    }

    //TFT::drawRect
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
    {
        //big switch (when the b is x, big switch switches between left and right, when b is y, it switches up and down
        for(int a = 0; a <= 1; a += 1)
        {
            //x and y switch
            for(int b = 0; b <= 1; b += 1)
            {
                // setWindow(x + ((b)?(0):(((a)?(w - 1):(0)))), y + ((b)?(((a)?(h - 1):(0))):(0)), x + ((b)?(w - 1):(((a)?(w - 1):(0)))), y + ((b)?(((a)?(h - 1):(0))):(h - 1)));

                fillRect(x + ((b)?(0):(((a)?(w - 1):(0)))),
                         y + ((b)?(((a)?(h - 1):(0))):(0)),
                         x + ((b)?(w - 1):(((a)?(w - 1):(0)))) - (x + ((b)?(0):(((a)?(w - 1):(0))))) + 1,
                         y + ((b)?(((a)?(h - 1):(0))):(h - 1)) - (y + ((b)?(((a)?(h - 1):(0))):(0))) + 1,
                         colour);

                // uint8_t hi = colour >> 8;
                // uint8_t lo = colour & 0xFF;
                
                // digitalWrite(DC_PIN, HIGH);
                // digitalWrite(CS_PIN, LOW);

                // for (int i = 0; i < ((b)?(w):(h)); i++) {
                //     SPI_BUS.transfer(hi);//BE CAREFULL' WON'T WORK ON GIGA (NO TRANSACTIONS)
                //     SPI_BUS.transfer(lo);
                // }
                // digitalWrite(CS_PIN, HIGH);
            }
        }
    }

    //TFT::drawChar
    void drawChar(char c, int16_t x, int16_t y, int16_t si, uint16_t colour)
    {
        //return if the character is space (we don't need to draw space)
        if(c == ' ' || c == '\t')
        {
            return;
        }

        int charIndex = -1;
        for (int i = 0; (int unsigned)i < strlen(fontChar); i++)
        {
            if(fontChar[i] == c)
            {
                charIndex = i;
                break;
            }
        }

        //si ont a pas trouvé le charactère (c)
        if (charIndex == -1)
        {
            Serial.print("ERROR, couldn't find the character : '"); Serial.print(c); Serial.print("' -> "); Serial.println((uint8_t)c);
            delay(1000);
            charIndex = 0;//put the "unknown" character

            // return;
        }

        //iChar c'est l'index de la lettre. (A,B,C, etc...)
        for (int iChar = charIndex; iChar < 1 + charIndex; iChar ++) //ont rajoute charIndex pour modifier où ont itère (trust sa marche)
        {
            //itération de la position y de la lettre
            for (int posY = 0; posY < 6 * si; posY += si) //ont ajoute la position pour déplacé la lettre
            {
            //itération de la position x de la lettre
            for (int posX = 0; posX < 5 * si; posX += si)
            {
                //ont vas déterminé l'index qu'il faut pour déterminé l'index dans la variables font
                int trueIndex = iChar * 5 * 6 + posY / si * 5 + posX / si;

                if (fontUnpacker(trueIndex))
                {
                    fillRect((uint16_t)(posX + x), (uint16_t)(posY + y), (uint16_t)(si), (uint16_t)(si), colour);
                }
            }
            }
        }
    }

    //TFT::setCursor
    void setCursor(int16_t cursor_x, int16_t cursor_y)
    {
        this->cursor_x  = cursor_x;
        this->cursor_y  = cursor_y;
    }

    //TFT::setCursor
    void setCursor(int16_t cursor_x, int16_t cursor_y, int8_t fontSize, uint16_t fontColor)
    {
        this->cursor_x  = cursor_x;
        this->cursor_y  = cursor_y;
        this->fontSize  = fontSize;
        this->fontColor = fontColor;
    }

    //TFT::setCursor
    void setCursor(int16_t cursor_x, int16_t cursor_y, int8_t fontSize, uint16_t fontColor, int16_t cursor_x_start)
    {
        this->cursor_x = cursor_x;
        this->cursor_y = cursor_y;
        this->fontSize = fontSize;
        this->fontColor = fontColor;
        this->cursor_x_start = cursor_x_start;
    }

    //TFT::print
    void s_print(const char* s)
    {
        if(s == nullptr)
        {
            Serial.print("ERROR line ");Serial.print(__LINE__);Serial.println(" in function print, the string inputed in the function did not point to a valid place in memory (it was a nullptr)");
            return;
        }

        //iterate through all of the string
        int i = 0;
        while(s[i] != '\0')
        {

            if(s[i] == '\n')
            {
                cursor_y += (FONT_HEIGHT + 1) * fontSize;//step the cursor_y down (since new line)
                cursor_x = cursor_x_start;//reset x
                
            }else
            {
                drawChar(s[i], cursor_x, cursor_y, fontSize, fontColor);

                cursor_x += (FONT_WIDTH + 1) * fontSize;//step cursor_x for next character
            }

            i++;
        }
    }

    //TFT::print
    void s_print(String s)
    {
        s_print(s.c_str());
    }

    //TFT::print
    void s_print(char c)
    {
        if(c == '\n')
        {
            cursor_y += (FONT_HEIGHT + 1) * fontSize;//step the cursor_y down (since new line)
            cursor_x = cursor_x_start;//reset x
            
        }else
        {
            //draw the character
            drawChar(c, cursor_x, cursor_y, fontSize, fontColor);

            cursor_x += (FONT_WIDTH + 1) * fontSize;//step cursor_x for next character
        }
    }

    //TFT::print
    void s_print(long long n)
    {

        int nlength = 0;//length of the number
        {
            long long n2 = n;//copy n
            while(0 < n2)
            {
                //everytime that ther is a new number we get rid of it and add to the counter of numbers we have
                n2 /= 10;
                nlength += 1;
            }
        }


        //we will go through all of the digits of the number, starting from the left (the biggest digits, to the smallest)
        while(nlength > 0)
        {

            //convert the number into a character
            char converted = '0' + getDigits(n, nlength);

            //draw that character
            drawChar(converted, cursor_x, cursor_y, fontSize, fontColor);

            //move the cursor to the right
            cursor_x += (FONT_WIDTH + 1) * fontSize;//step cursor_x for next character

            //update our index, which is nlength
            nlength -= 1;
        }

    }

    //TFT::print
    void s_print(unsigned long long n)
    {
        int nlength = 0;//length of the number
        {
            long long n2 = n;//copy n
            while(0 < n2)
            {
                //everytime that ther is a new number we get rid of it and add to the counter of numbers we have
                n2 /= 10;
                nlength += 1;
            }
        }

        //we will go through all of the digits of the number, starting from the left (the biggest digits, to the smallest)
        while(nlength > 0)
        {
            //convert the number into a character
            char converted = '0' +  getDigits(n, nlength);

            //draw that character
            drawChar(converted, cursor_x, cursor_y, fontSize, fontColor);

            //move the cursor to the right
            cursor_x += (FONT_WIDTH + 1) * fontSize;//step cursor_x for next character

            //update our index, which is nlength
            nlength -= 1;
        }
    }

    //TFT::print
    void s_print(long n)
    {
        s_print((long long)(n));
    }

    //TFT::print
    void s_print(unsigned long n)
    {
        s_print((unsigned long long)(n));
    }

    //TFT::print
    void s_print(int n)
    {
        s_print((long long)(n));
    }

    //TFT::print
    void s_print(unsigned int n)
    {
        s_print((unsigned long long)(n));
    }

    //TFT::print
    void s_print(unsigned char n)
    {
        s_print((unsigned long long)(n));
    }

    //TFT::print
    void s_print(double d)
    {

        long long n = d*1000000;//convert double to long long

        int nlength = 0;//length of the number
        {
            long long n2 = n;//copy n
            while(0 < n2)
            {
                //everytime that ther is a new number we get rid of it and add to the counter of numbers we have
                n2 /= 10;
                nlength += 1;
            }
        }

        //we will go through all of the digits of the number, starting from the left (the biggest digits, to the smallest)
        int i = nlength;
        while(6 < i)
        {

            //convert the number into a character
            char converted = '0' +  getDigits(n, i);

            //draw that character
            drawChar(converted, cursor_x, cursor_y, fontSize, fontColor);

            //move the cursor to the right
            cursor_x += (FONT_WIDTH + 1) * fontSize;//step cursor_x for next character

            //update our index, which is i
            i -= 1;
        }

        //draw the middle dot
        drawChar('.', cursor_x, cursor_y, fontSize, fontColor);
        
        //move the cursor to the right
        cursor_x += (FONT_WIDTH + 1) * fontSize;//step cursor_x for next character


        //get rid of the digits before the dot
        n = getDigits(n, 0, 6);

        //update nlength to go reverse so we don't include ending zero digits
        {
            nlength = 0;//reset nlength
            long long n2 = n;//copy n
            while(0 < n2)
            {
                //remove the most significant digit
                n2 = getDigits(n2, 1, 6 - nlength);

                nlength += 1;

                if(6 < nlength - 1)
                {
                    Serial.print("ERROR line ");Serial.print(__LINE__);Serial.println(", nlength hast gone outside of what it should be at");
                    while(true);//hold
                }
            }

            nlength -= 1; //decrease nlength by one because was causing issues (wouldn't get the right size)
        }


        //print all of the last digits after the dot
        for(int i = 0; i < nlength; i++)
        {

            //convert the number into a character
            char converted = '0' + getDigits(n, 6 - i);

            //draw that character
            drawChar(converted, cursor_x, cursor_y, fontSize, fontColor);

            //move the cursor to the right
            cursor_x += (FONT_WIDTH + 1) * fontSize;//step cursor_x for next character
        }

    }

    void print(){}

    template<typename T> 
    void print(T input){s_print(input);}

    template<typename T, typename... Args>
    void print(T input, Args... other){s_print(input); print(other...);}

    void println(){s_print('\n');}

    template<typename T>
    void println(T input){s_print(input); s_print('\n');}

    template<typename T, typename... Args>
    void println(T input, Args... other){s_print(input); println(other...);}

    //TFT::drawText
    void drawText(const char* s, int16_t x, int16_t y, int16_t si, uint16_t colour, bool wrapText)
    {
        int i = 0;
        int displacement = 0;
        while((unsigned int)i < strlen(s))
        {
            char sChar = s[i];
            if(sChar == '\n')
            {
                y += si * 7;//move the text down one line
                displacement = -(i + 1);
                
            }else if(sChar != ' ')//if its not a space
            {
                drawChar(sChar, x + i * 6 * si + displacement * 6 * si, y, si, colour);
            }

            if(sChar != '\n')
            {
                if((SCREEN_WIDTH - 1 < (x + i * 6 * si + displacement * 6 * si) + (FONT_WIDTH + 1) * si) && wrapText)
                {
                    //simulate a new line
                    y += si * 7;//move the text down one line
                    displacement = -(i + 1); 
                }
            }
            
            i++;
        }
    }

    //TFT::drawText
    void drawText(const char* s, int16_t x, int16_t y, int16_t si, uint16_t colour)
    {
        drawText(s, x, y, si, colour, 0);
    }

    //TFT::drawText
    void drawText(String s, int16_t x, int16_t y, int16_t si, uint16_t colour)
    {
        drawText(s.c_str(), x, y ,si, colour, 0);
    }

    //TFT::drawText
    void drawText(String s, int16_t x, int16_t y, int16_t si, uint16_t colour, bool wrapText)
    {
        drawText(s.c_str(), x, y ,si, colour, wrapText);
    }

    //TFT::drawLine
    void drawLine(double x1, double y1, double x2, double y2, uint16_t colour)
    {
        ((x1)>(x2)?(x1):(x2)) += 1;
        ((y1)>(y2)?(y1):(y2)) += 1;

        //get the rule of the line (y = ax + b)
        double a = (y2 - y1) / (x2 - x1);
        double b = y1 - a * x1;

        //get the total width and height for later iteration
        int w = abs((int)(x2 - x1));
        int h = abs((int)(y2 - y1));

        //iterate either over the height (if the line is more horizontal) or wdith (if its more vertical)
        for(int i = 0; i < ((-1 <= a && a <= 1)?(h):(w)); i++)
        {
            uint16_t x3;
            uint16_t y3;
            uint16_t w3;
            uint16_t h3;

            //if line is more horizontal we get the horizontal blocks
            if(-1 <= a && a <= 1)
            {
                double y     = (min((int)y1,(int)y2) + i);
                double x     = (y2 == y1)?(x1):((y - b) / a);
                double nexty = (min((int)y1,(int)y2) + i + 1);
                double nextx = (y2 == y1)?(x1):((nexty - b) / a);

                int width  = (int)ceil(max(nextx, x)) - (int)floor(min(nextx, x));
                int xstart = floor(min(nextx, x));

                // x3 = clamp16(xstart);
                // y3 = clamp16(y);
                // x4 = clamp16(xstart + width);
                // y4 = clamp16(y);
                x3 = clamp16(xstart);
                y3 = clamp16(y);
                w3 = clamp16(width);
                h3 = clamp16(1);

                // nOfPixels = width;
            }else
            {
                double x     = (min((int)x1,(int)x2) + i);
                double y     = (x2 == x1)?(y1):(a * x + b);
                double nextx = (min((int)x1,(int)x2) + i + 1);
                double nexty = (x2 == x1)?(y1):(a * nextx + b);

                int height  = (int)ceil(max(nexty, y)) - (int)floor(min(nexty, y));
                int ystart = floor(min(nexty, y));

                // x3 = clamp16(x);
                // y3 = clamp16(ystart);
                // x4 = clamp16(x);
                // y4 = clamp16(ystart + height);
                x3 = clamp16(x);
                y3 = clamp16(ystart);
                w3 = clamp16(1);
                h3 = clamp16(height);

                // nOfPixels = height;
            }


            fillRect(x3, y3, w3, h3, colour);
            // setWindow(x3, y3, x4, y4);

            // uint8_t hi = colour >> 8;
            // uint8_t lo = colour &  0xFF;

            // digitalWrite(DC_PIN, HIGH);
            // digitalWrite(CS_PIN, LOW);

            // for(int k = 0; k < nOfPixels; k++)
            // {
            //     SPI_BUS.transfer(hi);//WON'T WORK ON GIGA, NO TRANSACTIONS
            //     SPI_BUS.transfer(lo);
            // }
            
            // digitalWrite(CS_PIN, HIGH);
        }

    }

    //TFT::drawOctantOfCircle
    void drawOctantOfCircle(int octant, double x, double y, double r, uint16_t colour)
    {
        if(0 <= octant && octant <= 7)
        {
            bool b  = !((bool)(octant & 0b00000010));
            int  c  = (b != ((bool)(octant & 0b00000100))) * 2 - 1;
            int  a  = ((bool)(octant & 0b00000100) != (bool)(octant & 0b00000001)) * 2 - 1;

            //a is for small switch
            //b is for switching x and y axis
            //c is for big switch

            double distance = (r / sqrt(2.0));
            double s = r - distance;

            for(int i = 0; i < (int)ceil(s) + 1; i++)
            {
                // uint16_t x1;
                // uint16_t y1;
                // uint16_t x2;
                // uint16_t y2;
                int nOfPixels;

                if(b)
                {
                    double ya     = y + c * (-r + i - 0.5);
                    double xa     = a * sqrt(pow(r, 2.0) - pow(ya - y, 2.0)) + x;
                    double nextya = y + c * (-r + i + 0.5);
                    double nextxa = a * sqrt(pow(r, 2.0) - pow(nextya - y, 2.0)) + x;

                    //check for nan 
                    if(xa != xa)
                    {
                        xa = x;
                    }

                    //check for nan
                    if(nextxa != nextxa)
                    {
                        nextxa = x;
                    }

                    println1(ya, xa, nextya, nextxa);

                    int xstart = floor(min(nextxa, xa));
                    int width  = ceil(max(nextxa, xa)) - xstart;

                    // x1 = clamp16(xstart);
                    // y1 = clamp16(ya);
                    // x2 = clamp16(xstart + width);
                    // y2 = clamp16(ya);

                    nOfPixels = width;
                }else
                {
                    double xa     = x + c * (-r + i);
                    double ya     = a * sqrt(pow(r, 2.0) - pow(xa - x, 2.0)) + y;
                    double nextxa = x + c * (-r + i + 1);
                    double nextya = a * sqrt(pow(r, 2.0) - pow(nextxa - x, 2.0)) + y;

                    //check for nan
                    if(ya != ya)
                    {
                        ya = 0.0;
                    }

                    //check for nan
                    if(nextya != nextya)
                    {
                        nextya = 0.0;
                    }

                    int ystart = floor(min(nextya, ya));
                    int height = ceil(max(nextya, ya)) - ystart;

                    // x1 = clamp16(xa);// - ((c<0)?(1):(0)));
                    // y1 = clamp16(ystart);
                    // x2 = clamp16(xa);
                    // y2 = clamp16(ystart + height);

                    nOfPixels = height;
                }

                #if !WOKWI_SIM
                    SPI_BUS.beginTransaction(SPISettings(SPI_SETTINGS, MSBFIRST, SPI_MODE0));
                #endif

                digitalWrite(CS_PIN, LOW);

                // setWindow(x1, y1, x2, y2);

                uint8_t hi = colour >> 8;
                uint8_t lo = colour &  0xFF;

                digitalWrite(DC_PIN, HIGH);

                for(int k = 0; k < nOfPixels; k++)
                {

                    SPI_BUS.transfer(hi);
                    SPI_BUS.transfer(lo);

                }
                
                digitalWrite(CS_PIN, HIGH);

                #if !WOKWI_SIM
                    SPI_BUS.endTransaction();
                #endif
            }
        }else
        {
            ::println("ERROR, drawOctantCircle recieved '",octant,"' but the range of possible octants are from 0 to 7 (this makes for 8 possiblilities because there are 8 octants in a circle)");
        }
    }

    //TFT::drawCircle
    void drawCircle(double x, double y, double r, uint16_t colour)
    {
        ::println("ACCESS DENIED");
        return;

        //normal  : (sqrt(pow(r, 2.0) - pow(x - a, 2.0)) + b );
        //inverse : sqrt(pow(r, 2.0) - pow(y - b, 2.0)) + a ;

        //draw circle in 8 parts
        for(int i = 0; i < 8; i++)
        {
            drawOctantOfCircle(i, x, y, r, colour);
        }
    }

    //TFT::fillCircle
    void fillCircle(double x, double y, double r, uint16_t colour)
    {
        int totalheight = (int)ceil(y + r) - (int)floor(y - r);
        for(int i = 0; i <= totalheight; i++)
        {
            //normal  : (sqrt(pow(r, 2.0) - pow(x - a, 2.0)) + b );
            //inverse : sqrt(pow(r, 2.0) - pow(y - b, 2.0)) + a ;

            int startY = ((int)floor(y - r) + i);
            double funcStart1 = sqrt(pow(r, 2.0) - pow(((y - r + i) - 0.5) - y, 2.0));
            double funcStart2 = sqrt(pow(r, 2.0) - pow(((y - r + i) + 0.5) - y, 2.0));

            //check if funcStart1 is Nan and set it to 0 if it is
            if(funcStart1 != funcStart1)
            {
                funcStart1 = 0.0;
            }

            //check if funcStart2 is Nan and set it to 0 if it is
            if(funcStart2 != funcStart2)
            {
                funcStart2 = 0.0;
            }
            
            int startX = floor(min(x - funcStart1, x - funcStart2));
            int width  = ceil(max(x + funcStart1, x + funcStart2)) - startX + 1;

            fillRect(clamp16(startX), clamp16(startY), clamp16(width), clamp16(startY), colour);
        }
    }
};

inline TFT tft;

inline void SerialBegin()
{
    Serial.begin(9600);
    while(!Serial);
    delay(500);
    Serial.println("Starting...");

    tft.begin();
    tft.fillScreen(0x0);
}

#endif