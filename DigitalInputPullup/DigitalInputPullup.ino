/*
  Input Pullup Serial

  This example demonstrates the use of pinMode(INPUT_PULLUP). It reads a
  digital input on pin 2 and prints the results to the serial monitor.

  The circuit:
   Momentary switch attached from pin 2 to ground
   Built-in LED on pin 13

  Unlike pinMode(INPUT), there is no pull-down resistor necessary. An internal
  20K-ohm resistor is pulled to 5V. This configuration causes the input to
  read HIGH when the switch is open, and LOW when it is closed.

  created 14 March 2012
  by Scott Fitzgerald

  http://www.arduino.cc/en/Tutorial/InputPullupSerial

  This example code is in the public domain

*/
int oldSensor1 = 0;
int oldSensor2 = 0;

void setup() {
  //start serial connection
  Serial.begin(9600);
  //configure pin2 as an input and enable the internal pull-up resistor
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  
  pinMode(13, OUTPUT);

}

void loop() {
  //read the pushbutton value into a variable
  int sensorVal = digitalRead(2);
  //print out the value of the pushbutton
  //Serial.println(sensorVal);

  // Keep in mind the pullup means the pushbutton's
  // logic is inverted. It goes HIGH when it's open,
  // and LOW when it's pressed. Turn on pin 13 when the
  // button's pressed, and off when it's not:
  if (sensorVal == HIGH) {
    digitalWrite(13, LOW);
    oldSensor1 = 1;

  } else {
    if (oldSensor1 == 1) {
      digitalWrite(13, HIGH);
      Serial.println("NEXT");
      delay(150);
    }
    oldSensor1 = 0;
  }

  sensorVal = digitalRead(4);
  //print out the value of the pushbutton
  //Serial.println(sensorVal);

  // Keep in mind the pullup means the pushbutton's
  // logic is inverted. It goes HIGH when it's open,
  // and LOW when it's pressed. Turn on pin 13 when the
  // button's pressed, and off when it's not:
  if (sensorVal == HIGH) {
    digitalWrite(13, LOW);
    oldSensor2 = 1;

  } else {
    if (oldSensor2 == 1) {
      digitalWrite(13, HIGH);
      Serial.println("PREV");
      delay(150);
    }
    oldSensor2 = 0;
  }

  
}



