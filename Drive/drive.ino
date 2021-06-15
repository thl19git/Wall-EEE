#include <assert.h>
#include <Wire.h>
#include <INA219_WE.h>
#include <Math.h>

#include "SPI.h"


#define PIN_SS        10
#define PIN_MISO      12
#define PIN_MOSI      11
#define PIN_SCK       13

#define PIN_MOUSECAM_RESET     8
#define PIN_MOUSECAM_CS        7

#define ADNS3080_PIXELS_X                 30
#define ADNS3080_PIXELS_Y                 30

#define ADNS3080_PRODUCT_ID            0x00
#define ADNS3080_REVISION_ID           0x01
#define ADNS3080_MOTION                0x02
#define ADNS3080_PIXEL_SUM             0x06
#define ADNS3080_CONFIGURATION_BITS    0x0a
#define ADNS3080_MOTION_BURST          0x50
#define ADNS3080_PRODUCT_ID_VAL        0x17

//Global Vars for Optical Flow sensor
long total_x = 0;
long total_y = 0;

long total_x1 = 0;
long total_y1 = 0;

long distance_x = 0;
long distance_y = 0;

//End optical flow global vars

INA219_WE ina219; // this is the instantiation of the library for the current sensor

//SMPS control variables
float open_loop, closed_loop; // Duty Cycles
float vpd, vb, vref, iL, dutyref, current_mA; // Measurement Variables
unsigned int sensorValue0, sensorValue1, speed_ref, sensorValue3; // ADC sample values declaration
float ev = 0, cv = 0, ei = 0, oc = 0; //internal signals
float Ts = 0.0008; //1.25 kHz control frequency. It's better to design the control period as integral multiple of switching period.
float kpv = 0.05024, kiv = 15.78, kdv = 0; // voltage pid.
float u0v, u1v, delta_uv, e0v, e1v, e2v; // u->output; e->error; 0->this time; 1->last time; 2->last last time
float kpi = 0.02512, kii = 39.4, kdi = 0; // current pid.
float u0i, u1i, delta_ui, e0i, e1i, e2i; // Internal values for the current controller
float uv_max = 4, uv_min = 0; //anti-windup limitation
float ui_max = 1, ui_min = 0; //anti-windup limitation
float current_limit = 1.0;
boolean Boost_mode = 0;
boolean CL_mode = 0;



unsigned int loopTrigger;

int iSGN(int a);

//************************** Motor Constants **************************//
int DIRRstate = LOW;              //initializing direction states
int DIRLstate = HIGH;

int DIRL = 20;                    //defining left direction pin
int DIRR = 21;                    //defining right direction pin

int pwmr = 5;                     //pin to control right wheel speed using pwm
int pwml = 9;                     //pin to control left wheel speed using pwm
//*******************************************************************//

struct pos {
  float x;
  float y;
};

float saturation( float sat_input, float uplim, float lowlim);

//Communications global variables:
const uint32_t SENDING_PERIOD = 100; //how often the message is sent in milliseconds

byte esp_sending_data[10] = {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000};
byte esp_received_data[3] = {B00000000, B00000000, B00000000};

//End Communications global variables

bool driveturn = 0; //0 = Drive, 1 = Turn
bool FBLR = 0; //0 = Forwards/Left, 1 = Backwards/Right
int distangle = 0; //Distance to go/angle to turn
bool gostop = 0;
bool instrflag = 0;
bool instrflagold = 0;

int current;

bool commandstatus = 0; //0 = ready for command, 1 = executing command

bool gogogogo = 0;

pos startpos = {0, 0};

pos addvec(pos a, pos b);

int radius = 135; //Distance from optical flow sensor to centre of rotation (mm)

float ep2, ep1, ep0, up1; //posPID




//--------------------------ROVER CLASS------------------------------------
class Rov {
  public:
    int speed_perc = 30;
    int angle = 0;

    pos location;

    void go();
    void halt();
    void fore();
    void back();
    void left();
    void right();
    void set_speed(float perc);
    void set_vel(float perc);
    void update_angle(int newangle);
    void update_location();
    bool check_moving();


};

void Rov::go() {
  digitalWrite(pwmr, HIGH);
  digitalWrite(pwml, HIGH);
  this->moving = 1;
}


void Rov::halt() {
  digitalWrite(pwmr, LOW);
  digitalWrite(pwml, LOW);
  this->moving = 0;
}

void Rov::fore() {
  digitalWrite(DIRR, LOW);
  digitalWrite(DIRL, HIGH);
}

void Rov::back() {
  digitalWrite(DIRR, HIGH);
  digitalWrite(DIRL, LOW);
}

void Rov::left() {
  digitalWrite(DIRR, HIGH);
  digitalWrite(DIRL, HIGH);
}


void Rov::right() {
  digitalWrite(DIRR, LOW);
  digitalWrite(DIRL, LOW);
}

void Rov::set_speed(float perc) {
  if (perc == 0) {
    speed_ref = 0;
  } else {
    speed_ref = (perc * 6.48) + 375;
  }
  this->speed_perc = perc;
}

void Rov::set_vel(float perc) {
  int desspeed = saturation(perc, 100, -100);

  if (desspeed >= 0) {
    this->fore();
    this->set_speed(desspeed);
  } else {
    this->set_speed(-desspeed);
    this->back();
  }
}


//TODOTODOTODOTODO
void Rov::update_angle(int anglin) {

  if (!FBLR) {
    anglin = -anglin;
  }
  int newangle = angle + anglin;

  while (newangle > 360) {
    newangle = newangle - 360;
  }

  while (newangle < -360) {
    newangle = newangle + 360;
  }

  if (newangle < -180) {
    newangle = 360 + newangle;
  }

  if (newangle > 180) {
    newangle = newangle - 360;
  }

  this->angle = newangle;
}

void Rov::update_location() {

  pos newvec;

  newvec.x = -iSGN(FBLR) * distangle * sin(this->angle);
  newvec.y = -iSGN(FBLR) * distangle * cos(this->angle);

  if (abs(this->angle) >= 90) {
    newvec.y = -newvec.y;
  }

  if (this->angle < 0) {
    newvec.x = -newvec.x;
  }
  this->location = addvec(this->location, newvec);
}

//--------------------------END ROVER CLASS----------------------------
int speed_store = 0;
Rov Rover;

void setup() {
  Serial1.begin(115200);
  Serial.begin(115200);
  //************************** Motor Pins Defining **************************//
  pinMode(DIRR, OUTPUT);
  pinMode(DIRL, OUTPUT);
  pinMode(pwmr, OUTPUT);
  pinMode(pwml, OUTPUT);
  digitalWrite(pwmr, LOW);       //setting right motor speed at maximum
  digitalWrite(pwml, LOW);       //setting left motor speed at maximum
  //*******************************************************************//

  //Basic pin setups

  noInterrupts(); //disable all interrupts
  pinMode(13, OUTPUT);  //Pin13 is used to time the loops of the controller
  pinMode(3, INPUT_PULLUP); //Pin3 is the input from the Buck/Boost switch
  pinMode(2, INPUT_PULLUP); // Pin 2 is the input from the CL/OL switch
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC

  // TimerA0 initialization for control-loop interrupt.

  TCA0.SINGLE.PER = 999; //
  TCA0.SINGLE.CMP1 = 999; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output

  pinMode(6, OUTPUT);
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz
  analogWrite(6, 120);

  interrupts();  //enable interrupts.
  Wire.begin(); // We need this for the i2c comms for the current sensor
  ina219.init(); // this initiates the current sensor
  Wire.setClock(700000); // set the comms speed for i2c

  Rover.halt();

  //Optical Flow Setup:
  pinMode(PIN_SS, OUTPUT);
  pinMode(PIN_MISO, INPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_SCK, OUTPUT);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);

  if (mousecam_init() == -1) {
    Serial.println("Mouse cam failed to init");
    while (1);
  }

  Rover.set_speed(30);

}


void loop() {

  commswithESP();
  if (loopTrigger) { // This loop is triggered, it wont run unless there is an interrupt
    SMPS_Control_Loop();
    pos currentpos = Optical_Flow_Loop();//Does the optical flow stuff and assigns currentpos
    //Rover.update_location(currentpos);
    Drive(currentpos);

    digitalWrite(13, LOW);   // reset pin13.
    loopTrigger = 0;
  }
}//Loop end

void commswithESP() {
  if (Serial1.available() > 0) {

    Serial1.readBytes(esp_received_data, 3);
    if (get_distangle() > 5) {
     
      driveturn = get_drive_turn(); //0 = Drive, 1 = Turn
      FBLR = get_FBLR(); //0 = Forwards/Left, 1 = Backwards/Right
      distangle = get_distangle(); //Distance to go/angle to turn
      instrflag = get_instruction_flag(); //Instruction flag needs to change every instruction so it knows that you've moved on to a new one----
    }
  }

  //Send position every SENDING_PERIOD
  static uint32_t previousMillis;
  if (millis() - previousMillis >= SENDING_PERIOD) {
    send_data(Rover.location.x, Rover.location.y, Rover.angle, iL, Rover.speed_perc / 434000);
    previousMillis += SENDING_PERIOD;
  }
}


void instrdiag() {
  Serial.println("Driveturn: " + String(driveturn));
  Serial.println("FBLR: " + String(FBLR));
  Serial.println("Distangle: " + String(distangle));
  Serial.println("Instruction Flag: " + String(instrflag));
}

bool get_instruction_flag() {
  int modulator = (int)(esp_received_data[2]);

  if (modulator == 4 || modulator == 5 || modulator == 6 || modulator == 7) {
    return true;
  }
  return false;
}

bool get_drive_turn() {
  int modulator = (int)(esp_received_data[2]);

  if (modulator == 2 || modulator == 3 || modulator == 6 || modulator == 7) {
    return true;
  }

  return false;
}

bool get_FBLR() {
  int modulator = (int)(esp_received_data[2]);

  if (modulator == 1 || modulator == 3 || modulator == 5 || modulator == 7) {
    return true;
  }

  return false;
}

int get_distangle() {
  return (int)(word(esp_received_data[1], esp_received_data[0]));
}


void send_data(int x, int y, float angle, int current, int speed_mm) {

  int modulated_angle = (floor)(angle * 100);

  esp_sending_data[9] = (byte)(speed_mm / 256);
  esp_sending_data[8] = (byte)(speed_mm % 256);

  esp_sending_data[7] = (byte)(current / 256);
  esp_sending_data[6] = (byte)(current % 256);

  esp_sending_data[5] = (byte)(x / 256);
  esp_sending_data[4] = (byte)(x % 256);

  esp_sending_data[3] = (byte)(y / 256);
  esp_sending_data[2] = (byte)(y % 256);

  esp_sending_data[1] = (byte)(modulated_angle / 256);
  esp_sending_data[0] = (byte)(modulated_angle % 256);

  Serial1.write(esp_sending_data, 10);

  //Serial.print("data sent:  "); // Test code
  //Serial.write(esp_sending_data, 3);
  //Serial.print("\n");
}


void Drive(pos currentpos) {

  if (instrflag != instrflagold) {
    startpos = currentpos;
    speed_store = Rover.speed_perc;
    instrflagold = instrflag;
    commandstatus = 1;

    if (driveturn) {
      //TURN
      Rover.set_speed(10);
      if (FBLR) {
        //TURN RIGHT
        Rover.right();
      } else {
        //TURN LEFT
        Rover.left();
      }
    } else {
      //DRIVE
      if (FBLR) {
        Rover.back();
        //DRIVE BACKWARDS
      } else {
        Rover.fore();
        //DRIVE FORWARDS
      }
    }
    Rover.go();
  }

  if (commandstatus) {
    commandstatus = check_done(currentpos); //Checks if the current command is still executing. If it is, stop the rover and deassert the commandstatus bit
  }
}

float kup = 3;
float Tu = 0.33;

float kpp = 1;//kup*0.45;
float kip = 0;//0.54*kup/Tu;
float kdp = 0;//0.075*kup*Tu*0;



int iSGN(int a) {
  if (a == 0) {
    return 1;
  }
  return -1;
}

int SGN(int a) {
  if (a >= 0) {
    return 1;
  }
  return -1;
}


void PIDdiag(int cposy) {
  //Serial.println("Current position: " + String(currentpos.y));
  //Serial.println("Position Error: " + String(difpos));
  Serial.print("Current Position:");
  Serial.print(cposy);
  Serial.print(",");
  Serial.print("Setpoint:");
  Serial.print(100);
  Serial.print(",");
  Serial.print("Speed:");
  Serial.print(Rover.speed_perc);
  Serial.print(",");
  Serial.print("Error:");
  Serial.println(startpos.y  - cposy);
}


bool check_done(pos currentpos) {

  Rover.set_speed(0.5 * (distangle - (currentpos.y - startpos.y)) + 10);

  if ((driveturn && check_angle(currentpos)) || (!driveturn && check_dist(currentpos))) {
    Rover.halt();
    Rover.set_speed(speed_store);
    instrflag != instrflag;

    return 0;
  }
  return 1;
}

bool check_dist(pos currentpos) {

  //float dist = sqrt((currentpos.x - startpos.x) * (currentpos.x - startpos.x) + (currentpos.y - startpos.y) * (currentpos.y - startpos.y));
  float dist = abs(currentpos.y - startpos.y);

  Serial.println("Distance:" + String(dist));

  if (dist >= distangle) {
    Rover.update_location();
    Serial.println("Updated Location");
    Serial.println("Current Position:" + String(Rover.location.x) + "," + String(Rover.location.y) + "}" );
    return true;
  }
  return false;
}

bool check_angle(pos currentpos) {
  float arclength = abs(currentpos.x - startpos.x);
  int angle_turned = (arclength / radius) * 57296 / 1000; //Convert from arclength to degrees

  Serial.println("Current Angle:" + String(angle_turned));
  if (angle_turned >= distangle) {
    Rover.update_angle(distangle);
    return true;
  }
  return false;
}

pos addvec(pos a, pos b) {
  pos ret;
  ret.x = a.x + b.x;
  ret.y = a.y + b.y;
  return ret;
}


void SMPS_Control_Loop() {

  digitalWrite(13, HIGH);   // set pin 13. Pin13 shows the time consumed by each control cycle. It's used for debugging.
  sampling();

  current_limit = 3; // Buck has a higher current limit
  ev = vref - vb;  //voltage error at this time
  cv = pidv(ev); //voltage pid
  cv = saturation(cv, current_limit, 0); //current demand saturation
  ei = cv - iL; //current error
  closed_loop = pidi(ei); //current pid
  closed_loop = saturation(closed_loop, 0.99, 0.01); //duty_cycle saturation
  pwm_modulate(closed_loop); //pwm modulation
  // closed loop control path
}


// This, clears the incoming interrupt flag and triggers the main loop.
// Timer A CMP1 interrupt. Every 800us the program enters this interrupt.
ISR(TCA0_CMP1_vect) {
  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
  loopTrigger = 1;
}

// This subroutine processes all of the analogue samples, creating the required values for the main loop
void sampling() {
  // Make the initial sampling operations for the circuit measurements

  sensorValue0 = analogRead(A0); //sample Vb
  sensorValue3 = analogRead(A3); //sample Vpd
  current_mA = ina219.getCurrent_mA(); // sample the inductor current (via the sensor chip)

  vb = sensorValue0 * (4.096 / 1023.0); // Convert the Vb sensor reading to volts
  vref = speed_ref * (4.096 / 1023.0); // Create a vref value using speed_ref
  vpd = sensorValue3 * (4.096 / 1023.0); // Convert the Vpd sensor reading to volts


  iL = current_mA / 1000.0;
  dutyref = speed_ref * (1.0 / 1023.0);

}

float saturation( float sat_input, float uplim, float lowlim) { // Saturatio function
  if (sat_input > uplim) sat_input = uplim;
  else if (sat_input < lowlim ) sat_input = lowlim;
  else;
  return sat_input;
}

void pwm_modulate(float pwm_input) { // PWM function
  analogWrite(6, (int)(255 - pwm_input * 255));
}

// This is a PID controller for the voltage

float pidv( float pid_input) {
  float e_integration;
  e0v = pid_input;
  e_integration = e0v;

  //anti-windup, if last-time pid output reaches the limitation, this time there won't be any intergrations.
  if (u1v >= uv_max) {
    e_integration = 0;
  } else if (u1v <= uv_min) {
    e_integration = 0;
  }

  delta_uv = kpv * (e0v - e1v) + kiv * Ts * e_integration + kdv / Ts * (e0v - 2 * e1v + e2v); //incremental PID programming avoids integrations.there is another PID program called positional PID.
  u0v = u1v + delta_uv;  //this time's control output

  //output limitation
  saturation(u0v, uv_max, uv_min);

  u1v = u0v; //update last time's control output
  e2v = e1v; //update last last time's error
  e1v = e0v; // update last time's error
  return u0v;
}

// This is a PID controller for the current

float pidi(float pid_input) {
  float e_integration;
  e0i = pid_input;
  e_integration = e0i;

  //anti-windup
  if (u1i >= ui_max) {
    e_integration = 0;
  } else if (u1i <= ui_min) {
    e_integration = 0;
  }

  delta_ui = kpi * (e0i - e1i) + kii * Ts * e_integration + kdi / Ts * (e0i - 2 * e1i + e2i); //incremental PID programming avoids integrations.
  u0i = u1i + delta_ui;  //this time's control output

  //output limitation
  saturation(u0i, ui_max, ui_min);

  u1i = u0i; //update last time's control output
  e2i = e1i; //update last last time's error
  e1i = e0i; // update last time's error
  return u0i;
}


//-------------------------------Optical flow functions---------------------------------------------
int convTwosComp(int b) {
  //Convert from 2's complement
  if (b & 0x80) {
    b = -1 * ((b ^ 0xff) + 1);
  }
  return b;
}



int tdistance = 0;

void mousecam_reset()
{
  digitalWrite(PIN_MOUSECAM_RESET, HIGH);
  delay(1); // reset pulse >10us
  digitalWrite(PIN_MOUSECAM_RESET, LOW);
  delay(35); // 35ms from reset to functional
}


int mousecam_init()
{
  pinMode(PIN_MOUSECAM_RESET, OUTPUT);
  pinMode(PIN_MOUSECAM_CS, OUTPUT);

  digitalWrite(PIN_MOUSECAM_CS, HIGH);

  mousecam_reset();

  int pid = mousecam_read_reg(ADNS3080_PRODUCT_ID);
  if (pid != ADNS3080_PRODUCT_ID_VAL)
    return -1;

  // turn on sensitive mode
  mousecam_write_reg(ADNS3080_CONFIGURATION_BITS, 0x19);

  return 0;
}

void mousecam_write_reg(int reg, int val)
{
  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(reg | 0x80);
  SPI.transfer(val);
  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(50);
}

int mousecam_read_reg(int reg)
{
  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(reg);
  delayMicroseconds(75);
  int ret = SPI.transfer(0xff);
  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(1);
  return ret;
}

struct MD
{
  byte motion;
  char dx, dy;
  byte squal;
  word shutter;
  byte max_pix;
};


void mousecam_read_motion(struct MD * p)
{
  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(ADNS3080_MOTION_BURST);
  delayMicroseconds(75);
  p->motion =  SPI.transfer(0xff);
  p->dx =  SPI.transfer(0xff);
  p->dy =  SPI.transfer(0xff);
  p->squal =  SPI.transfer(0xff);
  p->shutter =  SPI.transfer(0xff) << 8;
  p->shutter |=  SPI.transfer(0xff);
  p->max_pix =  SPI.transfer(0xff);
  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(5);
}

// pdata must point to an array of size ADNS3080_PIXELS_X x ADNS3080_PIXELS_Y
// you must call mousecam_reset() after this if you want to go back to normal operation

byte frame[ADNS3080_PIXELS_X * ADNS3080_PIXELS_Y];

//End of Optical Flow functions

pos Optical_Flow_Loop() {

  int val = mousecam_read_reg(ADNS3080_PIXEL_SUM);
  MD md;

  mousecam_read_motion(&md);

  distance_x = md.dx; //convTwosComp(md.dx);
  distance_y = md.dy; //convTwosComp(md.dy);

  total_x1 = (total_x1 + distance_x);
  total_y1 = (total_y1 + distance_y);

  total_x = 10 * total_x1 / 157; //Conversion from counts per inch to mm (400 counts per inch)
  total_y = 10 * total_y1 / 157; //Conversion from counts per inch to mm (400 counts per inch)


  pos currentpos = {total_x, total_y};
  return currentpos;
}
