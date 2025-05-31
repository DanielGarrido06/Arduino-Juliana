#include <IRremote.h>
#include <Adafruit_LiquidCrystal.h>

// IR receiver pin
const int RECV_PIN = 2;
IRrecv irrecv(RECV_PIN);
decode_results results;


Adafruit_LiquidCrystal lcd_1(0);

void setup() {
    lcd_1.begin(16, 2); // 16x2 LCD
    Serial.begin(9600);
    lcd_1.print("Waiting for IR!");
    irrecv.enableIRIn(); // Start the receiver
}

void loop() {
    if (irrecv.decode(&results)) {
        lcd_1.clear();
        lcd_1.setCursor(0, 0);
        lcd_1.print("IR Code:");
        lcd_1.setCursor(0, 1);
        // Print code in HEX
        Serial.print("Received IR code: 0x");
        Serial.println(results.value, HEX);
        irrecv.resume(); // Receive the next value
        delay(1000); // Show code for 1 second
        lcd_1.clear();
        lcd_1.print("Waiting for IR");
    }
}