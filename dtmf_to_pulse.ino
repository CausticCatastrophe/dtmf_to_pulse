/*
Convert output of MT8870 DTMF converter to pulses for telephone exchange.
for Look Mum No Computer

https://www.patreon.com/posts/im-aware-this-66205770
https://microcontrollerslab.com/mt8870-dtmf-decoder-module-pinout-interfacing-with-arduino-features/#Pin_Configuration
https://www.microsemi.com/document-portal/doc_download/127041-mt8870d-datasheet-oct2006

Cobbled together on a discord chat by:
Caustic
https://github.com/CausticCatastrophe

rgm3
https://github.com/rgm3

Curlymorphic
https://github.com/curlymorphic

MarCNet (for the tip with disabling interrupts while writing to the queue)

DAX (help sorting out the * short-circuiting)

*/

#include <Arduino.h>

// Change me to false for production
# define DEBUG false

// PIN DEFINITIONS
#define PULSE_PIN 8 // The pin on the arduino that sends out the pulse code.

// Delayed Steering (Output). Presents a logic high when a received tone-pair has been
// registered and the output latch updated; returns to logic low when the voltage on St/GT falls
// below VTSt.
#define std_pin 3   // StD pin (ready signal) on the MT8870 DTMF decoder. Nano can only have d2, d3 for interrupts.

#define q1_pin 4    // q1 pin on the MT8870 DTMF - Most significant bit in 4-bit output
#define q2_pin 5    // q2 pin on the MT8870 DTMF
#define q3_pin 6    // q3 pin on the MT8870 DTMF
#define q4_pin 7    // q4 pin on the MT8870 DTMF - Least significant bit in 4-bit output

// Time constants
const float mult_time = DEBUG ? 4 : 1; // use this to slow down the output for testing.

const int pulse_length_make = 50 * mult_time;
const int pulse_length_break = 50 * mult_time;
const int pulse_hangup_delay = 1000; // the time taken for a hangup.
const int interdigit_gap = 300; // the time between digits when sending out.

// Timeouts
const unsigned long user_idle_timeout = 60 * 1000 * 2; // 2 minutes
const int DIAL_DONE_TIMEOUT_MS = 400;
unsigned long pulse_done_time = 0;

// The last time the user pressed the hash
long last_hash_time = 0;

// The last time the user dialed an input
unsigned long last_dialed_time = 0 ;

// Dial buffer vars
const int DIAL_BUFFER_LEN = 32;
char g_dial_buffer[DIAL_BUFFER_LEN] = "";
int buffer_position = 0;

// Interrupt flag that says if the dtmf has new goodies
volatile bool dtmf_received = false;

// Stores a hangup state
bool is_hung_up = false;

// Declare functions
void dtmf_interrupt();
void hang_up();
char read_mt8870();
void clear_buffer();
void number_pulse_out(int);

void setup() {
  Serial.begin(9600);
  Serial.println("Begin setup()");
  
  // Define input pins for DTMF Decoder pins connection
  pinMode(PULSE_PIN, OUTPUT);
  pinMode(std_pin, INPUT); // connect to Std pin
  pinMode(q4_pin, INPUT); // connect to Q4 pin
  pinMode(q3_pin, INPUT); // connect to Q3 pin
  pinMode(q2_pin, INPUT); // connect to Q2 pin
  pinMode(q1_pin, INPUT); // connect to Q1 pin

  // Attaches interrupt to set flag with dtmf_interrupt
  attachInterrupt(digitalPinToInterrupt(std_pin), dtmf_interrupt, RISING);

  digitalWrite(PULSE_PIN, HIGH);
  Serial.println("Done setup()");
}

/**
 * Main loop
 * React to interrupt flag, read device, send pulses.
 * Hang-up when idle.
 */
void loop() {
  // This indicates whether the dtmf code is a star.
  bool star_pushed = false;

  if (dtmf_received) {
    // Resets interrupt flag for next input.
    dtmf_received = false;

    Serial.println();
    Serial.println("DTMF data is ready (interrupt fired on StD pin)");

    // Persist this time, so we know the last time we have received a dial.
    last_dialed_time = millis();

    // add_key() will add the given key to the queue.
    // read_mt8870() will read the input pins and output the char (0-9, #, *)
    char phone_character = read_mt8870();
    if (phone_character == '*') {
      Serial.println("* button received.");
      star_pushed = true;
      // this is where we can impliment the end message with *
      // send whats in the queue to pulse exchange.
    } else {
      add_key(phone_character);
    }

  }
  
  // If enough time has elapsed since last DTMF, send buffer via pulses
  bool afterOutputTime = ((millis() - last_dialed_time) > DIAL_DONE_TIMEOUT_MS);
  if (afterOutputTime || star_pushed) {
    pulse_exchange(g_dial_buffer, buffer_position);
    buffer_position = 0;
    clear_buffer();
  }

  // Hang up if the user hasn't pushed keys in the allotted time.
  // Don't hang up if the time between the last DTMF received and 
  // sending out the pulses has not expired.
  if (afterOutputTime && (pulse_done_time > 0 && ( ( millis() - pulse_done_time ) > user_idle_timeout ) ) ) {
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
        // Convert from ascii char to int
        int count = buf[i] - 0x30;

        if (count < 0 || count > 9) {
          Serial.println("Invalid character in buffer.");
          Serial.print("buf[");
          Serial.print(i);
          Serial.print("] = ");
          Serial.println(buf[i]);
        }
        number_pulse_out(count);
        break;
    }
  }
}

void number_pulse_out(int number_pressed) {
  Serial.print("Pulse digit: ");
  Serial.println(number_pressed);

  pick_up_phone();

  // Send an appropriate pulse out for the specified number.
  for (int i = 0; i < number_pressed; i++)
  {
    if (DEBUG) Serial.print("-");
    digitalWrite(PULSE_PIN, HIGH);
    delay(pulse_length_make);
    if (DEBUG) Serial.print("_");
    digitalWrite(PULSE_PIN, LOW);
    delay(pulse_length_break);
  }
  if (DEBUG) 
  {
    Serial.println();
    Serial.println("Done pulsing digit.");
  }

  // Keep track of when we're finished pulsing the output, so we can detect idle state
  pulse_done_time = millis();
}

void pick_up_phone() {
  // Simulate a phone pickup
  digitalWrite(PULSE_PIN, LOW);
  delay(interdigit_gap);
  is_hung_up = false;
}

void hang_up() {
  Serial.println("Hang up");

  digitalWrite(PULSE_PIN, HIGH);
  delay(pulse_hangup_delay);
  digitalWrite(PULSE_PIN, LOW);
  delay(pulse_hangup_delay);
  delay(pulse_hangup_delay);
  digitalWrite(PULSE_PIN, HIGH);
  is_hung_up = true;
}

void clear_buffer() {
  for (int i = 0; i < DIAL_BUFFER_LEN; i++) {
    g_dial_buffer[i] = 0;
  }
  g_dial_buffer[0] = '\0';
}

void dtmf_interrupt() {
  // Set flag, avoid function calls in interrupt
  dtmf_received = true;
}

/**
 * Return character of the button pressed, 0-9, #, *.
 */
char read_mt8870() {
  Serial.println("read_mt8870() pins Q1-Q4");

  uint8_t mt_output_code;

  // This delay is present in the example code, but I don't think it's needed.
  //delay(150);

  // Debug to help simulating DIP switches
  // Almost worth using sprintf...
  int Q1 = digitalRead(q1_pin);
  int Q2 = digitalRead(q2_pin);
  int Q3 = digitalRead(q3_pin);
  int Q4 = digitalRead(q4_pin);
  Serial.print("Q1-Q4: ");
  Serial.print(Q1);
  Serial.print(Q2);
  Serial.print(Q3);
  Serial.println(Q4);

  // Convert the 4-bit binary value on pins q1,q2,q3,q4 to decimal.
  // This is the number corresponding to the DTMF tone the MT8870 heard.
  // Adapted from:
  // https://microcontrollerslab.com/mt8870-dtmf-decoder-module-pinout-interfacing-with-arduino-features/#Arduino_Code
  mt_output_code = (
    0x00 |
     (digitalRead(q4_pin) << 0) |
     (digitalRead(q3_pin) << 1) |
     (digitalRead(q2_pin) << 2) |
     (digitalRead(q1_pin) << 3)
  );
  /*
     Example inputs:
     Q1 Q2 Q3 Q4   DEC   HEX
      0  0  0  0 = 0          (invalid - chip sends 10 for zero)
      0  0  0  1 = 1          "1" digit
      0  0  1  0 = 2
      0  0  1  1 = 3
      0  1  0  0 = 4
      ...
      1  0  1  0 = 10   0x0A  "0" digit
      1  0  0  0 = 11   0x0B  "*" symbol
      1  1  0  0 = 12   0x0C  "#" symbol
  */

  Serial.print("MT8870 chip output: ");
  Serial.println(mt_output_code);

  if (mt_output_code < 1 || mt_output_code > 12) {
    Serial.println("ERROR: value out of range [1..12]");
    return 'E';
  }

  switch (mt_output_code) {
    case 12:
      return '#';
      break;
    case 11:
      return '*';
      break;
    case 10:
      return '0';
      break;
    default:
      // chars are really numbers 0-255 and ASCII digits are sequential starting at 0
      // No conversion is required from integer to char here.
      return '0' + mt_output_code;
      break;
  }
}

void add_key(char key){
  bool valid_key = (key >= '0' && key <= '9') || key == '#' || key == '*';

  if (!valid_key) {
    Serial.print("Invalid key: ");
    Serial.println(key);
    return;
  }

  Serial.print("Add key to buffer: ");
  Serial.println(key);

  g_dial_buffer[buffer_position] = key;
  buffer_position++;

  Serial.print("g_dial_buffer = ");
  Serial.println(g_dial_buffer);

}
