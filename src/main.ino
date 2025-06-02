#define DECODE_NEC
#define IR_RECEIVE_PIN 2
#define EXCLUDE_UNIVERSAL_PROTOCOLS // Saves up to 1000 bytes program space

#include <Adafruit_LiquidCrystal.h>
#include <IRremote.h>

// System states
enum SystemState {
  SYSTEM_OFF = 0,
  SYSTEM_ON = 1
};

// IR Remote button codes
const int POWER_BUTTON = 0;

// Display pins
const int SEGMENT_PINS[] = {4, 3, 9, 8, 7, 5, 6}; // A, B, C, D, E, F, G
const int SEGMENT_A = 0, SEGMENT_B = 1, SEGMENT_C = 2, SEGMENT_D = 3;
const int SEGMENT_E = 4, SEGMENT_F = 5, SEGMENT_G = 6;

// Timing constants
const unsigned long DEBOUNCE_DELAY = 500; // 500ms debounce
const int LOOP_DELAY = 10;

// LCD setup
Adafruit_LiquidCrystal lcd_1(0);
const int LCD_WIDTH = 16;
const int LCD_HEIGHT = 2;

// Global variables
SystemState currentState = SYSTEM_OFF;
int lastButtonPressed = -1;
unsigned long lastButtonTime = 0;
char userInputBuffer[LCD_WIDTH + 1] = ""; // +1 for null terminator

// 7-segment display patterns (A-G segments)
const bool DIGIT_PATTERNS[10][7] = {
  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}  // 9
};

// Function prototypes
int mapCodeToButton(unsigned long code);
int readInfrared();
void initializeSegmentDisplay();
void clearSegmentDisplay();
void displayDigitOnSegments(int digit);
void updateLCDSystemState();
void clearLCDInputLine();
void processCharacterInput(char c);
void processLetterInput(char letter);
void processNumberInput(char number);
void handleSystemOff(int button);
void handleSystemOn(int button);

void setup() {
  initializeSegmentDisplay();
  
  IrReceiver.begin(IR_RECEIVE_PIN);
  lcd_1.begin(LCD_WIDTH, LCD_HEIGHT);
  Serial.begin(9600);
  
  updateLCDSystemState();
}

void loop() {
  int button = readInfrared();
  
  // Handle button press with debouncing
  if (isValidButtonPress(button)) {
    updateLastButton(button);
    
    if (currentState == SYSTEM_OFF) {
      handleSystemOff(button);
    } else if (currentState == SYSTEM_ON) {
      handleSystemOn(button);
    }
  }
  
  // Process serial input only when system is on
  if (currentState == SYSTEM_ON) {
    processSerialInput();
  }
  
  delay(LOOP_DELAY);
}

bool isValidButtonPress(int button) {
  return (button != lastButtonPressed && 
          (millis() - lastButtonTime > DEBOUNCE_DELAY));
}

void updateLastButton(int button) {
  lastButtonPressed = button;
  lastButtonTime = millis();
}

int mapCodeToButton(unsigned long code) {
  // Map the IR code to the corresponding remote button.
  // The buttons are in this order on the remote:
  //    0   1   2
  //    4   5   6
  //    8   9  10
  //   12  13  14
  //   16  17  18
  //   20  21  22
  //   24  25  26
  //
  // Return -1, if supplied code does not map to a key.
  // For the remote used in the Tinkercad simulator,
  // the buttons are encoded such that the hex code
  // received is of the format: 0xiivvBF00
  // Where the vv is the button value, and ii is
  // the bit-inverse of vv.
  
  if ((code & 0x0000FFFF) == 0x0000BF00) {
    code >>= 16;
    if (((code >> 8) ^ (code & 0x00FF)) == 0x00FF) {
      return code & 0xFF;
    }
  }
  return -1;
}

int readInfrared() {
  if (!IrReceiver.decode()) {
    return -1;
  }
  
  unsigned long code = IrReceiver.decodedIRData.decodedRawData;
  
  // Ignore NEC repeat codes
  if (code == 0xFFFFFFFF || code == 0x00000000) {
    IrReceiver.resume();
    return -1;
  }
  
  int button = mapCodeToButton(code);
  
  if (button == -1) {
    Serial.print("Código IR desconhecido: ");
    Serial.println(code, HEX);
  } else {
    Serial.print("Botão pressionado: ");
    Serial.println(button);
  }
  
  IrReceiver.resume();
  return button;
}

void initializeSegmentDisplay() {
  for (int i = 0; i < 7; i++) {
    pinMode(SEGMENT_PINS[i], OUTPUT);
    digitalWrite(SEGMENT_PINS[i], LOW);
  }
}

void clearSegmentDisplay() {
  for (int i = 0; i < 7; i++) {
    digitalWrite(SEGMENT_PINS[i], LOW);
  }
}

void displayDigitOnSegments(int digit) {
  if (digit < 0 || digit > 9) return;
  
  for (int i = 0; i < 7; i++) {
    digitalWrite(SEGMENT_PINS[i], DIGIT_PATTERNS[digit][i]);
  }
}

void updateLCDSystemState() {
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  
  if (currentState == SYSTEM_OFF) {
    lcd_1.print("SISTEMA: OFF");
  } else {
    lcd_1.print("SISTEMA: ON ");
  }
}

void clearLCDInputLine() {
  lcd_1.setCursor(0, 1);
  lcd_1.print("                "); // Clear entire line
}

void updateLCDInputDisplay() {
  lcd_1.setCursor(0, 1);
  lcd_1.print(userInputBuffer);
  
  // Fill remaining space with blanks
  for (int i = strlen(userInputBuffer); i < LCD_WIDTH; i++) {
    lcd_1.print(" ");
  }
}

void handleSystemOff(int button) {
  clearSegmentDisplay();
  
  if (button == POWER_BUTTON) {
    currentState = SYSTEM_ON;
    userInputBuffer[0] = '\0';
    updateLCDSystemState();
    clearLCDInputLine();
    
    // Clear serial buffer
    while (Serial.available()) {
      Serial.read();
    }
  }
}

void handleSystemOn(int button) {
  if (button == POWER_BUTTON) {
    currentState = SYSTEM_OFF;
    userInputBuffer[0] = '\0';
    updateLCDSystemState();
    clearLCDInputLine();
    clearSegmentDisplay();
  }
}

void processSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    processCharacterInput(c);
  }
}

void processCharacterInput(char c) {
  // Skip LCD update during rapid input
  static unsigned long lastDisplayUpdate = 0;
  
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
    processLetterInput(c);
    
    // Update display only every 100ms during rapid input
    if (millis() - lastDisplayUpdate > 100) {
      updateLCDInputDisplay();
      lastDisplayUpdate = millis();
    }
  } else if (c >= '0' && c <= '9') {
    processNumberInput(c);
  }
}

void processLetterInput(char letter) {
  // Convert to uppercase if lowercase
  if (letter >= 'a' && letter <= 'z') {
    letter = letter - 'a' + 'A';
  }
  
  int bufferLength = strlen(userInputBuffer);
  
  if (bufferLength < LCD_WIDTH) {
    // Add to end of buffer
    userInputBuffer[bufferLength] = letter;
    userInputBuffer[bufferLength + 1] = '\0';
  } else {
    // Shift buffer left and add new character
    memmove(userInputBuffer, userInputBuffer + 1, LCD_WIDTH - 1);
    userInputBuffer[LCD_WIDTH - 1] = letter;
    userInputBuffer[LCD_WIDTH] = '\0';
  }
  
  updateLCDInputDisplay();
}

void processNumberInput(char number) {
  int digit = number - '0';
  displayDigitOnSegments(digit);
}