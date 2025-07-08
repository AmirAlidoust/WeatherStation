#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// Hardware-specific libraries
#include "Adafruit_GFX.h"
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

// Assign human-readable names to some common 16-bit color values
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Touchscreen setup
// information extracted from the calibration process
//#define YP A3
//#define XM A2
#define YM 9
#define XP 8

#define TS_MINX 919
#define TS_MAXX 97
#define TS_MINY 74
#define TS_MAXY 903

MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, A3, A2, YM, 300);

// define class typicalBtn
class typicalBtn {
  public:
    // x, y: top-left corner
    // w, h: width and height
    // label: The buttons's message
    typicalBtn(int x, int y, int w, int h, const char* label): _x(x), _y(y), _w(w), _h(h), _label(label) {}

    void draw(MCUFRIEND_kbv &tft){
      tft.fillRect(_x, _y, _w, _h, BLUE);
      tft.setTextColor(WHITE);
      tft.setTextSize(2);
      tft.setCursor(_x + 10, _y + 10);
      tft.print(_label);
    }
    bool isTouched(int x, int y) {
      return x >= _x && x <= (_x + _w) && y >= _y && y <= (_y + _h);
    }
  private:
    int _x, _y, _w, _h;
    const char* _label;
};

// define class dottedGraph
class dottedGraph {
  public:
    dottedGraph(MCUFRIEND_kbv &display): tft(display){}

    void draw(int *yVals, int len) {

      //xData = xVals;
      yData = yVals;
      dataLen = len;
      tft.fillScreen(WHITE);

      // Margins
      const int leftMargin = 20;
      const int bottomMargin = 30;
      const int topMargin = 20;
      const int rightMargin = 10;

      int graphWidth = tft.width() - leftMargin - rightMargin;
      Serial.print(tft.width());
      int graphHeight = tft.height() - topMargin - bottomMargin;

      float xScale = 5; // (float)graphWidth / 100;  // xScale = 10
      float yScale = (float)graphHeight / 100; // yScale value = 19 

      // Draw axes
      int originX = leftMargin;
      int originY = tft.height() - bottomMargin;

      // Draw axis lines
      tft.drawLine(originX, topMargin, originX, originY, BLACK); // Y-axis
      tft.drawLine(originX, originY, tft.width() - rightMargin, originY, BLACK); // X-axis

      // Draw tick marks and numbers on X axis
      for (int i = 0; i <= dataLen; i++) {
        int xTick = originX + i * xScale;
        tft.drawLine(xTick, originY - 3, xTick, originY + 3, BLACK);

        /*
        tft.setTextColor(BLACK);
        tft.setTextSize(1);
        tft.setCursor(xTick - 4, originY + 6);
        tft.print(String(i)+ "3:05");
        */
        }

      // Draw tick marks and numbers on Y axis
      for (int i = 0; i <= 100; i++) {
        int yTick = originY - i * yScale;
        tft.drawLine(originX - 3, yTick, originX + 3, yTick, BLACK);


        // tft.setTextColor(BLACK);
        // tft.setTextSize(1);
        // tft.setCursor(0, yTick - 3);
        // tft.print(i);
      }

      // Plot graph points
      for (int i = 0; i < dataLen; i++) {
        int x = originX + i * xScale;
        int y = originY - yData[i+50] * yScale;

        tft.fillCircle(x, y, 2, RED);

        if (i > 0) {
          int px = originX + (i-1) * xScale;
          int py = originY - yData[i + 49] * yScale;
          tft.drawLine(px, py, x, y, BLUE);
        }
      }
    }
  private:
    MCUFRIEND_kbv &tft;
    int *xData;
    int *yData;
    int dataLen = 0;
};

dottedGraph graph(tft);  // make an instance of the dottedGraph class
typicalBtn graphBtn(60, 150, 140, 40, "Show Graph"); // make an instance of the typicalBtn class

bool showGraph = false;

// test values
int yVals[288] = {
  47, 92, 76, 38, 62, 93, 70, 84, 44, 98, 35, 12, 64, 55, 17, 50,
  86, 33, 23, 14, 95, 77, 32, 41, 28, 49, 79, 25, 88, 40, 31, 61,
  10, 58, 66, 80, 63, 36, 13, 42, 20, 16, 91, 67, 57, 26, 85, 89,
  94, 43, 29, 96, 60, 87, 99, 30, 21, 18, 24, 11, 48, 15, 27, 46,
  59, 53, 22, 73, 90, 56, 97, 37, 34, 19, 45, 52, 51, 39, 54, 71,
  65, 68, 74, 78, 75, 72, 69, 83, 82, 81, 100, 0, 1, 2, 3, 4,
  5, 6, 7, 8, 9, 10, 23, 17, 33, 41, 58, 62, 76, 99, 14, 27,
  55, 36, 20, 12, 60, 45, 93, 71, 70, 80, 90, 74, 48, 26, 34, 87,
  46, 29, 24, 19, 11, 13, 16, 21, 25, 28, 30, 31, 32, 35, 38, 40,
  43, 47, 50, 53, 54, 63, 66, 67, 68, 73, 77, 79, 82, 83, 84, 85,
  86, 88, 89, 91, 92, 94, 95, 96, 98, 100, 3, 7, 15, 22, 39, 42,
  44, 49, 51, 52, 57, 59, 61, 64, 65, 69, 75, 81, 18, 37, 6, 8,
  0, 1, 2, 4, 5, 9, 56, 78, 10, 33, 20, 77, 60, 19, 54, 68,
  84, 97, 25, 35, 18, 22, 39, 70, 41, 48, 92, 29, 11, 28, 5, 64,
  66, 80, 24, 21, 50, 88, 27, 93, 38, 16, 7, 55, 86, 23, 67, 17,
  34, 30, 79, 62, 43, 9, 87, 52, 85, 37, 61, 59, 10, 73, 47, 71,
  76, 31, 95, 82, 44, 83, 94, 42, 53, 72, 58, 26, 15, 90, 3, 4,
  36, 63, 78, 45, 12, 13, 81, 6, 1, 8, 0, 14, 65, 17, 2, 10
};

//const uint16_t xVals[] = {3, 2, 4, 4, 5};
//const uint16_t yVals[] = {0, 1, 2, 3, 4};
int dataCount = sizeof(yVals) / sizeof(yVals[0]);

void setup() {
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(1); // 0=0, 1=90, 2=180, 3=270
  tft.fillScreen(WHITE);
  tft.setCursor(70,30);
  tft.setTextColor(BLACK);
  tft.setTextSize(2); // choose between 1 to 5
  tft.println("Weather Station");

  graphBtn.draw(tft);

}

void loop() {
  
  TSPoint p = ts.getPoint(); // get the pressure values p.x, p.y and p.z
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  if (p.z > 50 && p.z < 1000){  // check the intesity of pressure
    // map the raw values to pixel values
    int x = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
    int y = map(p.x, TS_MINX, TS_MAXX, tft.height(), 0);

    if (!showGraph && graphBtn.isTouched(x, y)) {
      showGraph = true;
      graph.draw(yVals, dataCount);
    }
    else if (showGraph) {
      // Go back to first screen
      showGraph = false;
      tft.fillScreen(WHITE);
      tft.setCursor(70, 30);
      tft.setTextColor(BLACK);
      tft.setTextSize(2);
      tft.println("Weather Station");
      graphBtn.draw(tft);
    }
  }
}
