/*
Convert output of DTMF converter to pulses for telephone exchange.
for Look Mum No Computer
https://www.patreon.com/posts/im-aware-this-66205770
https://microcontrollerslab.com/mt8870-dtmf-decoder-module-pinout-interfacing-with-arduino-features/#Pin_Configuration

Cobbled together on a discord chat by:
Caustic
https://github.com/CausticCatastrophe

rgm3
https://github.com/rgm3

Curlymorphic
https://github.com/curlymorphic

MarCNet (for the tip with disabling interrupts while writing to the queue)

*/

#include <Arduino.h>

// PIN DEFINITIONS
#define PULSE_PIN 7 // The pin on the arduino that send out the pulse code.
#define stq_pin 2 //for nano, can only use D2 or D3 for interrupt pins.
#define q1_pin 3
#define q2_pin 4
#define q3_pin 5
#define q4_pin 6

// Time constants
const float mult_time = 1; // use this to slow down the output for testing.
const int pulse_length_make = 33 * mult_time;
const int pulse_length_break = 66 * mult_time;
const int pulse_hangup_delay = 1000; // the time taken for a hangup.
const int interdigit_gap = 300; // the time between digits when sending out.

// Timeouts
const int user_idle_timeout = 60*1000*15; // 15 minutes
const int DIAL_DONE_TIMEOUT_MS = 2000;
unsigned long pulse_done_time = 0;

// The last time the user pressed the hash
long last_hash_time = 0;

// The last time the user dialed an input
unsigned long last_dialed_time = 0 ;

// Dial buffer vars
const int DIAL_BUFFER_LEN = 32;
char g_dial_buffer[DIAL_BUFFER_LEN] = "";
int buffer_position = 0;

// interrupt flag that says if the dtmf has new goodies
volatile bool dtmf_received = false; 

// Stores a hangup state
bool is_hung_up = true;

// Declare functions
void dtmf_interrupt();
void hang_up();
char read_dtmf_inputs();
void clear_buffer();
void number_pulse_out(int);

void setup() {
  Serial.begin(9600);
  Serial.println("Hello, im alive!");

  /*Define input pins for DTMF Decoder pins connection */
  pinMode(stq_pin, INPUT); // connect to Std pin
  pinMode(q4_pin, INPUT); // connect to Q4 pin
  pinMode(q3_pin, INPUT); // connect to Q3 pin
  pinMode(q2_pin, INPUT); // connect to Q2 pin
  pinMode(q1_pin, INPUT); // connect to Q1 pin

  // Attaches interrupt to set flag with dtmf_interrupt
  attachInterrupt(digitalPinToInterrupt(stq_pin), dtmf_interrupt, FALLING);

  hang_up();
}
bool done=false;

void loop() {
  // TESTING FOR MANUAL INTERRUPT
  /*
  if (!done){
    if (millis() > 4000) 
    {
      Serial.println("before interrupt call");
      // testy test for the interrupts
      dtmf_interrupt();
      done=true;
    }
  }
  */

  if (dtmf_received) {
    // Resets interrupt flag for next input.
    dtmf_received = false;

    // Persist this time, so we know the last time we have received a dial.
    last_dialed_time = millis();

    //Prevent queue being modified while we're reading stuff
    noInterrupts();

    // add_key() will add the given key to the queue. 
    // read_dtmf_inputs() will read the input pins and output the char (0-9, #, *)
    add_key(read_dtmf_inputs());

    //Turn interrupts back on
    interrupts();
  }

  // If enough time has elapsed since last DTMF, send buffer via pulses
  if ((millis() - last_dialed_time) > DIAL_DONE_TIMEOUT_MS) {
    pulse_exchange(g_dial_buffer, buffer_position);
    buffer_position = 0;
    clear_buffer();
  }

  // Hang up if the user hasnt pushed keys in the allotted time.
  if ( pulse_done_time > 0 && ( ( millis() - pulse_done_time ) > user_idle_timeout ) )
  {
    hang_up();
  }
}

void pulse_exchange(char buf[], int num_chars) {
  for(int i = 0; i < num_chars; i++) {
    switch (buf[i]) {
      case '#':
        hang_up();
        break;
      case '*':
        // TODO: Implement me. (Or not im not your dad).     
        break;
      case '0':
        number_pulse_out(10);
        break;
      default:
        //convert from ascii char to int
        int count = buf[i] - 0x30;
        number_pulse_out(count);
        break;
    }
  }
}

void number_pulse_out(int number_pressed) 
{
  // Simulate a phone pickup
  digitalWrite(PULSE_PIN, LOW);
  delay(interdigit_gap);
  is_hung_up = false;
  // send an appropriate pulse out for the specified number.
  for (int i = 0; i < number_pressed; i++)
  {
    digitalWrite(PULSE_PIN, HIGH);
    delay(pulse_length_make);
    digitalWrite(PULSE_PIN, LOW);
    delay(pulse_length_break);
  }
  pulse_done_time = millis();
}

void hang_up()
{
  if (is_hung_up){
    return;
  }
  is_hung_up = true;
  // hang up when startup
  Serial.println("hangup signal");
  digitalWrite(PULSE_PIN, HIGH);
  delay(pulse_hangup_delay);
  digitalWrite(PULSE_PIN, LOW);
  delay(pulse_hangup_delay);
  delay(pulse_hangup_delay);
  digitalWrite(PULSE_PIN, HIGH);
}

void clear_buffer() {
  for (int i = 0; i < DIAL_BUFFER_LEN; i++) {
    g_dial_buffer[i] = 0;
  }
  g_dial_buffer[0] = '\0';
}

void dtmf_interrupt() {
  // Set flag, avoid function calls in interrupt
  dtmf_received = 1;
}

/*
 * Returns character of the button pressed, 0-9, #, *.
 * Returns 'E' for error.
 */
char read_dtmf_inputs() {
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
      return '1';
      break;
    case 0x02:
      Serial.println("Button Pressed =  2");
      return '2';
      break;
    case 0x03:
      Serial.println("Button Pressed =  3");
      return '3';
      break;
    case 0x04:
    Serial.println("Button Pressed =  4");
      return '4';
      break;
    case 0x05:
      Serial.println("Button Pressed =  5");
      return '5';
      break;
    case 0x06:
      Serial.println("Button Pressed =  6");
      return '6';
      break;
    case 0x07:
      Serial.println("Button Pressed =  7");
      return '7';
      break;
    case 0x08:
      Serial.println("Button Pressed =  8");
      return '8';
      break;
    case 0x09:
      Serial.println("Button Pressed =  9");
      return '9';
      break;
    case 0x0A:
      Serial.println("Button Pressed =  0");
      return '0';
      break;
    case 0x0B:
      Serial.println("Button Pressed =  *");
      return '*';
      break;
    case 0x0C:
      Serial.println("Button Pressed =  #");
      return '#';
      break;
  }
  Serial.println("Erroneous button press");
  return 'E';
}

void add_key(char key){
  g_dial_buffer[buffer_position] = key;
  buffer_position++;
  Serial.println(g_dial_buffer);
}
  
