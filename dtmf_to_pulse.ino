// 1 pulse = 50ms on, 50ms off
int pulse_length_make = 33;
int pulse_length_break = 66;
int pulse_hangup_delay = 1000;
int interdigit_gap = 300;

//int hangup_timeout = 60*1000*15;
int hangup_timeout = 3000;

long last_hash_time = 0;

// pin config
#define stq_pin 3 //for nano, can only use D2 or D3 for interrupt pins.
#define q1_pin 4
#define q2_pin 5
#define q3_pin 6
#define q4_pin 7

#define output_pin 8


void setup() {
  Serial.begin(9600);
  Serial.println("Hello, I'm in a terminal!");
  Serial.println();

  /*Define input pins for DTMF Decoder pins connection */
  pinMode(stq_pin, INPUT); // connect to Std pin
  pinMode(q4_pin, INPUT); // connect to Q4 pin
  pinMode(q3_pin, INPUT); // connect to Q3 pin
  pinMode(q2_pin, INPUT); // connect to Q2 pin
  pinMode(q1_pin, INPUT); // connect to Q1 pin

  attachInterrupt(digitalPinToInterrupt(stq_pin), read_dtmf_inputs, FALLING);
}

void loop() {
  uint8_t number_pressed = 0x0C;
  number_pulse_out(number_pressed);
  Serial.println("we done!");
  long now = millis();
  long diff_times = (now-last_hash_time);
  if ( (diff_times > (hangup_timeout)) & (diff_times != 0) )
  {
    hang_up();
    Serial.println("you took too long!!!");
  }

}

void read_dtmf_inputs()
{
  Serial.println("Hello, I'm in read_dtmf_inputs()!");
  Serial.println();
  uint8_t number_pressed;
  delay(250);
  // checks q1,q2,q3,q4 to see what number is pressed.
  number_pressed = ( 0x00 | (digitalRead(q4_pin)<<0) | (digitalRead(q3_pin)<<1) | (digitalRead(q2_pin)<<2) | (digitalRead(q1_pin)<<3) );
  switch (number_pressed)
  {
    case 0x01:
    Serial.println("Button Pressed =  1");
    break;
    case 0x02:
    Serial.println("Button Pressed =  2");
    break;
    case 0x03:
    Serial.println("Button Pressed =  3");
    break;
    case 0x04:
    Serial.println("Button Pressed =  4");
    break;
    case 0x05:
    Serial.println("Button Pressed =  5");
    break;
    case 0x06:
    Serial.println("Button Pressed =  6");
    break;
    case 0x07:
    Serial.println("Button Pressed =  7");
    break;
    case 0x08:
    Serial.println("Button Pressed =  8");
    break;
    case 0x09:
    Serial.println("Button Pressed =  9");
    break;
    case 0x0A:
    Serial.println("Button Pressed =  0");
    break;
    case 0x0B:
    Serial.println("Button Pressed =  *");
    break;
    case 0x0C:
    Serial.println("Button Pressed =  #");
    last_hash_time = millis();
    break;    
  }
}

void number_pulse_out(int number_pressed) 
{
  if (number_pressed==0x0C) {
    hang_up();
  }
  
  for (int i = 0; i < number_pressed; i++)
  {
    digitalWrite(output_pin, HIGH);
    delay(pulse_length_make);
    digitalWrite(output_pin, LOW);
    delay(pulse_length_break);
    Serial.print("\nnumber_pressed=");
    Serial.print(number_pressed);
    Serial.print("\ni=");
    Serial.print(i);
    Serial.println();
  }
  delay(interdigit_gap);
}


void hang_up()
{
  digitalWrite(output_pin, HIGH);
  delay(pulse_hangup_delay);
  digitalWrite(output_pin, LOW);
  delay(pulse_hangup_delay);
  delay(pulse_hangup_delay);
  digitalWrite(output_pin, HIGH);
  return;
}
