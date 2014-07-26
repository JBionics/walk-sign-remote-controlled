// This #include statement was automatically added by the Spark IDE.
#include "MCP23008-I2C/MCP23008-I2C.h"

#include "charMapping.h"

// Make sure to include the MCP23008-I2C library
//
// Connect pin #1 of the expander to D1 (i2c clock)
// Connect pin #1 to 5V (using a pullup resistor ~4K7 (4700) ohms)
// Connect pin #2 of the expander to D0 (i2c data)
// Connect pin #2 to 5V (using a pullup resistor ~4K7 (4700) ohms)
// Connect pins #3, 4 and 5 of the expander to ground (address selection)
// Connect pin #6 and 18 of the expander to 5V (power and reset disable)
// Connect pin #9 of the expander to ground (common ground)
// Pins 10 thru 17 are your input/output pins

Adafruit_MCP23008 digitLeft;
Adafruit_MCP23008 digitRight;

#define CLEAR_DIGIT     (0x20) // ASCII for space
#define DIGIT_LEFT      (0)
#define DIGIT_RIGHT     (1)

#define WALK     (1)
#define HAND     (2)

#define SCREEN_SAVER_KICK_IN_SECONDS    (30)

unsigned long lastUserCommand = 0;
bool demoRunning = false;
  
void setup() {  
    // Setup MCP ICs
    digitLeft.begin(0); // Addr pins -> 0
    digitRight.begin(1); // Addr pins -> 1
    
    // Configure IO expanders: all outputs
    for (int i = 0; i < 8; i++) {
        digitLeft.pinMode(i, OUTPUT);
        digitRight.pinMode(i, OUTPUT);
    }

    // Function made available to the cloud service for interacting with the sign
    Spark.function("signCommand", signCommand);
}

bool recentUserCommand()
{
    return (millis() - lastUserCommand < (SCREEN_SAVER_KICK_IN_SECONDS * 1000));
}

void loop() 
{
    // If no web commands from a user, show the "screensaver"
    while (!recentUserCommand()) {
        
        clearDisplay();
        showIcon(WALK);
        while (1) {
            demoRunning = true;
            // Show Numbers
            for (int i = 0; i < 99; i++) {
                if (recentUserCommand()) {
                    demoRunning = false;
                    break;
                }
                setDisplay(i);
                delay(250);                
            }        
            if (!demoRunning) break;
        }
    }
}

void clearDisplay()
{
    showIcon(CLEAR_DIGIT);
    displayChar(CLEAR_DIGIT, DIGIT_LEFT);
    displayChar(CLEAR_DIGIT, DIGIT_RIGHT);
}

// Receives a string command from the spark cloud
// First character indicates the command type, and optionally a string follows it
int signCommand(String command) {
    if (command.length() == 0) return -1;
    
    // Trim extra spaces
    command.trim();

    // Convert it to upper-case for easier matching
    command.toUpperCase();

    lastUserCommand = millis();
    
    if (demoRunning) {
        clearDisplay();
    }
    
    switch (command[0]) {
        case 'W':
            showIcon(WALK);
            break;
        case 'H':
            showIcon(HAND);
            break;
        case 'C':
            showIcon(CLEAR_DIGIT);
            break;
        case 'S':
            if (command.length() < 2) return -1;
            scrollMessage(&command[1], 500);
            break;
        case 'F':
            if (command.length() < 2) return -1;
            flashMessage(&command[1], 500, DIGIT_RIGHT);
            break;
        case 'D':
            if (command.length() < 2) return -1;
            staticDisplay(&command[1]);
            break;
    };
    
    return 0;
}

// Show hand or walk signal
void showIcon(uint8_t icon)
{
    switch (icon) {
        case HAND:
            digitRight.digitalWrite(7, LOW);
            digitLeft.digitalWrite(7, HIGH);
            break;
        case WALK:
            digitLeft.digitalWrite(7, LOW);
            digitRight.digitalWrite(7, HIGH);
            break;
        default:
            digitLeft.digitalWrite(7, LOW);
            digitRight.digitalWrite(7, LOW);
            break;
    };
}

// Show a 1 or 2 digit number of the display
void setDisplay(uint8_t num)
{
    displayDigit(num % 10, DIGIT_RIGHT);
    num = num / 10;
    
    // Don't dipslay left digit if it's a zero
    if (num % 10 != 0) {
        displayDigit(num % 10, DIGIT_LEFT);
    } else {
        displayDigit(CLEAR_DIGIT, DIGIT_LEFT);
    }
}

// Character to bit mapping representation
uint8_t charMap(char c)
{
    uint8_t mapping = 0;
    if (c == CLEAR_DIGIT) {
        mapping = 0; // redundant, included for clarity
    } else if (c >= 'A' && c <= 'Z') {
        mapping = charMapping[c - 'A'];
    } else if (c >= '0' && c <= '9') {
        mapping = digitMap[c - '0'];
    }
    return mapping;
}

// Show a character on the display
void displayChar(char c, uint8_t digitPos)
{
    uint8_t mapping = charMap(c);
    
    Adafruit_MCP23008& mcp = (digitPos == DIGIT_LEFT) ? digitLeft : digitRight;
    
    for (int i = 6; i >= 0; i--) {
        mapping = mapping >> 1;
        if (mapping & 1 && c != CLEAR_DIGIT) {
            mcp.digitalWrite(i, HIGH);
        } else {
            mcp.digitalWrite(i, LOW);
        }
    }    
}

// Static 1 or 2 character message
void staticDisplay(char* str)
{
    int len = strlen(str);
    if (len == 0) return;
    
    displayChar(str[0], DIGIT_LEFT);
    
    displayChar(str[0], DIGIT_LEFT);
    if (len > 1) {
        displayChar(str[1], DIGIT_RIGHT);
    } else {
        displayChar(CLEAR_DIGIT, DIGIT_RIGHT);
    }
}

// Flash a message
void flashMessage(char* str, int delayMs, uint8_t digitPos)
{
    int len = strlen(str);
    if (len == 0) return;
    
    // Clear digit not flashing the message
    displayChar(CLEAR_DIGIT, digitPos == DIGIT_LEFT ? DIGIT_RIGHT : DIGIT_LEFT);
    
    for (int i = 0; i < len; i++) {
        displayChar(str[i], digitPos);
        delay(delayMs);
    }
    
    displayChar(CLEAR_DIGIT, digitPos);
}

// Scroll a message
void scrollMessage(char* str, int delayMs)
{
    int len = strlen(str);
    if (len == 0) return;
    
    // First character on right digit
    displayChar(CLEAR_DIGIT, DIGIT_LEFT);
    displayChar(str[0], DIGIT_RIGHT);
    delay(delayMs);
    
    // Scroll message
    for (int i = 1; i < len; i++) {
        displayChar(str[i - 1], DIGIT_LEFT);
        displayChar(str[i], DIGIT_RIGHT);
        delay(delayMs);
    }
    
    // Display last character on left digit, then clear display
    displayChar(CLEAR_DIGIT, DIGIT_RIGHT);
    displayChar(str[len - 1], DIGIT_LEFT);
    delay(delayMs);
    displayChar(CLEAR_DIGIT, DIGIT_LEFT);
}

// Put a numerical digit on the display
void displayDigit(uint8_t d, uint8_t digitPos)
{
    if (d > 9 && d != CLEAR_DIGIT) return;
    
    uint8_t mapping = digitMap[d];
    
    Adafruit_MCP23008& mcp = (digitPos == DIGIT_LEFT) ? digitLeft : digitRight;
    
    for (int i = 6; i >= 0; i--) {
        mapping = mapping >> 1;
        if (mapping & 1 && d != CLEAR_DIGIT) {
            mcp.digitalWrite(i, HIGH);
        } else {
            mcp.digitalWrite(i, LOW);
        }
    }
}
