// Libraries
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h> 
#include <TouchScreen.h>
#include <Servo.h>
#include <DHT11.h>

// Pin definitions 
#define SERVO_PIN 11 // Servo control pin
#define POT_PIN A5 // Potentiometer analog input pin
#define DHT_PIN 10 // Digital pin for DHT11 sensor

// Touchscreen pins
#define YP A2
#define XM A3
#define YM 8
#define XP 9
#define TS_MINX 120
#define TS_MAXX 940
#define TS_MINY 103
#define TS_MAXY 922

// Touch pressure thresholds
#define MINPRESSURE 10
#define MAXPRESSURE 1000

// Color definitions
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

const int INTERVALS = 180;   // Simulating a day in 8 minutes (max records RAM can hold)

int current_interval = -1; // Index of the current data record (-1 = no data yet)
uint8_t selectedIndex = 4; // Current screen index (4 = main menu)
uint8_t zoomLevel = 1; // Zoom level for graphs (1 = full view)
int scrollOffset = 0; // Horizontal scroll offset for panning graphs
bool inMenu = true; // Flag to check if we are in the main menu screen

MCUFRIEND_kbv tft; // TFT display object
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300); // Touchscreen object
DHT11 dht11(DHT_PIN); // Temperature/humidity sensor object
Servo myservo;  // Servo motor object

// Button class to handle UI buttons
class Button{
  public:
    // Constructor to initialize button position, size, label, font size, and background color
    Button(int x, int y, int w, int h, const char* label, uint8_t font=1, uint16_t bgColor = BLUE): 
    _x(x), _y(y), _w(w), _h(h), _label(label), _font(font), _bgColor(bgColor){}
      
    // Draw the button on the screen
    void draw(MCUFRIEND_kbv &tft){
      tft.fillRect(_x, _y, _w, _h, _bgColor);
      tft.setTextColor(WHITE);
      tft.setTextSize(_font);

      // Calculate text position to center label
      int xPos = strlen(_label) * 6 * _font;
      xPos = _x + (_w - xPos)/2;
      int yPos = _y + (_h - 7* _font)/2;
      tft.setCursor(xPos, yPos);
      tft.print(_label);
    }
    // Check if a given touch point is inside the button area
    bool isTouched(int x, int y) {
      return x >= _x && x <= (_x + _w) && y >= _y && y <= (_y + _h);
    }
  private:
    int _x, _y;
    uint8_t _font, _w, _h;
    uint16_t _bgColor;
    const char* _label;
};

// Create buttons for the UI menu and controls
Button anglesBtn(60, 30, 200, 40, "Angles", 2, BLUE);
Button intensitiesBtn(60, 80, 200, 40, "Intensities", 2, BLUE);
Button tempsBtn(60, 130, 200, 40, "Temperatures", 2, BLUE);
Button humiditiesBtn(60, 180, 200, 40, "Humidities", 2, BLUE);
Button backBtn(10, 220, 60, 20,"Back",2, RED);
Button zoomInBtn(80, 220, 50, 20,"+",2, MAGENTA);
Button zoomOutBtn(140, 220, 50, 20,"-",2, MAGENTA);
Button panLeftBtn(200, 220, 50, 20,"<",2, BLUE);
Button panRightBtn(260, 220, 50, 20,">",2, BLUE); 

// Structure to hold data for each time interval
struct Records {
  uint8_t angles[INTERVALS] = {0};
  uint8_t intensities[INTERVALS] = {0};
  uint8_t temperatures[INTERVALS] = {0};
  uint8_t humidities[INTERVALS] = {0};
};

struct Records weather_data; // Global variable to store all recorded data

// Rotate servo to a specific angle
void rotateServo(int angle){
  myservo.write(180 - angle);
}

// Forward declarations of functions used later
void responsiveDelay(unsigned long a);
void drawGraphFromIndex();

// Find best servo angle by checking light intensity
void find_best_angle(){
  int max_light = 0; // Highest light reading found
  int new_angle = 0;

  // Sweep servo from 0 to 180 degrees in steps of 6
  for (int angle= 0; angle <= 180; angle += 6) {
    rotateServo(angle); // Move servo
    responsiveDelay(300); // Wait for servo to move and sensor to stabilize
    int obs_light = analogRead(POT_PIN); // Read potentiometer as light sensor
    // Keep track of max light reading
    if (obs_light > max_light) {
      max_light = obs_light;
      new_angle = angle;
    }
  }
  // Store best angle and corresponding intensity
  weather_data.angles[current_interval+1] = new_angle;
  weather_data.intensities[current_interval+1] = map(max_light, 0, 1023, 0, 100);
  rotateServo(new_angle); // Move servo to best angle found
}

// Read temperature and humidity from DHT11 sensor and store in records
bool find_temp_and_humid(){
    int temperature = 0;
    int humidity = 0;

    // Read sensor data
    int result = dht11.readTemperatureHumidity(temperature, humidity);
    // result = 0 means success
    if (result == 0) {
        weather_data.temperatures[current_interval+1] = temperature;
        weather_data.humidities[current_interval+1] = humidity;
    } else {
        // Print error message if sensor read failed
        Serial.println(DHT11::getErrorString(result));
        return false;
    }
    return true;
}

void setup() {
  Serial.begin(9600); // Start serial communication for debugging
  myservo.attach(SERVO_PIN); // Attach servo to its pin
  
  uint16_t ID = tft.readID(); // Read display ID
  tft.begin(ID); // Initialize TFT display
  tft.setRotation(1); // Set display rotation (1 = landscape)
  tft.fillScreen(BLACK); // Clear screen to black
  
  drawMenu(); // Show main menu on startup
}

// Draw main menu
void drawMenu() {
  inMenu = true;
  tft.fillScreen(BLACK);

  // Draw all main menu buttons
  anglesBtn.draw(tft); 
  intensitiesBtn.draw(tft); 
  tempsBtn.draw(tft); 
  humiditiesBtn.draw(tft); 
}

// Draw a graph of the data for a given dataset and title, with optional unit label
void drawGraph(const uint8_t (&dataset)[INTERVALS], const char* title, const char unit[]="%") {
  inMenu = false;
  tft.fillScreen(BLACK);

  // Display title and current interval index
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

  // Show current data value at current_interval
  tft.setCursor(250, 2);
  if(current_interval!=-1){
      char buff[10];
      int pval = dataset[current_interval];
      snprintf(buff, sizeof(buff), "%d%s", pval, unit);
      tft.print(buff);
  }
  else
      tft.print("...");
  
  // Draw X and Y axis lines
  tft.drawFastHLine(20, 200, 290, WHITE);
  tft.drawFastVLine(20, 20, 180, WHITE);

  // Calculate how many data points to show based on zoom level
  int dataPoints = INTERVALS / zoomLevel;

  // Find max Y value in current view to scale graph properly
  int maxY = 0;
  int data_upper_index_limit = dataPoints + scrollOffset;

  for(int i=scrollOffset;i<data_upper_index_limit;i++){
    if(dataset[i]>maxY)
      maxY = dataset[i];
  }

  // Draw Y-axis labels (11 divisions)
  tft.setTextSize(1);
  tft.setTextColor(RED);
  int y_axis_upper_limit = maxY;
  y_axis_upper_limit = y_axis_upper_limit==0? 50 : y_axis_upper_limit;
  for(int i=0; i<=10; i++){
    tft.drawFastHLine(18, 200 - i*18, 5, WHITE);
    tft.setCursor(2, 198-i*18);
    tft.print(i*y_axis_upper_limit/10);
  }

  // Draw X-axis labels and ticks (11 steps)
  for(int i=0,j=0 ; i<=290 ; i+=29, j++){
    tft.drawFastVLine(i+20, 197 , 7, WHITE);
    tft.setCursor(i+11, 207);
    int x_label = j*18/zoomLevel + scrollOffset;
    tft.print(x_label);
  }

  // boundary checking for dataPoints
  if(scrollOffset + dataPoints > INTERVALS)
    dataPoints = INTERVALS - scrollOffset;

  // Draw the graph line connecting data points
  for (int i=0;i<dataPoints-1 && (i+1+scrollOffset) <= current_interval;i++){ 

    int x1 = i*zoomLevel;
    x1 = map(x1, 0, (dataPoints-2)*zoomLevel, 20, 310);
    int y1 = map(dataset[i+scrollOffset],0,maxY,200,20);

    int x2 = (i+1)*zoomLevel;
    x2 = map(x2, 0,  (dataPoints-2)*zoomLevel , 20, 310);

    int y2 =  map(dataset[i+1+scrollOffset],0,maxY,200,20);

    tft.drawLine(x1, y1, x2, y2, RED);
  }

  // Draw navigation buttons on the graph screen
  backBtn.draw(tft);
  zoomInBtn.draw(tft);
  zoomOutBtn.draw(tft);
  panLeftBtn.draw(tft);
  panRightBtn.draw(tft);
}

// Handle touchscreen press coordinates and perform actions based on UI state
void handleTouch(int x, int y) {
  if (inMenu) {
    // In main menu: check which button was pressed and show corresponding graph
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
    // In graph view: handle zoom, pan, back button presses
    if (backBtn.isTouched(x, y)) {
      zoomLevel = 1;
      scrollOffset = 0;
      selectedIndex = 4; // Go back to main menu
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

// Update the graph display based on which dataset is selected
void drawGraphFromIndex() {
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

// Poll touchscreen input and handle touch events with simple debounce
void updateGUI() {

  static unsigned long lastTouchTime = 0;
  const int debounceDelay = 100; // Minimum time between touch events (ms)

  TSPoint p = ts.getPoint(); // Get touch point

  // Reset touchscreen pins to output mode after reading
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // Check if touch pressure is in valid range
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    unsigned long currentTime = millis();
    if (currentTime - lastTouchTime > debounceDelay) {
      // Map raw touchscreen values to screen coordinates
      int x = map(p.x, TS_MINX, TS_MAXX, 320, 0);
      int y = map(p.y, TS_MINY, TS_MAXY, 240, 0);
      handleTouch(x, y);  // Pass touch coordinates to handler
      lastTouchTime = currentTime;  // Update debounce timer
    }
  }
}

// Delay function that updates GUI while waiting (non-blocking)
void responsiveDelay(unsigned long delayTimeMs) {
  unsigned long startTime = millis();
  while (millis() - startTime < delayTimeMs) {
    updateGUI(); // Keep GUI responsive during delay
    delay(1); // Small delay to reduce CPU load
  }
}

void loop() {
  // Main data collection loop runs through all intervals
  for (int i = 0; i < INTERVALS; i++){
    long start = millis();
    
    // Find and save best servo angle and light intensity
    find_best_angle();
    
    // Read and save temperature and humidity
    if(!find_temp_and_humid()){
      break;
    }
    
    // Move to next interval after all data is recorded
    current_interval++; 
    
    // Update graph on screen
    drawGraphFromIndex();

    int runtime = millis() - start;

    // Wait for remaining time to reach 15 seconds per reading
    responsiveDelay (15000 - runtime);
  }
  
  // Reset interval index after completing all records
  current_interval = -1;
}