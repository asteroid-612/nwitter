#include "SoftwareSerial.h"
SoftwareSerial H2S_sensor(13, 12); //WemosD1r2 D7(GPIO13)/RX, D6(GPIO12)/TX

String H2S_string;
String temp[11];

#include "Ucglib.h"
#define TFT_RST 3
#define TFT_CS D8
#define TFT_DC D2
Ucglib_ILI9341_18x240x320_HWSPI ucg(TFT_DC, TFT_CS, TFT_RST);

int variable1 = 0;


void setup() {
  lcdSetup();
  H2S_sensor.begin(9600);
  Serial.begin(9600);
  delay(100);

  Serial.flush();
  H2S_sensor.flush();
  H2S_sensor.write('c');
  
}


void loop() {
  //Serial.print("Main loop: ");
  //Serial.println(H2S_serial.available());
  while(H2S_sensor.available()){
    //Serial.print("in While: ");
    //Serial.println(H2S_serial.available());
    char inByte = H2S_sensor.read();
    H2S_string += inByte;
    if (inByte == '\n') {
      //Serial.println("in If");
      stringSplit(H2S_string);
      Serial.print("H2S (ppb) = ");
      Serial.print(temp[1]);
      Serial.print("    Temperature(ºC) = ");
      Serial.print(temp[2]);
      Serial.print("    Humidity (%)  = ");
      Serial.println(temp[3]);
      //Serial.print(H2S_string);
      lcdLoop();   //If you comment this line out then the serial monitor will continue printing the H2S sensor values but the screen doesn't print values.
                        //If you don't comment this function out then the serial monitor and LCD print the h2S values out once then stop working.
      
      H2S_string = "";
      variable1 += 1;
    }
  }
}

void lcdLoop() {
  ucg.begin(UCG_FONT_MODE_SOLID);
  ucg.setRotate270();

  /*********************  H2S    **************************/
  ucg.setFont(ucg_font_fub42_hf);
  ucg.setColor(0, 0, 255, 0);
  ucg.setColor(1, 0, 0, 0);
  ucg.setPrintPos(20, 100);  //ucg.setPrintPos(48, 100);
  ucg.print(variable1);

  /*********************  NH3   **************************/
  ucg.setFont(ucg_font_fub30_hf);
  ucg.setColor(0, 0, 255, 0);
  ucg.setColor(1, 0, 0, 0);
  ucg.setPrintPos(220, 85);
  ucg.print(variable1);

  /*********************  TVOC   **********************/
  ucg.setFont(ucg_font_fub30_hf);
  ucg.setColor(0, 0, 255, 0);
  ucg.setColor(1, 0, 0, 0);
  ucg.setPrintPos(220, 195);
  ucg.print(variable1);
  Serial.println("lcdLoop");
}

void lcdSetup() {
  ucg.begin(UCG_FONT_MODE_SOLID);
  ucg.clearScreen();
  ucg.setRotate270();

  /*********************  Structure   **************************/
  ucg.setColor(255, 255, 255);  // Set color (0,R,G,B)
  ucg.drawFrame(0, 11, 320, 216);  // Start from top-left pixel (x, y, wigth, height, radius)
  ucg.drawVLine(160, 11, 216);
  ucg.drawHLine(160, 120, 160);

  /*********************  Header   **************************/
  ucg.setFont(ucg_font_helvR08_hf); // Size 8 font
  ucg.setColor(0, 255, 255, 255);  // Set color (0,R,G,B)
  ucg.setColor(1, 0, 0, 0);  // Set color of text background (1,R,G,B)

  ucg.setPrintPos(0, 8); // Set position (x,y)
  ucg.print("Magok Odor Sensor");  // Print text or value

  ucg.setPrintPos(220, 8);  // Set position (x,y)
  ucg.print("Wifi:");  // Print text or value

  ucg.setPrintPos(245, 8); // Set position (x,y)
  ucg.setColor(0, 255, 0, 0);  // Set color (0,R,G,B)
  ucg.print("Not Connected");  // Print text or value

  /*********************  Footer   **************************/
  ucg.setFont(ucg_font_helvR08_hf);
  ucg.setColor(0, 255, 255, 255);  // Set color (0,R,G,B)
  ucg.setPrintPos(0, 238);  // Set position (x,y)
  ucg.print("cmcKorea.org");  // Print text or value

  ucg.setPrintPos(210, 238);  // Set position (x,y)
  ucg.print("Firmware Version: 2.4.2");  // Print text or value

  /*********************  H2S setup   ***********************/
  ucg.setFont(ucg_font_helvB12_hf);
  ucg.setColor(0, 255, 255, 255);  // Set color (0,R,G,B)
  ucg.setPrintPos(65, 35);
  ucg.print("H2S");

  ucg.setFont(ucg_font_helvR08_hf);
  ucg.setColor(0, 255, 255, 255);
  ucg.setPrintPos(65, 125);
  ucg.print("(PPB)");

  ucg.setColor(2, 227, 1);
  ucg.drawBox(10, 210, 28, 6);
  ucg.setColor(255, 255, 0);
  ucg.drawBox(38, 210, 28, 6);
  ucg.setColor(255, 126, 0);
  ucg.drawBox(66, 210, 28, 6);
  ucg.setColor(255, 0, 0);
  ucg.drawBox(94, 210, 28, 6);
  ucg.setColor(154, 0, 70);
  ucg.drawBox(122, 210, 28, 6);

  ucg.setColor(255, 255, 255);
  ucg.drawDisc(30, 205, 2, UCG_DRAW_ALL);   //Bar indicator circle

  ucg.setColor(2, 227, 1);
  ucg.drawDisc(80, 165, 25, UCG_DRAW_ALL);   //face
  ucg.setColor(0, 0, 0);
  ucg.drawDisc(80, 165, 22, UCG_DRAW_ALL);

  /*********************  NH3 setup  *********************/
  ucg.setFont(ucg_font_helvB12_hf);
  ucg.setColor(0, 255, 255, 255);  // Set color (0,R,G,B)
  ucg.setPrintPos(225, 35);
  ucg.print("NH3");

  ucg.setFont(ucg_font_helvR08_hf);
  ucg.setColor(0, 255, 255, 255);
  ucg.setPrintPos(225, 110);
  ucg.print("(ug/m3)");

  /*********************  TVOC setup *****************/
  ucg.setFont(ucg_font_helvB12_hf);
  ucg.setColor(0, 255, 255, 255);  // Set color (0,R,G,B)
  ucg.setPrintPos(220, 145);
  ucg.print("TVOC");

  ucg.setFont(ucg_font_helvR08_hf);
  ucg.setColor(0, 255, 255, 255);
  ucg.setPrintPos(225, 215);
  ucg.print("(ug/m3)");
}

#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0])))

void stringSplit(String s) {
  int _to = s.indexOf(',');
  int i;
  int ptr = 0;
  int str_size = s.length();
  

  for (i = 0; i < s.length(); i++) {
    temp[ptr++] = s.substring(0, _to);
    s = s.substring(_to + 2, str_size - 1);
    _to = s.indexOf(',');
    
    if (_to == -1) {
      break;
    }
  }
}
