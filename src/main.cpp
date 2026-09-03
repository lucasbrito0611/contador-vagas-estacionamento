#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

void setup() {
    LCD.init();
    LCD.backlight();
}

void loop() {
    LCD.setCursor(0, 0);
    LCD.println("Hello, World!");
    delay(1000);

    LCD.clear();
    delay(500);
}
