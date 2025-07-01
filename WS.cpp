#include <Servo.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

Servo myservo;  // create Servo object to control a servo

int potpin = A1;    // analog pin used to connect the potentiometer
float intensity;    // variable to read the value from the analog pin

int R2;
int R1 = 120;
int best_angle;
const INTERVALS = 2881;   // a day in seconds
int curr_interval = 0;

struct Records {
  int angles[INTERVALS];
  int intensities[INTERVALS];
  int temperatures[INTERVALS];
  int humidities[INTERVALS];
}

Records weather_data;


int find_sun(int new_angle = 0){
  int opt_val = 1023;
  for (int angle=0; angle <= 180; angle += 6) {
    myservo.write(angle);
    delay(300);
    int obs_val = analogRead(potpin);
    if (obs_val < opt_val) {
      opt_val = obs_val;
      new_angle = angle;
    }
  }
  weather_data.angles[curr_interval]=new_angle;
  weather_data.intensity[curr_interval]=opt_val;
}


void setup() {
  myservo.attach(3);    // attaches the servo on pin 3 to the Servo object
  Serial.begin(9600);
  find_sun();
  dht.begin();
}

void loop() {

  for (int i = 0; i <= 86400; i+=30){
    find_sun(best_angle);
    delay(5000);
  }

  //Serial.print("intensity= ");
  //Serial.println(intensity);

  delay(100);                           // waits for the servo to get there

}