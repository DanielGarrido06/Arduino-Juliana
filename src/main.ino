#define IR_RECEIVE_PIN 2 // Pino de recepção do IR
#define EXCLUDE_UNIVERSAL_PROTOCOLS // Exclui protocolos IR não usados
#define DECODE_NEC // Inclui de volta o protocolo NEC, que é de fato usado por este controle remoto

#include <Adafruit_LiquidCrystal.h>
#include <IRremote.h>

// Enum representando os estados do sistema
enum SystemState {
  SYSTEM_OFF = 0,
  SYSTEM_ON = 1
};

// Mapeamento dos botões do controle remoto
const int POWER_BUTTON = 0;
const int BUTTON_CODES[10] = {12, 16, 17, 18, 20, 21, 22, 24, 25, 26}; // BUTTON_CODES[i] = Código do botão i no controle remoto

// Pinos do display de 7 segmentos
const int SEGMENT_PINS[] = {4, 3, 9, 8, 7, 5, 6}; // A, B, C, D, E, F, G
const int SEGMENT_A = 0, SEGMENT_B = 1, SEGMENT_C = 2, SEGMENT_D = 3;
const int SEGMENT_E = 4, SEGMENT_F = 5, SEGMENT_G = 6;

// Constantes de tempo
const unsigned long DEBOUNCE_DELAY = 500; // intervalo de debounce em milissegundos
const int LOOP_DELAY = 10; // intervalo de loop em milissegundos

// Configuração do LCD
Adafruit_LiquidCrystal lcd_1(0); // Inicializa o LCD com o endereço 0x27
const int LCD_WIDTH = 16; // Largura do LCD
const int LCD_HEIGHT = 2; // Altura do LCD

// Variáveis globais
SystemState currentState = SYSTEM_OFF;
int lastButtonPressed = -1;
unsigned long lastButtonTime = 0;
unsigned long lastLoopTime = 0; // Add this for non-blocking timing
char userInputBuffer[LCD_WIDTH + 1] = ""; // +1 for null terminator

// Padrões dos dígitos para o display de 7 segmentos (a, b, c, d, e, f, g)
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

// Protótipos de funções
bool isValidButtonPress(int button);
int mapCodeToButton(unsigned long code);
int readInfrared();
void updateLastButton(int button);
void initializeSegmentDisplay();
void clearSegmentDisplay();
void displayDigitOnSegments(int digit);
void updateLCDSystemState();
void clearLCDInputLine();
void updateLCDInputDisplay();
void handleSystemOff(int button);
void handleSystemOn(int button);
void processIRInput(int button);
void processSerialInput();
void processCharacterInput(char c);
void processLetterInput(char letter);
void processNumberInput(char number);

// Função de configuração do Arduino, chamada uma vez no início
void setup() {
  initializeSegmentDisplay();
  
  IrReceiver.begin(IR_RECEIVE_PIN);
  lcd_1.begin(LCD_WIDTH, LCD_HEIGHT);
  Serial.begin(9600);
  
  updateLCDSystemState();
}

// Função de loop do Arduino, chamada repetidamente a cada LOOP_DELAY milissegundos
void loop() {
    // delay não-bloqueante para evitar travamentos
  unsigned long currentTime = millis();
  if (currentTime - lastLoopTime < LOOP_DELAY) {
    return; // Exit early if not enough time has passed
  }
  lastLoopTime = currentTime;

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
}

// Função para verificar se o botão pressionado é válido
bool isValidButtonPress(int button) {
  return (button != lastButtonPressed && 
          (millis() - lastButtonTime > DEBOUNCE_DELAY));
}

// Função para mapear o código IR para o botão correspondente
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

// Função para ler o código IR do controle remoto
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

// Função para atualizar o último botão pressionado e o tempo de pressionamento do botão
void updateLastButton(int button) {
  lastButtonPressed = button;
  lastButtonTime = millis();
}

// Função para inicializar o display de 7 segmentos
void initializeSegmentDisplay() {
  for (int i = 0; i < 7; i++) {
    pinMode(SEGMENT_PINS[i], OUTPUT);
    digitalWrite(SEGMENT_PINS[i], LOW);
  }
}

// Função para limpar o display de 7 segmentos
void clearSegmentDisplay() {
  for (int i = 0; i < 7; i++) {
    digitalWrite(SEGMENT_PINS[i], LOW);
  }
}

// Função para exibir um dígito no display de 7 segmentos
void displayDigitOnSegments(int digit) {
  if (digit < 0 || digit > 9) return;
  
  for (int i = 0; i < 7; i++) {
    digitalWrite(SEGMENT_PINS[i], DIGIT_PATTERNS[digit][i]);
  }
}

// Função para atualizar o estado do sistema no LCD
void updateLCDSystemState() {
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  
  if (currentState == SYSTEM_OFF) {
    lcd_1.print("SISTEMA: OFF");
  } else {
    lcd_1.print("SISTEMA: ON ");
  }
}

// Função para limpar a linha de entrada do LCD
void clearLCDInputLine() {
  lcd_1.setCursor(0, 1);
  lcd_1.print("                "); // Clear entire line
}

// Função para atualizar a exibição de entrada do LCD
void updateLCDInputDisplay() {
  lcd_1.setCursor(0, 1);
  lcd_1.print(userInputBuffer);
}

// Função para lidar com o estado do sistema desligado
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

// Função para lidar com o estado do sistema ligado
void handleSystemOn(int button) {
  if (button == POWER_BUTTON) {
    currentState = SYSTEM_OFF;
    userInputBuffer[0] = '\0';
    updateLCDSystemState();
    clearLCDInputLine();
    clearSegmentDisplay();
  }
  else{
    processIRInput(button);
  }  
}

// Função para processar a entrada de números do controle remoto
void processIRInput(int button){
  for (int i = 0; i < 10; i++) {
    if (button == BUTTON_CODES[i]) {
      displayDigitOnSegments(i);
      break;
    }
  }
}

// Função para processar a entrada do usuário via serial
void processSerialInput() {
  static unsigned long lastCharProcessTime = 0;
  
  // Process only one character per loop iteration for better responsiveness
  if (Serial.available() && (millis() - lastCharProcessTime >= LOOP_DELAY)) {
    char c = Serial.read();
    processCharacterInput(c);
    lastCharProcessTime = millis();
  }
}

// Função para processar a entrada de caracteres
void processCharacterInput(char c) {
  // Skip LCD update during rapid input
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
    processLetterInput(c);
  } else if (c >= '0' && c <= '9') {
    processNumberInput(c);
  }
}

// Função para processar a entrada de letras
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

// Função para processar a entrada de números
void processNumberInput(char number) {
  int digit = number - '0';
  displayDigitOnSegments(digit);
}