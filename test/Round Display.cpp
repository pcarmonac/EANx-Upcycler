#if GUI == 1
  void gaugeBaseLayout() {
    // Draw Layout -- Adjust this layouts to suit you LCD  
    gauge.createSprite(TFT_WIDTH,240);
    gauge.setSwapBytes(true);
    gauge.setTextDatum(4);
    gauge.setTextColor(TFT_WHITE,backColor);    
    needle.createSprite(TFT_WIDTH,240);
    needle.setSwapBytes(true);
    background.createSprite(TFT_WIDTH,240);
    background.setSwapBytes(true);

    background.fillSprite(TFT_BLACK);
    debugln("FILL BG FILL BG ");

    for(int i=0;i<360;i++) {
        x[i]=((r-10)*cos(DEG2RAD*angle))+cx;
        y[i]=((r-10)*sin(DEG2RAD*angle))+cy;
        px[i]=((r-14)*cos(DEG2RAD*angle))+cx;
        py[i]=((r-14)*sin(DEG2RAD*angle))+cy;
        lx[i]=((r-24)*cos(DEG2RAD*angle))+cx;
        ly[i]=((r-24)*sin(DEG2RAD*angle))+cy;
        nx[i]=((r-36)*cos(DEG2RAD*angle))+cx;
        ny[i]=((r-36)*sin(DEG2RAD*angle))+cy;
        x2[i]=((r-10)*cos(DEG2RAD*angle2))+cx;
        y2[i]=((r-10)*sin(DEG2RAD*angle2))+cy2;
        px2[i]=((r-14)*cos(DEG2RAD*angle2))+cx;
        py2[i]=((r-14)*sin(DEG2RAD*angle2))+cy2;
        lx2[i]=((r-24)*cos(DEG2RAD*angle2))+cx;
        ly2[i]=((r-24)*sin(DEG2RAD*angle2))+cy2;
        nx2[i]=((r-36)*cos(DEG2RAD*angle2))+cx;
        ny2[i]=((r-36)*sin(DEG2RAD*angle2))+cy2;
        
        angle++;
        if(angle==360)
        angle=0;
        
        angle2++;
        if(angle2==360)
        angle2=0;
      }


    // Upper Gauge 
    gauge.drawSmoothArc(cx, cy, r, ir, 120, 240, gaugeColor, backColor);
    gauge.drawSmoothArc(cx, cy, r-5, r-6, 120, 240, TFT_WHITE, backColor);
    gauge.drawSmoothArc(cx, cy, r-9, r-8, 120, 140, TFT_RED, backColor);
    gauge.drawSmoothArc(cx, cy, r-38, ir-37, 120, 240, gaugeColor, backColor);

    // Lower Gauge 
    gauge.drawSmoothArc(cx2, cy2, r, ir, 300, 60, gaugeColor, backColor);
    gauge.drawSmoothArc(cx2, cy2, r-5, r-6, 300, 60, TFT_WHITE, backColor);
    gauge.drawSmoothArc(cx2, cy2, r-9, r-8, 30, 60, TFT_RED, backColor);
    gauge.drawSmoothArc(cx2, cy2, r-38, ir-37, 300, 60, gaugeColor, backColor);


    // Draw Gauge Markings
    for(int i=0;i<21;i++){

      if (i*5 <= 21) { color1=TFT_CYAN, color2=TFT_CYAN; }
      if (i*5  <= 18) { color1=TFT_RED; color2=TFT_YELLOW; }
      if (i*5  >= 22) { color1=TFT_GREEN; color2=TFT_GREEN; }

      int wedginc = 6;
      int wedginc2 = 6;

      if(i%2==0) {
        gauge.drawWedgeLine(x[i*wedginc],y[i*wedginc],px[i*wedginc],py[i*wedginc],2,1,color1);
        gauge.setTextColor(color1,backColor);
        gauge.drawString(String(i*5),lx[i*wedginc],ly[i*wedginc]);
      } 
      else {
        gauge.drawWedgeLine(x[i*wedginc],y[i*wedginc],px[i*wedginc],py[i*wedginc],1,1,color2);
      }
      if (i*12 <= 60) { color1=TFT_CYAN, color2=TFT_GREEN; }
      if (i*12  > 60) { color1=TFT_RED; color2=TFT_YELLOW; }
      if (i*12  >= 130) { color1=TFT_GREEN; color2=TFT_RED; }

      if(i%2==0) {
        gauge.drawWedgeLine(x2[i*wedginc2],y2[i*wedginc2],px2[i*wedginc2],py2[i*wedginc2],2,1,color1);
        gauge.setTextColor(color2,backColor);
        gauge.drawString(String(i*12),lx2[i*wedginc2],ly2[i*wedginc2]);
      } 
      else {
        gauge.drawWedgeLine(x2[i*wedginc2],y2[i*wedginc2],px2[i*wedginc2],py2[i*wedginc2],1,1,color2);
      }
    }
    /*
    gauge.setTextColor(TFT_ORANGE, TFT_BLACK);
    gauge.setTextSize(1 * ResFact);
    gauge.drawCentreString("@1.4  M@D  @1.6", TFT_WIDTH *.5, TFT_HEIGHT * 0.5, 2);
    */
    //gauge.pushSprite(0,spFactor);
    //gauge.pushToSprite(&background,0,0);
    debugln("First Gauge Create");


  }

  void  displayGaugeData() {
  // Text info
  if (prevO2 != currentO2) {
    if (currentO2 > 20 and currentO2 < 22) { gauge.setTextColor(TFT_CYAN, TFT_BLACK); }
    if (currentO2 <= 20) { gauge.setTextColor(TFT_YELLOW, TFT_BLACK); }
    if (currentO2 <= 18) { gauge.setTextColor(TFT_RED, TFT_BLACK); }
    if (currentO2 >= 22) { gauge.setTextColor(TFT_GREEN, TFT_BLACK); }

    gauge.setTextSize(1);
    String o2 = String(currentO2, 1);
    gauge.drawCentreString(o2, TFT_WIDTH * 0.5, TFT_HEIGHT * 0.30, 7);

    tft.setTextSize(1 * ResFact);
/*
    if (metric == 1)  {
      String mod14m = String(mod14msw);
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.drawString(String(mod14m + "-m  "), TFT_WIDTH * 0.05, TFT_HEIGHT * 0.75, 2);
      tft.setTextColor(TFT_GOLD, TFT_BLACK);
      String mod16m = String(mod16msw);
      tft.drawString(String(mod16m + "-m  "), TFT_WIDTH * 0.7, TFT_HEIGHT * 0.75, 2);
    }
    else {
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      String mod14f = String(mod14fsw);
      tft.drawString(String(mod14f + "-FT  "), TFT_WIDTH * 0, TFT_HEIGHT * 0.75, 2);
      tft.setTextColor(TFT_GOLD, TFT_BLACK);
      String mod16f = String(mod16fsw);
      tft.drawString(String(mod16f + "-FT  "), TFT_WIDTH * 0.6, TFT_HEIGHT * 0.75, 2);
    }
*/

    }
  // Draw gauge with needle
  int sA=currentO2*1.2;
  int m14A=mod14fsw/2;
  int m16A=mod16fsw/2;

  gauge.pushToSprite(&needle, 0, 0);

  // gauge.pushSprite(0,spFactor);

  needle.drawWedgeLine(px[(int)sA],py[(int)sA],nx[(int)sA],ny[(int)sA],3,3,needleColor);
  needle.drawWedgeLine(px2[(int)m14A],py2[(int)m14A],nx2[(int)m14A],ny2[(int)m14A],2,2,TFT_SKYBLUE);
  needle.drawWedgeLine(px2[(int)m16A],py2[(int)m16A],nx2[(int)m16A],ny2[(int)m16A],2,2,needleColor);


  needle.pushSprite(0,spFactor);

  debugln("Needle Sequence");
  }
  
#endif