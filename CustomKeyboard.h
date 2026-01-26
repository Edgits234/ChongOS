#ifndef CustomKeyboard_h
#define CustomKeyboard_h

#if !defined(ENGLISH_CANADIAN_KEYBOARD) && !defined(FRENCH_CANADIAN_KEYBOARD)
    #define FRENCH_CANADIAN_KEYBOARD
#endif

//============= CUSTOM KEYBOARD CLASS ===============//
#include <USBHostGiga.h>//this is for the keyboard

Keyboard keyb;

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
    int16_t cursorIndex = 0;//imagine an invisible cursor, the code will try and modify the text there instead of always at the end
    uint8_t delayLimiter = 0;//are we repeating like letters when writting text (1) or are we most rapid fire ever (0), (2) for no repeat at all

    void begin()
    {
        keyb.begin();
    }
    
    size_t available()
    {
        if(Available == true)
        {
            Available -= 1;
            return Available + 1;
        }else
        {
            return Available;
        }
    }

    void testUpdate()
    {

        // Available = keyb.available();
        if(keyb.available())
        {
            HID_KEYBD_Info_TypeDef key = keyb.read();
            keys = key;
            // printlnArray(key.keys); // can't use that

            return;
        }
    }


    void testKeys(HID_KEYBD_Info_TypeDef key)
    {
        //if no special keys were pressed
        if(key.lalt == 0 && key.lctrl == 0 && key.lshift == 0)
        {

            if(currKey == KEY_UPARROW)
            {
                //We do nothing yet (will have to do some complex as balls algorithm to make cursor go up and down)
            
                return;//return so it doesn't place the last character
            }else if(currKey == KEY_DOWNARROW)
            {
                //We do nothing yet (will have to do some complex as balls algorithm to make cursor go up and down)

                return;//return so it doesn't place the last character
            }else if(currKey == KEY_LEFTARROW)
            {
                //don't subtract to the cursor Index if its at the begining of the text (Bound check)
                if(0 < cursorIndex)
                {
                    cursorIndex -= 1;
                }

                return;//return so it doesn't place the last character
            }else if(currKey == KEY_RIGHTARROW)
            {
                //don't add to the cursor Index if its at the end of the buffer
                if(cursorIndex <= (int)buffer.length() - 1)
                {
                    cursorIndex += 1;
                }

                return;//return so it doesn't place the last character
            }
        
        }
        
        if(key.lalt == 0 && key.lctrl == 0)
        {
            
            //now we update the buffer
            if(4 <= currKey && currKey <= 29)
            {
                currChar = (char)(currKey + 93);

                if(key.lshift == 1)
                {
                    currChar = (char)(currChar - 32);
                }

            //if its a number
            }else if(KEY_1_EXCLAMATION_MARK <= currKey && currKey <= KEY_0_CPARENTHESIS && key.lshift == 0)
            {

                currChar = (char)(((currKey - 29) % 10) + 48);

            }else if(currKey == KEY_0_CPARENTHESIS     && key.lshift == 1)
            {
                currChar = ')';
            }else if(currKey == KEY_1_EXCLAMATION_MARK && key.lshift == 1)
            {
                currChar = '!';
            }else if(currKey == KEY_2_AT               && key.lshift == 1)
            {
                #if defined(FRENCH_CANADIAN_KEYBOARD)
                    currChar = '\"';
                #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                    currChar = '@';
                #endif
            }else if(currKey == KEY_3_NUMBER_SIGN      && key.lshift == 1)
            {
                #if defined(FRENCH_CANADIAN_KEYBOARD)
                    currChar = '/';
                #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                    currChar = '#';
                #endif
            }else if(currKey == KEY_4_DOLLAR           && key.lshift == 1)
            {
                currChar = '$';
            }else if(currKey == KEY_5_PERCENT          && key.lshift == 1)
            {
                currChar = '%';
            }else if(currKey == KEY_6_CARET            && key.lshift == 1)
            {
                #if defined(FRENCH_CANADIAN_KEYBOARD)
                    currChar = '?';
                #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                    currChar = '^';
                #endif
            }else if(currKey == KEY_7_AMPERSAND        && key.lshift == 1)
            {
                currChar = '&';
            }else if(currKey == KEY_8_ASTERISK         && key.lshift == 1)
            {
                currChar = '*';
            }else if(currKey == KEY_9_OPARENTHESIS     && key.lshift == 1)
            {
                currChar = '(';
            }else if(currKey == KEY_SLASH_QUESTION)
            {
                #if defined(FRENCH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        // currChar = 'é';// weird character fix custom graphics
                    }else if(key.lshift == 1)
                    {
                        // currChar = 'É';// weird character fix custom graphics
                    }
                #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        currChar = '/';
                    }else if(key.lshift == 1)
                    {
                        currChar = '?';
                    }
                #endif
            }else if(currKey == KEY_SINGLE_AND_DOUBLE_QUOTE)
            {
                #if defined(FRENCH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        currChar = '`';//weird character 
                    }else if(key.lshift == 1)
                    {
                        currChar = '`';//weird character
                    }
                #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        currChar = '\'';
                    }else if(key.lshift == 1)
                    {
                        currChar = '\"';
                    }
                #endif
            }else if(currKey == KEY_BACKSLASH_VERTICAL_BAR)
            {
                #if defined(FRENCH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        currChar = '<';
                    }else if(key.lshift == 1)
                    {
                        currChar = '>';
                    }
                #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        currChar = '\\';
                    }else if(key.lshift == 1)
                    {
                        currChar = '|';
                    }
                #endif
            }else if(currKey == KEY_GRAVE_ACCENT_AND_TILDE)
            {
                #if defined(FRENCH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        currChar = '#';
                    }else if(key.lshift == 1)
                    {
                        currChar = '|';
                    }
                #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        currChar = '`';
                    }else if(key.lshift == 1)
                    {
                        currChar = '~';
                    }
                #endif
            }else if(currKey == KEY_MINUS_UNDERSCORE)
            {
                if(key.lshift == 0)
                {
                    currChar = '-';
                }else if(key.lshift == 1)
                {
                    currChar = '_';
                }
            }else if(currKey == KEY_EQUAL_PLUS)
            {
                if(key.lshift == 0)
                {
                    currChar = '=';
                }else if(key.lshift == 1)
                {
                    currChar = '+';
                }
            }else if(currKey == KEY_SEMICOLON_COLON)
            {
                if(key.lshift == 0)
                {
                    currChar = ';';
                }else if(key.lshift == 1)
                {
                    currChar = ':';
                }
            }else if(currKey == KEY_OBRACKET_AND_OBRACE)
            {
                #if defined(FRENCH_CANADIAN_KEYBOARD)
                    currChar = '^';//weird character
                #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        currChar = '[';
                    }else if(key.lshift == 1)
                    {
                        currChar = '{';
                    }
                #endif
            }else if(currKey == KEY_CBRACKET_AND_CBRACE)
            {
                #if defined(FRENCH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        // currChar = '¸';//weird character
                    }else if(key.lshift == 1)
                    {
                        // currChar = '¨';//weird character
                    }
                #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                    if(key.lshift == 0)
                    {
                        currChar = ']';
                    }else if(key.lshift == 1)
                    {
                        currChar = '}';
                    }
                #endif

            //if we just pressed backspace
            }else if(currKey == KEY_BACKSPACE)
            {

                //we don't need to do anything if the string is already length 0
                if(cursorIndex != 0)
                {
                    //remove the character just behind the cursorIndex
                    buffer = buffer.substring(0, cursorIndex - 1) + buffer.substring(cursorIndex);

                    cursorIndex -= 1;
                }



                return;

                

            //if the current key is enter
            }else if(currKey == KEY_ENTER)
            {
                currChar = '\n';

            }else if(currKey == KEY_SPACEBAR)
            {
                currChar = ' ';

            }else if(currKey == KEY_DOT_GREATER)
            {
                if(key.lshift == 0)
                {
                    currChar = '.';
                }else if(key.lshift == 1)
                {
                    #if defined(FRENCH_CANADIAN_KEYBOARD)
                        currChar = '.';
                    #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                        currChar = '>';
                    #endif
                }
            }else if(currKey == KEY_COMMA_AND_LESS)
            {
                if(key.lshift == 0)
                {
                    currChar = ',';
                }else if(key.lshift == 1)
                {
                    #if defined(FRENCH_CANADIAN_KEYBOARD)
                        currChar = '\'';
                    #elif defined(ENGLISH_CANADIAN_KEYBOARD)
                        currChar = '<';
                    #endif
                }
            }else
            {
                //we didn't find conclusive character
                return;
            }

        
        }else if(key.lalt == 0 && key.lctrl == 1 && key.lshift == 0)
        {

            if(currKey == KEY_BACKSPACE)
            {
                
                //we don't need to do anything if the cursorrIndex is already 0
                if(cursorIndex != 0)
                {
                    //delete all of the spaces and new lines until you hit a letter
                    while(buffer[cursorIndex - 1] == ' ' || buffer[cursorIndex - 1] == '\n')
                    {
                        
                        //remove the character just behind the cursorIndex
                        buffer = buffer.substring(0, cursorIndex - 1) + buffer.substring(cursorIndex);

                        cursorIndex -= 1;

                        //if we depleated all the string, skip the next check (it'll break everything)
                        if(cursorIndex <= 0)
                        {
                            return; //no point in continuing
                        }
                    }

                    //delete all of the letters until you hit a space or a new line again
                    while(buffer[cursorIndex - 1] != ' ' && buffer[cursorIndex - 1] != '\n')
                    {
                        
                        //remove the character just behind the cursorIndex
                        buffer = buffer.substring(0, cursorIndex - 1) + buffer.substring(cursorIndex);

                        cursorIndex -= 1;

                        //if we depleated all the string, skip the next check (it'll break everything)
                        if(cursorIndex <= 0)
                        {
                            return; //no point in continuing
                        }
                    }
                }

                return;
            }else if(currKey == KEY_UPARROW)
            {
                //We do nothing yet (will have to do some complex as balls algorithm to make cursor go up and down)
            
                return;//return so it doesn't place the last character
            }else if(currKey == KEY_DOWNARROW)
            {
                //We do nothing yet (will have to do some complex as balls algorithm to make cursor go up and down)

                return;//return so it doesn't place the last character
            }else if(currKey == KEY_LEFTARROW)
            {
                if(0 < cursorIndex)
                {
                    //go past ONE of the new lines if there are any
                    if(buffer[cursorIndex - 1] == '\n')
                    {
                    
                        //don't subtract to the cursor Index if its at the begining of the text (Bound check)
                        if(0 < cursorIndex)
                        {
                            cursorIndex -= 1;

                        //else if its out of bounds
                        }else
                        {
                            return; //no point in continuing
                        }
                    }

                    //go past all of the spaces
                    while(buffer[cursorIndex - 1] == ' ')
                    {

                        //don't subtract to the cursor Index if its at the begining of the text (Bound check)
                        if(0 < cursorIndex)
                        {
                            cursorIndex -= 1;

                        //else if its out of bounds
                        }else
                        {
                            return;//return we are at the end, no point in continuing
                        }
                    }

                    //go past everything that is not a new line or a space
                    while(buffer[cursorIndex - 1] != ' ' && buffer[cursorIndex - 1] != '\n')
                    {

                        //don't subtract to the cursor Index if its at the begining of the text (Bound check)
                        if(0 < cursorIndex)
                        {
                            cursorIndex -= 1;

                        //else if its out of bounds
                        }else
                        {
                            return;//return we are at the end, no point in continuing
                        }
                    }

                }
              

                return;//return so it doesn't place the last character
            }else if(currKey == KEY_RIGHTARROW)
            {
                if(cursorIndex <= (int)buffer.length() - 1)
                {
                    //go past ONE of the new lines if there are any
                    if(buffer[cursorIndex] == '\n')
                    {
                    
                        //don't subtract to the cursor Index if its at the end of the text (Bound check)
                        if(cursorIndex <= (int)buffer.length() - 1)
                        {
                            cursorIndex += 1;

                        //else if its out of bounds
                        }else
                        {
                            return; //no point in continuing
                        }
                    }

                    //go past all of the spaces
                    while(buffer[cursorIndex] == ' ')
                    {

                        //don't subtract to the cursor Index if its at the end of the text (Bound check)
                        if(cursorIndex <= (int)buffer.length() - 1)
                        {
                            cursorIndex += 1;

                        //else if its out of bounds
                        }else
                        {
                            return;//return we are at the end, no point in continuing
                        }
                    }

                    //go past everything that is not a new line or a space
                    while(buffer[cursorIndex] != ' ' && buffer[cursorIndex] != '\n')
                    {

                        //don't subtract to the cursor Index if its at the end of the text (Bound check)
                        if(cursorIndex <= (int)buffer.length() - 1)
                        {
                            cursorIndex += 1;

                        //else if its out of bounds
                        }else
                        {
                            return;//return we are at the end, no point in continuing
                        }
                    }

                }

                return;//return so it doesn't place the last character
            }else
            {
                return;
            }

            return;
        }else
        {
            return;
        }

        //if the key is undefined or unusable don't do shit (the 0 <= currKey &&  is in our hearts)
        if(currKey <= 3)
        {
            return;
        }

        //add the new character just before the cursor
        buffer = buffer.substring(0, cursorIndex) + currChar + buffer.substring(cursorIndex);

        //also add to the cursorIndex (as we just added a character, its only fitting that we move the cursor to follow after what we typed)
        cursorIndex += 1;
    }

    //simple function that checks if you are currently holding the specific key
    bool checkForKey(uint8_t key)
    {
        for(int i = 0; i < 6; i++)
        {
            if(key == keys.keys[i])
            {
                return true;
            }
        }

        return false;
    }

    bool keysClear()
    {
        for(int i = 0; i < 6; i++)
        {
            if(keys.keys[i] != 0)
            {
                return false;
            }
        }

        return true;
    }

    void update()
    {
        //fix the cursor if fixable (bring to the nearest good spot)
        if(cursorIndex < 0)
        {
            cursorIndex = 0;
        
        //check if out of bounds but in the bigger side (and also if the cursorIsn't at max number (kind of like a "don't do shit with the cursor"))
        }else if((int)buffer.length() < cursorIndex && cursorIndex != INT16_MAX)
        {
            cursorIndex = buffer.length();
        }

        Available = keyb.available();
        if(Available)
        {
            HID_KEYBD_Info_TypeDef key = keyb.read();
            keys = key;
            // printlnArray(key.keys); // can't use that

            // return;

            //go through all of the keys in the key array
            for(int i = 0; i < 6; i++)
            {
                bool success = true;

                //check if any of the keys in previous keys match the current key from key array
                for(int j = 0; j < 6; j++)
                {
                    //if they match, then make success false (is it the same, did not change)
                    if(key.keys[i] == prevKeys[j])
                    {
                        success = false;
                        break;
                    }
                }

                //if we have a key that didn't match any of the previous one, IT MUST BE A NEW KEY, so we set currKey to the NEW KEY
                if(success)
                {
                    lastPress = millis();
                    // println("got a new key : ",key.keys[i]); // can't use that
                    lastKey = currKey;
                    currKey = key.keys[i];
                    break;
                }

                //if we reached the end and didn't find conclusive character, then we return and don't do shit
                if(i == 5)
                {
                    //don't forget to update the previous keys
                    for(int i = 0; i < 6; i++)
                    {
                        prevKeys[i] = key.keys[i];
                    }

                    lastKey = currKey;
                    currKey = 0;
                    return;
                }
            }

            //don't forget to update the previous keys
            for(int i = 0; i < 6; i++)
            {
                prevKeys[i] = key.keys[i];
            }

            if(enableBuffer)
            {
                testKeys(key);
            }
        }
    
        //first / initial delay
        int delayms = 500;
        
        if(delayLimiter == 0)
        {
            delayms = 30;
        }
        
        //keyboard lib doesn't do repeat by default so we check if no change has been made on the keys and repeat
        //we first check if the last change / (last key press) was delayms which changes based on delayLimter this is the initial delay (for text 500 ms)
        //we then pass the initial delay for good and we then go on the the lastRepeat delay, this one has a shorter fixed, 30 ms cooldown
        if(Available == 0 && millis() - lastPress > delayms && millis() - lastRepeat > 30 && currKey != 0 && delayLimiter != 2)
        {
            // println("REPEATING");

            lastRepeat = millis();

            if(enableBuffer)
            {
                testKeys(keys);
            }

            Available += 1;
        }
    }

};

KeyboardManager kbd;

//============= CUSTOM KEYBOARD CLASS ===============//

#endif