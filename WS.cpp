#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <Servo.h>
#include <DHT11.h>

#define SERVO_PIN 11
#define POT_PIN A5 // analog pin used to connect the potentiometer
#define DHT_PIN 10

#define YP A2
#define XM A3
#define YM 8
#define XP 9
#define TS_MINX 120
#define TS_MAXX 940
#define TS_MINY 103
#define TS_MAXY 922
#define MINPRESSURE 10
#define MAXPRESSURE 1000

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

const int INTERVALS = 180;   // a day in 8 minutes (max records ram can hold)
int current_interval = -1;

uint8_t selectedIndex = 4; // index 4 = main menu, index of current screen
uint8_t zoomLevel = 1;         // 1 = full view, 2 = zoom in, etc.
int scrollOffset = 0;      // For panning left/right
bool inMenu = true;

MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
DHT11 dht11(DHT_PIN); // digital pin 2
Servo myservo;  // create Servo object to control a servo

class Button{
  public:
    // x, y: top-left corner
    Button(int x, int y, int w, int h, const char* label, uint8_t font=1, uint16_t bgColor = BLUE): 
    _x(x), _y(y), _w(w), _h(h), _label(label), _font(font), _bgColor(bgColor){}

    void draw(MCUFRIEND_kbv &tft){
      tft.fillRect(_x, _y, _w, _h, _bgColor);
      tft.setTextColor(WHITE);
      tft.setTextSize(_font);
      int xPos = strlen(_label) * 6 * _font;
      xPos = _x + (_w - xPos)/2;
      int yPos = _y + (_h - 7* _font)/2;
      tft.setCursor(xPos, yPos);
      tft.print(_label);
    }
    bool isTouched(int x, int y) {
      return x >= _x && x <= (_x + _w) && y >= _y && y <= (_y + _h);
    }
  private:
    int _x, _y;
    uint8_t _font, _w, _h;
    uint16_t _bgColor;
    const char* _label;
};

Button anglesBtn(60, 30, 200, 40, "Angles", 2, BLUE);
Button intensitiesBtn(60, 80, 200, 40, "Intensities", 2, BLUE);
Button tempsBtn(60, 130, 200, 40, "Temperatures", 2, BLUE);
Button humiditiesBtn(60, 180, 200, 40, "Humidities", 2, BLUE);
Button backBtn(10, 220, 60, 20,"Back",2, RED);
Button zoomInBtn(80, 220, 50, 20,"+",2, MAGENTA);
Button zoomOutBtn(140, 220, 50, 20,"-",2, MAGENTA);
Button panLeftBtn(200, 220, 50, 20,"<",2, BLUE);
Button panRightBtn(260, 220, 50, 20,">",2, BLUE); 

struct Records {
  uint8_t angles[INTERVALS] = {0};
  uint8_t intensities[INTERVALS] = {0};
  uint8_t temperatures[INTERVALS] = {0};
  uint8_t humidities[INTERVALS] = {0};
};

struct Records weather_data;

void rotateServo(int angle ){
  myservo.write(180 - angle);
}

void responsiveDelay(unsigned long a);

void drawGraphFromIndex();

void find_best_angle( int current_best_angle = 0 ){ // finds best angle and adds it to weather_data at index = current_interval

  int max_light = 0; // high at higher light intensity. 0 light = 0volt (means very high photo resistance)
  current_best_angle = 0; // always start from 0 degrees
  int new_angle = current_best_angle;

  for (int angle= current_best_angle; angle <= 180; angle += 6) {
    
    rotateServo(angle);
    responsiveDelay(300); // allow servo to move to new position and photoresistor to take new reading
    int obs_light = analogRead(POT_PIN);
    if (obs_light > max_light) {
      max_light = obs_light;
      new_angle = angle;
    }
  }
  weather_data.angles[current_interval+1] = new_angle;
  weather_data.intensities[current_interval+1] = map(max_light, 0, 1023, 0, 100);
  rotateServo(new_angle);
}

bool find_temp_and_humid(){ // reads temp and humidity and adds them to weather_data at index = current_interval

    int temperature = 0;
    int humidity = 0;

    int result = dht11.readTemperatureHumidity(temperature, humidity);

    if (result == 0) {
        weather_data.temperatures[current_interval+1] = temperature;
        weather_data.humidities[current_interval+1] = humidity;
    } else {
        Serial.println(DHT11::getErrorString(result));
        return false;
    }
    return true;
}

void setup() {
  Serial.begin(9600);
  myservo.attach(SERVO_PIN); 
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(BLACK);

  drawMenu();
}

void drawMenu() {
  inMenu = true;
  tft.fillScreen(BLACK);

  anglesBtn.draw(tft); 
  intensitiesBtn.draw(tft); 
  tempsBtn.draw(tft); 
  humiditiesBtn.draw(tft); 
}

void drawGraph(const uint8_t (&dataset)[INTERVALS], const char* title, const char unit[]="%") {
  inMenu = false;
  tft.fillScreen(BLACK);

  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(5, 1);
  tft.print(title);

  tft.setCursor(147, 3);
  tft.setTextSize(2);
  tft.setTextColor(YELLOW);
  tft.print("T:");
  tft.setCursor(168, 3);
  tft.print(current_interval);

  tft.setCursor(250, 2);
  if(current_interval!=-1){
      char buff[10];
      
      int pval = dataset[current_interval];

      snprintf(buff, sizeof(buff), "%d%s", pval, unit);

      tft.print(buff);
  }
  else
      tft.print("...");

  tft.drawFastHLine(20, 200, 290, WHITE);
  tft.drawFastVLine(20, 20, 180, WHITE);

  int maxY = 0;
  for(int i=scrollOffset;i<=current_interval;i++){//find max value in dataset for mapping later.
    if(dataset[i]>maxY)
      maxY = dataset[i];
  }

  // draw y-axis labels
  tft.setTextSize(1);
  tft.setTextColor(RED);
  int y_axis_upper_limit = maxY;
  y_axis_upper_limit = y_axis_upper_limit==0? 50 : y_axis_upper_limit ;
  for(int i=0; i<=10; i++){
    tft.drawFastHLine(18, 200 - i*18, 5, WHITE);
    tft.setCursor(2, 198-i*18);
    tft.print(i*y_axis_upper_limit/10);
  }


  for(int i=0,j=0 ; i<=290 ; i+=29, j++){ // 11 steps

    tft.drawFastVLine(i+20, 197 , 7, WHITE);
    tft.setCursor(i+11, 207);

    int x_label = j*18/zoomLevel + scrollOffset;

    tft.print(x_label);
  }


  int dataPoints = INTERVALS/zoomLevel;

  if(scrollOffset+dataPoints>INTERVALS) // dataset boundary checking
    dataPoints = INTERVALS - scrollOffset;
  
  for(int i=0;i<dataPoints-1 && (i+1+scrollOffset) <= current_interval;i++){ 

    int x1 = i*zoomLevel;
    x1 = map(x1, 0, (dataPoints-1)*zoomLevel, 20, 310);
    int y1 = map(dataset[i+scrollOffset],0,maxY,200,20);

    int x2 = (i+1)*zoomLevel;
    x2 = map(x2, 0,  (dataPoints-1)*zoomLevel , 20, 310);

    Serial.println(current_interval);

    int y2 =  map(dataset[i+1+scrollOffset],0,maxY,200,20);

    tft.drawLine(x1, y1, x2, y2, RED);
  }

  backBtn.draw(tft);
  zoomInBtn.draw(tft);
  zoomOutBtn.draw(tft);
  panLeftBtn.draw(tft);
  panRightBtn.draw(tft);
}

void handleTouch(int x, int y) {
  if (inMenu) {
    if (anglesBtn.isTouched(x, y))  {
      selectedIndex = 0;
      drawGraphFromIndex();
    }
    else if (intensitiesBtn.isTouched(x,y)) { 
      selectedIndex = 1;
      drawGraphFromIndex();
    }
    else if (tempsBtn.isTouched(x, y)) { 
      selectedIndex = 2;
      drawGraphFromIndex();
    }
    else if (humiditiesBtn.isTouched(x, y)) { 
      selectedIndex = 3;
      drawGraphFromIndex();
    }
  }
  else {
    
    if (backBtn.isTouched(x, y)) {
      zoomLevel = 1;
      scrollOffset = 0;
      selectedIndex = 4;
      drawMenu();
    } 
    else if (zoomInBtn.isTouched(x, y)) {
      if (zoomLevel < 8) 
        zoomLevel *= 2;
      drawGraphFromIndex();
    } 
    else if (zoomOutBtn.isTouched(x, y)) {

      if (zoomLevel > 1) 
        zoomLevel /= 2;
      drawGraphFromIndex();
    } 
    else if (panLeftBtn.isTouched(x, y)) {

      scrollOffset -= (INTERVALS/4)/zoomLevel;
      if (scrollOffset < 0)
        scrollOffset = 0;

      drawGraphFromIndex();
    }
    else if (panRightBtn.isTouched(x, y)) {
      scrollOffset += (INTERVALS/4)/zoomLevel;
      drawGraphFromIndex();
    }
  }
}

void drawGraphFromIndex() { // update graph after  +,-,<,> & new readings

    switch (selectedIndex) {
      case 0:
        drawGraph(weather_data.angles, "Angle", "\xF7"); break;
      case 1: 
        drawGraph(weather_data.intensities, "Intensity"); break;
      case 2: 
        drawGraph(weather_data.temperatures, "Temperature","\xF7""C"); break;
      case 3: 
        drawGraph(weather_data.humidities, "Humidity","%RH"); break;
      default:
        return;
    }
}

void updateGUI() { //non-blocking debounce, handles touch functionality

  static unsigned long lastTouchTime = 0;
  const int debounceDelay = 100;

  TSPoint p = ts.getPoint();

  // Restore pin modes
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    unsigned long currentTime = millis();
    if (currentTime - lastTouchTime > debounceDelay) { // valid touch if true
      int x = map(p.x, TS_MINX, TS_MAXX, 320, 0);
      int y = map(p.y, TS_MINY, TS_MAXY, 240, 0);
      handleTouch(x, y);  // touch handler
      lastTouchTime = currentTime;  // Reset debounce timer
    }
  }
}

void responsiveDelay(unsigned long delayTimeMs) {
  unsigned long startTime = millis(); // = 5
  while (millis() - startTime < delayTimeMs) {
    updateGUI(); // 10ms
    delay(1); //  cpu breathing
  }
}

void loop() {
  
  for (int i = 0; i < INTERVALS; i++){

    long start = millis();

    //int cba = getCurrentBestAngle();

    find_best_angle();

    if(!find_temp_and_humid()){ // sensor error
      break;
    }

    current_interval++; // increment current interval only after all 4 measurements are taken and recorded.

    // GUI STUFF HERE
    drawGraphFromIndex();

    int runtime = millis() - start;
    
    responsiveDelay (15000 - runtime); // 15 sec delay between every measurement
  }

  current_interval = -1;

}