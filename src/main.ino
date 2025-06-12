/**
  Este código implementa um sistema embarcado que utiliza um controle remoto infravermelho (IR) para interagir com um Arduino.
  O sistema possui dois estados principais: ligado (ON) e desligado (OFF), controlados por um botão POWER do controle remoto.
  
  Funcionalidades:
  - Recepção de comandos IR para alternar o estado do sistema e exibir dígitos no display de 7 segmentos.
  - Exibição do estado atual do sistema em um display LCD 16x2.
  - Entrada de caracteres pelo teclado do computador conectado, exibindo letras na segunda linha do LCD e números no display de 7 segmentos.
  - Implementação de debounce para evitar múltiplas leituras do mesmo botão IR.
  - Mapeamento dos códigos dos botões do controle remoto para ações específicas.
  - Atualização não-bloqueante do loop principal para garantir responsividade.
  - Os números de 0 a 9 são exibidos no display de 7 segmentos tanto quando os botões correspondentes são pressionados,
    quanto quando números são digitados via serial.
 */

#define IR_RECEIVE_PIN 2 // Pino de recepção do IR
#define EXCLUDE_UNIVERSAL_PROTOCOLS // Exclui protocolos IR não usados
#define DECODE_NEC // Inclui de volta o protocolo NEC, que é de fato usado por este controle remoto

#include <Adafruit_LiquidCrystal.h> // Biblioteca para controle do display LCD
#include <IRremote.h> // Biblioteca para recepção de sinais infravermelhos

// Enum representando os estados do sistema
enum SystemState {
  SYSTEM_OFF = 0,
  SYSTEM_ON = 1
};

// Mapeamento dos botões do controle remoto
const int POWER_BUTTON = 0;
const int BUTTON_CODES[10] = {12, 16, 17, 18, 20, 21, 22, 24, 25, 26}; // BUTTON_CODES[i] = Código do botão do número i no controle remoto (12 para 0, 16 para 1, etc.) Os codigos são da função dada pelo Tinkercad.

// Pinos do display de 7 segmentos
const int SEGMENT_PINS[] = {4, 3, 9, 8, 7, 5, 6}; // A, B, C, D, E, F, G
const int SEGMENT_A = 0, SEGMENT_B = 1, SEGMENT_C = 2, SEGMENT_D = 3;
const int SEGMENT_E = 4, SEGMENT_F = 5, SEGMENT_G = 6;

// Constantes de tempo
const unsigned long DEBOUNCE_DELAY = 500; // Intervalo de debounce em milissegundos
const int LOOP_DELAY = 10; // Intervalo de loop em milissegundos

// Configuração do LCD
Adafruit_LiquidCrystal lcd_1(0); // Inicializa o LCD com o endereço 0x27
const int LCD_WIDTH = 16; // Largura do LCD
const int LCD_HEIGHT = 2; // Altura do LCD

// Variáveis globais
SystemState currentState = SYSTEM_OFF;  // Estado atual do sistema (OFF ou ON)
int lastButtonPressed = -1;  // Último botão pressionado (-1 significa nenhum botão pressionado) (útil pro debounce)
unsigned long lastButtonTime = 0; // Tempo do último botão pressionado
unsigned long lastLoopTime = 0; // Variável para controle do tempo do loop
char userInputBuffer[LCD_WIDTH + 1] = ""; // (+1 para o terminador nulo '\0') // variavel para armazenar a entrada do usuário (letras) no LCD

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

// Função de configuração do Arduino, chamada uma vez no início.
void setup() {
  initializeSegmentDisplay();
  
  IrReceiver.begin(IR_RECEIVE_PIN);
  lcd_1.begin(LCD_WIDTH, LCD_HEIGHT);
  Serial.begin(9600);
  
  updateLCDSystemState();
}

// Função de loop do Arduino, chamada repetidamente a cada LOOP_DELAY milissegundos. Essa é a função principal do programa.
void loop() {
    // delay não-bloqueante para evitar travamentos
  unsigned long currentTime = millis();
  if (currentTime - lastLoopTime < LOOP_DELAY) {
    return; // sai do loop se o tempo desde o último loop for menor que LOOP_DELAY
  }
  lastLoopTime = currentTime;

  int button = readInfrared();  // Lê o código IR do controle remoto e retorna o número do botão pressionado ou -1 se não for um botão válido
  
  // lida com o botão pressionado, se for válido
  if (isValidButtonPress(button)) {
    updateLastButton(button);
    
    if (currentState == SYSTEM_OFF) {
      handleSystemOff(button);
    } else if (currentState == SYSTEM_ON) {
      handleSystemOn(button);
    }
  }
  
  // processa a entrada serial se o sistema estiver ligado
  if (currentState == SYSTEM_ON) {
    processSerialInput();
  }
}

// Função para verificar se o botão pressionado é válido. Retorna TRUE ou FALSE.
bool isValidButtonPress(int button) {
  return (button != lastButtonPressed && 
          (millis() - lastButtonTime > DEBOUNCE_DELAY));
}

// Função para mapear o código IR para o botão correspondente. Retorna o número do botão ou -1 se não for um botão válido.
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

// Função para ler o código IR do controle remoto. Retorna o número do botão pressionado ou -1 se não for um botão válido.
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
  lcd_1.print("                "); // Limpa a linha de entrada
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
    
    // Limpa o buffer de entrada do usuário
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
  
  // Processa somente se houver caracteres disponíveis e se o tempo desde o último processamento for maior que LOOP_DELAY
  if (Serial.available() && (millis() - lastCharProcessTime >= LOOP_DELAY)) {
    char c = Serial.read();
    processCharacterInput(c);
    lastCharProcessTime = millis();
  }
}

// Função para processar a entrada de caracteres
void processCharacterInput(char c) {
  // Verifica se o caractere é uma letra ou um número
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
    processLetterInput(c);
  } else if (c >= '0' && c <= '9') {
    processNumberInput(c);
  }
}

// Função para processar a entrada de letras
void processLetterInput(char letter) {
  // Converte letras minúsculas para maiúsculas usando aritimética ASCII
  if (letter >= 'a' && letter <= 'z') {
    letter = letter - 'a' + 'A';
  }
  
  int bufferLength = strlen(userInputBuffer);
  
  if (bufferLength < LCD_WIDTH) {
    // Adiciona a letra ao final do buffer
    userInputBuffer[bufferLength] = letter;
    userInputBuffer[bufferLength + 1] = '\0';
  } else {
    // Desloca o buffer para a esquerda e adiciona a letra no final
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