  // for type definitions

int eepromReadInt(int address){
   int value = 0x0000;
   value = value | (EEPROM.read(address) << 8);
   value = value | EEPROM.read(address+1);
   return value;
}
 
void eepromWriteInt(int address, int value){
   EEPROM.write(address, (value >> 8) & 0xFF );
   EEPROM.write(address+1, value & 0xFF);
}
 

unsigned int eepromReadString(int offset, int bytes, char *buf){
  char c = 0;
  for (int i = offset; i < (offset + bytes); i++) {
    c = EEPROM.read(i);
    buf[i - offset] = c;
    if (c == 0) 
    {
      return i-offset;
    }
  }
}



unsigned int eepromWriteString(int offset, int bytes, char *buf){
  char c = 0;
  //int len = (strlen(buf) < bytes) ? strlen(buf) : bytes;
  for (int i = 0; i < bytes; i++) {
    c = buf[i];
    EEPROM.write(offset + i, c); 
    if (c == 0) 
    {
      return i;
    }
  }
}
unsigned int eepromWriteString1(int offset, int bytes, const char *buf){
  char c = 0;
  //int len = (strlen(buf) < bytes) ? strlen(buf) : bytes;
  for (int i = 0; i < bytes; i++) {
    c = buf[i];
    EEPROM.write(offset + i, c); 
    if (c == 0) 
    {
      return i;
    }
  }
}
