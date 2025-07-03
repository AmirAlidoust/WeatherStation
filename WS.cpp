#include <Servo.h>
#include <DHT11.h>

Servo myservo;  // create Servo object to control a servo

int potpin = A1;    // analog pin used to connect the potentiometer

DHT11 dht11(2); // digital pin 2

const int INTERVALS = 288;   // a day in 5 minutes
int current_interval = -1;

struct Records {
  uint8_t angles[INTERVALS] = {0};
  uint8_t intensities[INTERVALS] = {0};
  uint8_t temperatures[INTERVALS] = {0};
  uint8_t humidities[INTERVALS] = {0};
};

struct Records weather_data;

// START ------- helper functions

void rotateServo(int angle ){
  myservo.write(180 - angle);
}

int getCurrentBestAngle(){

  if(current_interval==-1)
    return 0;

  return weather_data.angles[current_interval];
}

void printUint8Array(const uint8_t* arr) {

  int size = current_interval+1;
  //size = INTERVALS; // prints entire dataset;

  Serial.print("[");
  for (int i = 0; i < size; i++) {
    Serial.print(arr[i]);
    if (i < size - 1) Serial.print(",");
  }
  Serial.println("]");
}

void printRecords(const Records& rec) {
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

// END ------- helper functions

void find_sun( int current_best_angle = 0 ){ // finds best angle and adds it to weather_data at index = current_interval

  int max_light = 0; // high at higher light intensity. 0 light = 0volt (means very high photo resistance)
  current_best_angle = 0; // always start from 0
  int new_angle = current_best_angle;

  for (int angle= current_best_angle; angle <= 180; angle += 6) {
    
    rotateServo(angle);
    delay(300); // allow servo to move to new position and photoresistor to take new reading
    int obs_light = analogRead(potpin);
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

  myservo.attach(3);    // attaches the servo on pin 3 to the Servo object
  Serial.begin(9600);

  // dht11.setDelay(500); // Set this to the desired delay. Default is 500ms.

}

void loop() {

  for (int i = 0; i <= 288; i++){

    long start = millis();
    int cba = getCurrentBestAngle();

    current_interval++; // increment current_interval before calling find_sun & find_temp functions so new data is added at next index.

    find_sun( cba );

    if(!find_temp_and_humid()){ // sensor error
      break;
    }

    printRecords(weather_data);

    // GUI STUFF HERE

    Serial.print("Runtime: ");
    int runtime = millis() - start;
    Serial.println( runtime );
    Serial.println(); // blank line for readability
    
    delay(30000 - runtime);
  }

}



