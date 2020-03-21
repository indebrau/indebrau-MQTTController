byte StringToPin(String pinstring)
{
  for (int i = 0; i < NUMBER_OF_PINS; i++)
  {
    if (PIN_NAMES[i] == pinstring)
    {
      return PINS[i];
    }
  }
  return 9;
}

String PinToString(byte pinbyte)
{
  for (int i = 0; i < NUMBER_OF_PINS; i++)
  {
    if (PINS[i] == pinbyte)
    {
      return PIN_NAMES[i];
    }
  }
  return "NaN";
}

bool isPin(byte pinbyte)
{
  bool returnValue = false;
  for (int i = 0; i < NUMBER_OF_PINS; i++)
  {
    if (PINS[i] == pinbyte)
    {
      returnValue = true;
      goto Ende;
    }
  }
Ende:
  return returnValue;
}

byte convertCharToHex(char ch)
{
  byte returnType;
  switch (ch)
  {
    case '0':
      returnType = 0;
      break;
    case '1':
      returnType = 1;
      break;
    case '2':
      returnType = 2;
      break;
    case '3':
      returnType = 3;
      break;
    case '4':
      returnType = 4;
      break;
    case '5':
      returnType = 5;
      break;
    case '6':
      returnType = 6;
      break;
    case '7':
      returnType = 7;
      break;
    case '8':
      returnType = 8;
      break;
    case '9':
      returnType = 9;
      break;
    case 'A':
      returnType = 10;
      break;
    case 'B':
      returnType = 11;
      break;
    case 'C':
      returnType = 12;
      break;
    case 'D':
      returnType = 13;
      break;
    case 'E':
      returnType = 14;
      break;
    case 'F':
      returnType = 15;
      break;
    default:
      returnType = 0;
      break;
  }
  return returnType;
}
