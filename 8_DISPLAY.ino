void drawDisplayContent(void) {
  if(USE_DISPLAY) {
    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(2, 7);
    char buf[4];
    dtostrf(ptSensors[0].value, 2, 2, buf);
    display.print(buf);
    display.setTextSize(1);
    display.print(" ");
    display.setTextSize(2);
    display.write(247); // deg symbol
    display.setTextSize(3);
    display.print("C");
    display.display();
  }
}

void drawDisplayContentError(void) {
  if(USE_DISPLAY) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(2, 7);
    display.print("No Signal!");
    display.display();
  }
}
