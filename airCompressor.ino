#define VERSION "Compressor 02jan17"
#undef DEBUG
/* Alex Barrick Benjamin Ramirez,
  //october 19 2016
  //Smart Air Compressor
  01 Jan 17 BR, from "Compressor 4 DEC 2016"
  Note: if compressor switch is off and arduino is powered from USB port, the pressor sensor isn't powered
  and will read ~-19psi.
  Note: with a 2 second READ_INTERVAL, serial commands will generally take at READ_INTERVAL to respond
  Note: serial command set, valid commands begin with "c ", reducing the likelihood noise can generate commands,
  as well as enabling the serial port to be shared with devices with other cmd sets
*/
int PRESSURE_PIN = A0; //Pressure reading from the Air compressor, set to A1 for debugging
const int LED_PIN = 4; // Button for manually switching states
const int RELAY_PIN = 5; // if ON it controls the relay that lets current flow.

#define HOURS *60*60L*1000
#define MINUTES *60L*1000
#define SECONDS *1000
#define ON HIGH
#define OFF LOW
const int MIN_P = 40; //value in PSI Turns on Compressor
const int MAX_P = 75; //max PSI , turns off compressor
const int READ_INTERVAL = 2 SECONDS;  // check the sensor and run loop this often
#ifdef DEBUG
unsigned long REBOOT_TIME = 5 MINUTES; // exit killed state this often
unsigned long MAX_ON_TIME = 1 MINUTES; // kill motor if don't reach MAX_P before this
#else if 
unsigned long REBOOT_TIME = 12 HOURS; // exit killed state this often
unsigned long MAX_ON_TIME = 6 MINUTES; // kill motor if don't reach MAX_P before this
#endif
enum states { off = OFF, on = ON, killed = 2};
enum states MOTOR; //holds the current state of the motor
volatile int psi;
unsigned long startTime, timeSpent;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, OFF);
  pinMode(PRESSURE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println(VERSION);
  turnMotorOff();
}   // setup

void loop() {
  timeSpent = millis() - startTime;
  psi = adu2psi(averageReading(100));

  if (MOTOR != killed) // heart_beat
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

  if (MOTOR == on && psi >= MAX_P)
    turnMotorOff();
  else if (MOTOR == on && timeSpent > MAX_ON_TIME)   // doesn't matter what psi is
    killMotor();
  else if (MOTOR == off && psi <  MIN_P)
    turnMotorOn();
  else if (MOTOR == killed && timeSpent > REBOOT_TIME)
    turnMotorOn();
  checkForSerialCmd();
  delay(READ_INTERVAL);
} // loop

int averageReading(int numTimes) {
  float sum = 0.0;
  for (int i = 0; i < numTimes; i++) {
    sum += analogRead(PRESSURE_PIN);
    delayMicroseconds(random(16667)); // average out 60Hz noise and harmonics
  }
  return (sum / numTimes);
} // averageReading

int adu2psi(int reading) { //mapping the adu reading to PSI
  return map(reading, 172, 944, 0, 100);  // empirical params from pressure gauge
}

void turnMotorOn() {
  digitalWrite(RELAY_PIN, ON);
  MOTOR = on;
  startTime = millis();
  Serial.print("turning ON after (min): ");
  Serial.println((float)timeSpent / (1 MINUTES), 1);
}

void turnMotorOff() {
  digitalWrite(RELAY_PIN, OFF);
  MOTOR = off;
  startTime = millis();
  Serial.print("turning OFF after (min): ");
  Serial.println((float)timeSpent / (1 MINUTES), 1);
}

void killMotor() {
  digitalWrite(RELAY_PIN, OFF);
  MOTOR = killed;
  startTime = millis();
  Serial.print("Killing motor after ");
  Serial.print(MAX_ON_TIME / (1 SECONDS));
  Serial.print(" seconds, psi = ");
  Serial.println(psi);
  digitalWrite(LED_PIN, ON); // solid on
}

void checkForSerialCmd()
{
  char  buf[10] = "";
  /*
      serial command set:
      c s -- get status
      c o -- turn on motor
      c w -- turn off motor
      c k -- kill motor
  */
  if (!Serial.available()) return;
  Serial.readBytes(buf, sizeof(buf));
  if (!((buf[0] == 'c') && (buf[1] == ' '))) return;
  switch (buf[2]) {
    case 's': {
        Serial.print("Motor = ");
        if (MOTOR == killed) Serial.print("killed");
        else Serial.print(MOTOR ? "ON" : "OFF");
        Serial.print(", psi = ");
        Serial.print(psi);
        Serial.print(", timeSpent in present state (min) = ");
        Serial.println((float)timeSpent / (1 MINUTES), 1);
        break;
      }
    case 'o': {
        turnMotorOn();
        break;
      }
    case 'w': {
        turnMotorOff();
        break;
      }
    case 'k': {
        killMotor();
        break;
      }
  }
}  // checkForSerialCmd
