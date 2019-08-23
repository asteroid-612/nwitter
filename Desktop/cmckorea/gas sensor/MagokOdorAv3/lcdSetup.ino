//void lcdSetup() {
//  ucg.begin(UCG_FONT_MODE_SOLID);
//  ucg.clearScreen();
//  ucg.setRotate270();
//
//  /*********************  Structure   **************************/
//  ucg.setColor(255, 255, 255);  // Set color (0,R,G,B)
//  ucg.drawFrame(0, 11, 320, 216);  // Start from top-left pixel (x, y, wigth, height, radius)
//  ucg.drawVLine(160, 11, 216);
//  ucg.drawHLine(160, 120, 160);
//
//  /*********************  Header   **************************/
//  ucg.setFont(ucg_font_helvR08_hf); // Size 8 font
//  ucg.setColor(0, 255, 255, 255);  // Set color (0,R,G,B)
//  ucg.setColor(1, 0, 0, 0);  // Set color of text background (1,R,G,B)
//
//  ucg.setPrintPos(0, 8); // Set position (x,y)
//  ucg.print("Magok Odor Sensor");  // Print text or value
//
//  ucg.setPrintPos(220, 8);  // Set position (x,y)
//  ucg.print("Wifi:");  // Print text or value
//
//  ucg.setPrintPos(245, 8); // Set position (x,y)
//  ucg.setColor(0, 255, 0, 0);  // Set color (0,R,G,B)
//  ucg.print("Not Connected");  // Print text or value
//
//  /*********************  Footer   **************************/
//  ucg.setFont(ucg_font_helvR08_hf);
//  ucg.setColor(0, 255, 255, 255);  // Set color (0,R,G,B)
//  ucg.setPrintPos(0, 238);  // Set position (x,y)
//  ucg.print("cmcKorea.org");  // Print text or value
//
//  ucg.setPrintPos(210, 238);  // Set position (x,y)
//  ucg.print("Firmware Version: 2.4.2");  // Print text or value
//
//  /*********************  H2S setup   ***********************/
//  ucg.setFont(ucg_font_helvB12_hf);
//  ucg.setColor(0, 255, 255, 255);  // Set color (0,R,G,B)
//  ucg.setPrintPos(65, 35);
//  ucg.print("H2S");
//
//  ucg.setFont(ucg_font_helvR08_hf);
//  ucg.setColor(0, 255, 255, 255);
//  ucg.setPrintPos(65, 125);
//  ucg.print("(PPB)");
//
//  ucg.setColor(2, 227, 1);
//  ucg.drawBox(10, 210, 28, 6);
//  ucg.setColor(255, 255, 0);
//  ucg.drawBox(38, 210, 28, 6);
//  ucg.setColor(255, 126, 0);
//  ucg.drawBox(66, 210, 28, 6);
//  ucg.setColor(255, 0, 0);
//  ucg.drawBox(94, 210, 28, 6);
//  ucg.setColor(154, 0, 70);
//  ucg.drawBox(122, 210, 28, 6);
//
//  ucg.setColor(255, 255, 255);
//  ucg.drawDisc(30, 205, 2, UCG_DRAW_ALL);   //Bar indicator circle
//
//  ucg.setColor(2, 227, 1);
//  ucg.drawDisc(80, 165, 25, UCG_DRAW_ALL);   //face
//  ucg.setColor(0, 0, 0);
//  ucg.drawDisc(80, 165, 22, UCG_DRAW_ALL);
//
//  /*********************  NH3 setup  *********************/
//  ucg.setFont(ucg_font_helvB12_hf);
//  ucg.setColor(0, 255, 255, 255);  // Set color (0,R,G,B)
//  ucg.setPrintPos(225, 35);
//  ucg.print("NH3");
//
//  ucg.setFont(ucg_font_helvR08_hf);
//  ucg.setColor(0, 255, 255, 255);
//  ucg.setPrintPos(225, 110);
//  ucg.print("(ug/m3)");
//
//  /*********************  TVOC setup *****************/
//  ucg.setFont(ucg_font_helvB12_hf);
//  ucg.setColor(0, 255, 255, 255);  // Set color (0,R,G,B)
//  ucg.setPrintPos(220, 145);
//  ucg.print("TVOC");
//
//  ucg.setFont(ucg_font_helvR08_hf);
//  ucg.setColor(0, 255, 255, 255);
//  ucg.setPrintPos(225, 215);
//  ucg.print("(ug/m3)");
//}
