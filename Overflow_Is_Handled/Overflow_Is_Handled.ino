void setup() {
  Serial.begin(9600);
  Serial.println((uint32_t)((uint32_t) 4294967297 - (uint32_t) 4294967290 )); // 4294967296 is UINT32_MAX + 2
  Serial.println((uint32_t)((uint32_t)          1 - (uint32_t) 4294967290 )); // 1 is the 2nd value after overflow
}

void loop() {
}
