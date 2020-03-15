void drawDisplayContent(void) {
  if(use_display) {
    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(2, 7);
    char buf[4];
    // PT sensor has priority
    if(numberOfPTSensors > 0){
      dtostrf(ptSensors[0].value, 2, 2, buf);
    }
    else if(numberOfOneWireSensors > 0){
      dtostrf(oneWireSensors[0].sens_value, 2, 2, buf);
    }
    else{
      buf[0] = 0; 
    }
    
    if(buf[0] != 0){ // sensor found
    display.print(buf);
    display.setTextSize(1);
    display.print(" ");
    display.setTextSize(2);
    display.write(247); // deg symbol
    display.setTextSize(3);
    display.print("C");
    }
    else{
      display.setTextSize(2);
      display.print("No Sensor!");
    }
    
    display.display();
  }
}

void drawDisplayContentError(void) {
  if(use_display) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(2, 7);
    display.print("No Signal!");
    display.display();
  }
}
