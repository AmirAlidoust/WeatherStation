#include <Servo.h>

Servo myservo;  // create Servo object to control a servo

int potpin = A1;  // analog pin used to connect the potentiometer
float val;    // variable to read the value from the analog pin

int R2;
int R1 = 120;
int angle;


void setup() {
  myservo.attach(3);  // attaches the servo on pin 9 to the Servo object
  Serial.begin(9600);

  int best_angle = 0;
  int opt_val = 1023;

  for (angle=0; angle <= 180; angle += 5) {
    myservo.write(angle);
    delay(300);
    int obs_val = analogRead(potpin);
    if (obs_val < opt_val) {
      opt_val = obs_val;
      best_angle = angle;
    }
  }
  myservo.write(best_angle);

}

void loop() {
  val = analogRead(potpin);            // reads the value of the potentiometer (value between 0 and 1023)

  Serial.print("val= ");
  Serial.println(val);

  delay(100);                           // waits for the servo to get there

}
