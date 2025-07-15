#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <Servo.h>
#include <DHT11.h>

MCUFRIEND_kbv tft;

#define SERVO_PIN 11
#define POT_PIN A5 // analog pin used to connect the potentiometer
#define DHT_PIN 10
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define YP A2
#define XM A3
#define YM 8
#define XP 9
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920
#define MINPRESSURE 10
#define MAXPRESSURE 1000

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

uint8_t selectedIndex = 0;
uint8_t zoomLevel = 1;         // 1 = full view, 2 = zoom in, etc.
int scrollOffset = 0;      // For panning left/right
bool inMenu = true;

class Button{
  public:
    // x, y: top-left corner
    // w, h: width and height
    // label: The buttons's message
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

Button anglesBtn(60, 30, 200, 40, "1. Angles", 2, BLUE);
Button intensitiesBtn(60, 80, 200, 40, "2. Intensities", 2, BLUE);
Button tempsBtn(60, 130, 200, 40, "3. Temperatures", 2, BLUE);
Button humiditiesBtn(60, 180, 200, 40, "4. Humidities", 2, BLUE);
Button backBtn(10, 210, 60, 30,"Back",1, CYAN);
Button zoomInBtn(80, 210, 50, 30,"+",1, GREEN);
Button zoomOutBtn(140, 210, 50, 30,"-",1, GREEN);
Button panLeftBtn(200, 210, 50, 30,"<",1, BLUE);
Button panRightBtn(260, 210, 50, 30,">",1, BLUE); 

DHT11 dht11(DHT_PIN); // digital pin 2

const int INTERVALS = 180;   // a day in 5 minutes
int current_interval = -1;

struct Records {
  uint8_t angles[INTERVALS] = {0};
  uint8_t intensities[INTERVALS] = {0};
  uint8_t temperatures[INTERVALS] = {0};
  uint8_t humidities[INTERVALS] = {0};
};

struct Records weather_data;

Servo myservo;  // create Servo object to control a servo

void rotateServo(int angle ){
  myservo.write(180 - angle);
}

int getCurrentBestAngle(){

  if(current_interval==-1)
    return 0;

  return weather_data.angles[current_interval];
}

void printUint8Array(const uint8_t* arr) { //print for debugging

  int size = current_interval+1;

  //size = INTERVALS; // prints entire dataset;

  Serial.print("[");
  for (int i = 0; i < size ; i++) {
    Serial.print(arr[i]);
    if (i < size - 1) Serial.print(",");
  }
  Serial.println("]");
}

void printRecords(const Records& rec) { //print for debugging
  Serial.println("Records content:");

  Serial.print("angles:      ");
  printUint8Array(rec.angles);

  Serial.print("intensities: ");
  printUint8Array(rec.intensities);

  Serial.print("temperatures:");
  printUint8Array(rec.temperatures);

  Serial.print("humidities:  ");
  printUint8Array(rec.humidities);

}

void find_sun( int current_best_angle = 0 ){ // finds best angle and adds it to weather_data at index = current_interval

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
  weather_data.angles[current_interval] = new_angle;
  weather_data.intensities[current_interval] = map(max_light, 0, 1023, 0, 100);
  rotateServo(new_angle);
}

bool find_temp_and_humid(){ // reads temp and humidity and adds them to weather_data at index = current_interval

    int temperature = 0;
    int humidity = 0;

    int result = dht11.readTemperatureHumidity(temperature, humidity);

    if (result == 0) {
        weather_data.temperatures[current_interval] = temperature;
        weather_data.humidities[current_interval] = humidity;
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
  tft.setRotation(3);
  tft.fillScreen(BLACK);

  drawMenu();
}

void responsiveDelay(unsigned long delayTimeMs) {
  unsigned long startTime = millis();
  while (millis() - startTime < delayTimeMs) {
    updateGUI();
    delay(3); // allow cpu to breath a little.
  }
}

void loop() {
  for (int i = 0; i <= INTERVALS; i++){

    long start = millis();
    int cba = getCurrentBestAngle();

    current_interval++; // increment current_interval before calling find_sun & find_temp functions so new data is added at next index.

    find_sun( cba );

    if(!find_temp_and_humid()){ // sensor error
      break;
    }

    printRecords(weather_data);
    
    // GUI STUFF HERE
    updateGraph();

    Serial.print("Runtime: ");
    int runtime = millis() - start;
    Serial.println( runtime );
    Serial.println(); // blank line for readability
    
    //responsiveDelay(30000 - runtime);
    responsiveDelay (5000);
  }

}

void updateGUI() { //non-blocking

  static unsigned long lastTouchTime = 0;
  const int debounceDelay = 300;

  TSPoint p = ts.getPoint();

  // Restore pin modes
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    unsigned long currentTime = millis();
    if (currentTime - lastTouchTime > debounceDelay) {
      int x = map(p.x, TS_MINX, TS_MAXX, 0, 320);
      int y = map(p.y, TS_MINY, TS_MAXY, 0, 240);
      handleTouch(x, y);  // touch handler
      lastTouchTime = currentTime;  // Reset debounce timer
    }
  }
}

void drawMenu() {
  inMenu = true;
  tft.fillScreen(BLACK);

  anglesBtn.draw(tft); 
  intensitiesBtn.draw(tft); 
  tempsBtn.draw(tft); 
  humiditiesBtn.draw(tft); 
}

void drawGraph(const uint8_t (&dataset)[INTERVALS], const char* title) {
  inMenu = false;
  tft.fillScreen(BLACK);

  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(10, 2);
  tft.print(title);

  tft.drawRect(10, 20, 300, 180, WHITE);

  int originX = 10;
  int originY = 200;
  int maxY = 1;
  for(int i=0;i<INTERVALS;i++){//find max value in dataset for mapping later.
    if(dataset[i]>maxY)
      maxY = dataset[i];
  }

  int dataPoints = INTERVALS/zoomLevel;

  if(scrollOffset+dataPoints>INTERVALS)
    dataPoints = INTERVALS - scrollOffset;
  
  for(int i=0;i<dataPoints-1;i++){

    int x1 = i*zoomLevel;
    int y1 = map(dataset[i+scrollOffset],0,maxY,0,180);

    int x2 = (i+1)*zoomLevel;
    int y2 =  map(dataset[i+1+scrollOffset],0,maxY,0,180);

    tft.drawLine(x1+originX, originY - y1, x2+originX, originY - y2, RED);
  }

  backBtn.draw(tft);
  zoomInBtn.draw(tft);
  zoomOutBtn.draw(tft);
  panLeftBtn.draw(tft);
  panRightBtn.draw(tft);
}

void handleTouch(int x, int y) {
  if (inMenu) {
    if (anglesBtn.isTouched(x, y))  { selectedIndex = 0; drawGraph(weather_data.angles, "Angles"); }
    else if (intensitiesBtn.isTouched(x,y)) { selectedIndex = 1; drawGraph(weather_data.intensities, "Intensities"); }
    else if (tempsBtn.isTouched(x, y)) { selectedIndex = 2; drawGraph(weather_data.temperatures, "Temperatures"); }
    else if (humiditiesBtn.isTouched(x, y)) { selectedIndex = 3; drawGraph(weather_data.humidities, "Humidities"); }
    else 
      selectedIndex = 4;
  }
  else {
    if (backBtn.isTouched(x, y)) {
      zoomLevel = 1;
      scrollOffset = 0;
      drawMenu();
    } 
    else if (zoomInBtn.isTouched(x, y)) {
      if (zoomLevel < 8) 
        zoomLevel *= 2;
      updateGraph();
    } 
    else if (zoomOutBtn.isTouched(x, y)) {

      if (zoomLevel > 1) 
        zoomLevel /= 2;
      updateGraph();
    } 
    else if (panLeftBtn.isTouched(x, y)) {

      scrollOffset -= (INTERVALS/4)/zoomLevel;
      if (scrollOffset < 0)
        scrollOffset = 0;

      updateGraph();
    } 
    else if (panRightBtn.isTouched(x, y)) {
      scrollOffset += (INTERVALS/4)/zoomLevel;
      updateGraph();
    }
  }
}

void updateGraph() { // update graph after  +,-,<,> & new readings
  switch (selectedIndex) {
    case 0:
      drawGraph(weather_data.angles, "Angles"); break;
    case 1: 
      drawGraph(weather_data.intensities, "Intensities"); break;
    case 2: 
      drawGraph(weather_data.temperatures, "Temperatures"); break;
    case 3: 
      drawGraph(weather_data.humidities, "Humidities"); break;
    default:
      return;
  }
}