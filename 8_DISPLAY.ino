void drawDisplayContent(void)
{
  if (useDisplay)
  {
    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(2, 7);
    char buf[4];
    // PT sensor has priority
    if (numberOfPTSensors > 0)
    {
      if (ptSensors[0].value == -127.0)
      {
        display.setTextSize(2);
        display.print("Bad Value!");
      }
      else
      {
        dtostrf(ptSensors[0].value, 2, 2, buf);
        display.print(buf);
        display.setTextSize(1);
        display.print(" ");
        display.setTextSize(2);
        display.write(247); // deg symbol
        display.setTextSize(3);
        display.print("C");
      }
    }
    else if (numberOfOneWireSensors > 0)
    {
      if (oneWireSensors[0].value == -127.0)
      {
        display.setTextSize(2);
        display.print("Bad Value!");
      }
      else
      {
        dtostrf(oneWireSensors[0].value, 2, 2, buf);
        display.print(buf);
        display.setTextSize(1);
        display.print(" ");
        display.setTextSize(2);
        display.write(247); // deg symbol
        display.setTextSize(3);
        display.print("C");
      }
    }
    else if (useDistanceSensor)
    {
      if (distanceSensor.value == -127.0)
      {
        display.setTextSize(2);
        display.print("Bad Value!");
      }
      else
      {
        dtostrf(distanceSensor.value, 2, 1, buf);
        display.print(buf);
        display.setTextSize(1);
        display.print(" ");
        display.setTextSize(3);
        display.print("cm");
      }
    }
    else
    {
      display.setTextSize(2);
      display.print("No Sensor!");
    }

    display.display();
  }
}

void drawDisplayContentError(void)
{
  if (useDisplay)
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(2, 7);
    display.print("No Signal!");
    display.display();
  }
}
