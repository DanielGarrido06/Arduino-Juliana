// C++ code
//
#include <Adafruit_LiquidCrystal.h> // Biblioteca para o LCD

#include <IRremote.h> // Biblioteca para o receptor IR

int estado = 0; // Variável para armazenar o estado do sistema (0 = OFF, 1 = ON)

int botao = 0; // Variável para armazenar o botão pressionado

Adafruit_LiquidCrystal lcd_2(0); // Cria um objeto para o LCD

// Função dada pelos exemplos do Tinkercad para mapear o código IR para um botão específico
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

// Função dada pelos exemplos do Tinkercad para ler o IR
int readInfrared() {
  int result = -1;
  // Check if we've received a new code
  if (IrReceiver.decode()) {
    // Get the infrared code
    unsigned long code = IrReceiver.decodedIRData.decodedRawData;
    // Map it to a specific button on the remote
    result = mapCodeToButton(code);
    // Enable receiving of the next value
    IrReceiver.resume();
  }
  return result;
}

void setup()
{
  lcd_2.begin(16, 2); // Inicializa o LCD com 16 colunas e 2 linhas
  IrReceiver.begin(2); // Inicializa o receptor IR no pino 2


  lcd_2.print("Sistema: OFF"); // Exibe o estado inicial do sistema no LCD
  estado = 0; // Estado inicial do sistema é OFF
  botao = 0;
}

void loop()
{   
    // Função readInfrared() para ler o botão pressionado no controle remoto gera erro no Tinkercard por motivo desconhecido,
    // A função foi copiada do exemplo do Tinkercad, onde ela funciona corretamente, mas nesse projeto não funciona.
    
    botao = readInfrared();
    if(botao == 0) { // Botão de energia pressionado
        // Muda o estado do sistema
        if (estado == 0) {
            lcd_2.clear();
            lcd_2.print("Sistema: ON");
            estado = 1;
        } else {
            lcd_2.clear();
            lcd_2.print("Sistema: OFF");
            estado = 0;
        }
    }
    delay(10); // Pequeno atraso para evitar leituras rápidas demais (Sujerido pelos exemplos do Tinkercad)
}