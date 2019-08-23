//void lcdLoop() {
//  ucg.begin(UCG_FONT_MODE_SOLID);
//  ucg.setRotate270();
//
//  /*********************  H2S    **************************/
//  ucg.setFont(ucg_font_fub42_hf);
//  ucg.setColor(0, 0, 255, 0);
//  ucg.setColor(1, 0, 0, 0);
//  ucg.setPrintPos(20, 100);  //ucg.setPrintPos(48, 100);
//  ucg.print(temp[1]);
//
//  /*********************  NH3   **************************/
//  ucg.setFont(ucg_font_fub30_hf);
//  ucg.setColor(0, 0, 255, 0);
//  ucg.setColor(1, 0, 0, 0);
//  ucg.setPrintPos(220, 85);
//  ucg.print(temp[2]);
//
//  /*********************  TVOC   **********************/
//  ucg.setFont(ucg_font_fub30_hf);
//  ucg.setColor(0, 0, 255, 0);
//  ucg.setColor(1, 0, 0, 0);
//  ucg.setPrintPos(220, 195);
//  ucg.print(temp[3]);
//  Serial.println("lcdLoop");
//}
