
const int pwmA = 33; 
const int pwmB = 25;

void setup() {
  // set the LED as an output
  pinMode(pwmA, OUTPUT);
  pinMode(pwmB, OUTPUT);
}

void loop(){

    analogWrite(pwmA, 255);
    analogWrite(pwmB, 0);
    delay(1000);

    analogWrite(pwmA, 0);
    analogWrite(pwmB, 255);
    delay(1000);

    analogWrite(pwmA, 127);
    analogWrite(pwmB, 0);
    delay(1000);

    analogWrite(pwmA, 0);
    analogWrite(pwmB, 127);
    delay(1000);

}