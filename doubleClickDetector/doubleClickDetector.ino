#define SENDER_SENSOR_1 2
#define SENDER_SENSOR_2 3

#define DOUBLE_CLICK_MAX_TIME 750
#define DEBOUNCE_TIME 50

struct doubleClickState {
  int previousButtonState,
      buttonState,
      upDown,
      sensor;
  uint32_t doubleClickTimeout,
           upDownTime;
};

struct doubleClickState dc1 = { -1, 0, HIGH, SENDER_SENSOR_1, 0, 0 };
struct doubleClickState dc2 = { -1, 0, HIGH, SENDER_SENSOR_2, 0, 0 };

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(SENDER_SENSOR_1, INPUT_PULLUP);
  pinMode(SENDER_SENSOR_2, INPUT_PULLUP);

}

void loop() {
  checkForDoubleClick(&dc1);
  checkForDoubleClick(&dc2);
}


void checkForDoubleClick(struct doubleClickState * doubleClick) {
  if ((*doubleClick).previousButtonState != (*doubleClick).buttonState) {
    (*doubleClick).previousButtonState = (*doubleClick).buttonState;
    Serial.print("Button[");
    Serial.print((*doubleClick).sensor);
    Serial.print("] is now: ");
    Serial.println((*doubleClick).buttonState);
  }

  /* Beck if doubleclick time was exceded  */
  if ((*doubleClick).buttonState > 1 && (millis() - (*doubleClick).doubleClickTimeout) > DOUBLE_CLICK_MAX_TIME) {
    Serial.print("Button[");
    Serial.print((*doubleClick).sensor);
    Serial.println("] double click was too slow!");
    (*doubleClick).buttonState = 0;
    (*doubleClick).doubleClickTimeout = millis();
  }

  /* Button state chanced! with debounce */
  int temp = digitalRead((*doubleClick).sensor);
  if (temp != (*doubleClick).upDown && (millis() - (*doubleClick).upDownTime) > DEBOUNCE_TIME) {
    (*doubleClick).upDown = temp;
    (*doubleClick).upDownTime = millis();
    //Serial.println((*doubleClick).upDown ? "NOT PRESSED" : "PRESSED" );

    ((*doubleClick).buttonState)++;

    /* Handles edge case, with invalid state */
    if ((*doubleClick).upDown == HIGH && (*doubleClick).buttonState % 2 == 1)
      (*doubleClick).buttonState = 0;

    /* Handles edge case where 2nd button click is held down for too long */
    if ((*doubleClick).upDown == LOW && (*doubleClick).buttonState % 2 == 0)
      (*doubleClick).buttonState = 0;

    /* First click is done, start timeout counter */
    if ((*doubleClick).buttonState == 1) {
      (*doubleClick).doubleClickTimeout = millis();
    }

    /* Has been turned off and on again */
    if ((*doubleClick).buttonState == 4) {
      Serial.print("Button[");
      Serial.print((*doubleClick).sensor);
      Serial.println("] double clicked!");
      (*doubleClick).buttonState = 0;
    }
  }
}


