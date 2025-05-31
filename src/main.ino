// C++ code
//
#define DECODE_NEC
#define IR_RECEIVE_PIN      2
#define EXCLUDE_UNIVERSAL_PROTOCOLS   // Saves up to 1000 bytes program space.
#define EXCLUDE_EXOTIC_PROTOCOLS      // saves around 650 bytes program space if all other protocols are active

#include <Adafruit_LiquidCrystal.h>

#include <IRremote.h>

int button = 0; // Variável para armazenar o botão pressionado

int state = 0; // Estado do sistema: 0 = OFF, 1 = ON

int lastButton = -1 ; // Variável para armazenar o último botão pressionado

int segA = 4; // Pinos do display de 7 segmentos
int segB = 3;
int segC = 9;
int segD = 8;
int segE = 7;
int segF = 5;
int segG = 6;


Adafruit_LiquidCrystal lcd_1(0);

// Buffer para armazenar a linha de entrada (máx 16 caracteres para LCD 16x2)
char userInputBuffer[17] = ""; // +1 para o terminador nulo

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
int mapCodeToButton(unsigned long code) {
  // For the remote used in the Tinkercad simulator,
  // the buttons are encoded such that the hex code
  // received is of the format: 0xiivvBF00
  // Where the vv is the button value, and ii is
  // the bit-inverse of vv.
  // For example, the power button is 0xFF00BF000

  // Check for codes from this specific remote
  if ((code & 0x0000FFFF) == 0x0000BF00) {
    // No longer need the lower 16 bits. Shift the code by 16
    // to make the rest easier.
    code >>= 16;
    // Check that the value and inverse bytes are complementary.
    if (((code >> 8) ^ (code & 0x00FF)) == 0x00FF) {
      return code & 0xFF;
    }
  }
  return -1;
}

int readInfrared() {
  int result = -1;
  // Check if we've received a new code
  if (IrReceiver.decode()) {
    // Get the infrared code
    unsigned long code = IrReceiver.decodedIRData.decodedRawData;
    // Ignore NEC repeat code
    if (code == 0xFFFFFFFF) {
      IrReceiver.resume();
      return -1;
    }
    // Map it to a specific button on the remote
    result = mapCodeToButton(code);
    if (result == -1) {
      Serial.print("Código IR desconhecido: ");
      Serial.println(code, HEX);
    } else {
      Serial.print("Botão pressionado: ");
      Serial.println(result);
    }
    // Enable receiving of the next value
    IrReceiver.resume();
  }
  return result;
}

void setup()
{

  // Configuração dos pinos do display de 7 segmentos
  for (int i = 3; i <= 9; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW); // Inicializa todos os pinos como LOW
  }

  IrReceiver.begin(2);
  lcd_1.begin(16, 2);
  lcd_1.print("SISTEMA: OFF");
  Serial.begin(9600);
}

void loop()
{
  button = readInfrared();

  // Verifica se o botão pressionado é diferente do último
  if (button != lastButton) { 
    lastButton = button; // Atualiza o último botão pressionado
      
    if (state == 0) {
      // Estado de espera: só aceita POWER pelo IR
      if (button == 0) {
        lcd_1.clear();
        state = 1;
        userInputBuffer[0] = '\0';
        lcd_1.setCursor(0, 1);
        lcd_1.print("                ");
        while (Serial.available()) Serial.read(); // Limpa o buffer da Serial
      }
    } else if (state == 1) {
      // Estado de aquisição
      lcd_1.setCursor(0, 0);
      lcd_1.print("SISTEMA: ON ");

      // POWER desliga o sistema
      if (button == 0) {
        lcd_1.clear();
        lcd_1.setCursor(0, 0);
        lcd_1.print("SISTEMA: OFF");
        state = 0;
        userInputBuffer[0] = '\0'; // Limpa o buffer ao desligar
        lcd_1.setCursor(0, 1);
        lcd_1.print("                "); // Limpa a linha do LCD
        // TODO: DISPLAY DE 7 SEGMENTOS
      }
    }
  }

  // Processa entradas do teclado via Serial SOMENTE quando ligado
  if (state == 1) {
    while (Serial.available()) {
      char c = Serial.read();
      // Aceita apenas números e letras
      if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        // Converte para maiúsculo se for letra minúscula
        if (c >= 'a' && c <= 'z') {
          c = c - 'a' + 'A';
        }
        int len = strlen(userInputBuffer);
        if (len < 16) {
          userInputBuffer[len] = c;
          userInputBuffer[len + 1] = '\0';
        } else {
          memmove(userInputBuffer, userInputBuffer + 1, 15);
          userInputBuffer[15] = c;
          userInputBuffer[16] = '\0';
        }
        // Atualiza a segunda linha do LCD
        lcd_1.setCursor(0, 1);
        lcd_1.print(userInputBuffer);
        for (int i = strlen(userInputBuffer); i < 16; i++) {
          lcd_1.print(" ");
        }
      }
    }
  }
  delay(10); // Delay para estabilidade
}