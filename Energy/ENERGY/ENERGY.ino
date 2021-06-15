#include <Wire.h>
#include <INA219_WE.h>
#include <SPI.h>
#include <SD.h>

INA219_WE ina219; // this is the instantiation of the library for the current sensor

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

const int chipSelect = 10;
unsigned int rest_timer;
unsigned int loop_trigger;
unsigned int int_count = 0; // a variables to count the interrupts. Used for program debugging.
float kpv=0.05024,kiv=15.78,kdv=0; // voltage pid.
float u0v,u1v,delta_uv,e0v,e1v,e2v; // u->output; e->error; 0->this time; 1->last time; 2->last last time
float kpi=0.02512,kii=39.4,kdi=0; // current pid.
float u0i,u1i,delta_ui,e0i,e1i,e2i; // Internal values for the current controller
float uv_max=4, uv_min=0; //anti-windup limitation
float ui_max=1, ui_min=0; //anti-windup limitation
float Ts = 0.001; //1 kHz control frequency.
float current_measure, current_ref = 0,old_current=0, error_amps, current_limit, ev, vref, cv, cc; // Control
float pwm_out;
float V_Bat;
boolean input_switch;
int state_num=0,next_state;
String dataString;
float P_old=0,P_new=0;
int mppt_stage,next_stage;
float SOH=100,DODA, DODB, DODC;
float OCVA[]={2618,3005,3127,3189,3205,3229,3238,3249,3258,3264,3268,3272,3276,3278,3280,3282,3285,3295,3305,3308,3310};
float OCVB[]={2600,3005,3071,3148,3201,3220,3233,3241,3246,3254,3260,3266,3269,3272,3275,3280,3291,3297,3303,3305,3308};
float OCVC[]={2610,2965,3139,3168,3199,3217,3230,3245,3255,3258,3262,3267,3272,3279,3285,3293,3296,3299,3303,3304,3306};
float CapA=1.848e3,CapB=1.8788e3,CapC,nomcap=1.8e3;
float V_A,V_B,V_C=0;
float SOC,SOCA,SOCB,SOCC;
bool Acheck=false, Bcheck=false, Ccheck=false, NULLcheck=false;
float old_pwm=0, old2_pwm=0;
bool warning=false;
float meancurrent=0,meanAvoltage=0,meanBvoltage=0;
bool dischA=false,dischB=false;
bool DIS_STATE=false,CHA_STATE=true;
float currentout=0, mtrspeed=0;
float range=0;
float charge=3600;
int cycle=0;
//Comms Variables

const uint32_t SENDING_PERIOD = 100; //how often the message is sent in milliseconds

byte esp_sending_data[4] = {B00000000,B00000000,B00000000,B00000000};
byte esp_received_data[5] = {B00000000,B00000000,B00000000,B00000000,B00000000};

int charge_placeholder = 50;
int charge_rate_placeholder = 69;
int range_placeholder = 12345;

///////////////

void setup() {
  //Some General Setup Stuff

  Serial1.begin(115200);
  Serial.begin(115200);

  Wire.begin(); // We need this for the i2c comms for the current sensor
  Wire.setClock(700000); // set the comms speed for i2c
  ina219.init(); // this initiates the current sensor
  Serial.begin(9600); // USB Communications


  //Check for the SD Card
  Serial.println("\nInitializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("* is a card inserted?");
    while (true) {} //It will stick here FOREVER if no SD is in on boot
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  if (SD.exists("BatCycle.csv")) { // Wipe the datalog when starting
    SD.remove("BatCycle.csv");
  }

  
  noInterrupts(); //disable all interrupts
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC

  //SMPS Pins
  pinMode(13, OUTPUT); // Using the LED on Pin D13 to indicate status
  pinMode(2, INPUT_PULLUP); // Pin 2 is the input from the CL/OL switch
  pinMode(6, OUTPUT); // This is the PWM Pin

  //LEDs on pin 7 and 8
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  //Analogue input, the battery voltage (also port B voltage)
  pinMode(A0, INPUT);
  pinMode(A6, INPUT);


  // TimerA0 initialization for 1kHz control-loop interrupt.
  TCA0.SINGLE.PER = 999; //
  TCA0.SINGLE.CMP1 = 999; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz

  interrupts();  //enable interrupts.
  analogWrite(6, 120); //just a default state to start with

}

void loop() {

  if (loop_trigger%100==0 & loop_trigger>0){
    if (Serial1.available() > 0) {
        Serial1.readBytes(esp_received_data, 5);
    }

    static uint32_t previousMillis;
    if (millis() - previousMillis >= SENDING_PERIOD) {
      send_data(SOC, SOH, range);
      previousMillis += SENDING_PERIOD;
    }
    mtrspeed+=get_speed()/10;
    currentout+=get_current()/10;
  }
  
  if (loop_trigger == 1){ // FAST LOOP (1kHZ)
      state_num = next_state; //state transition
      V_B = analogRead(A2)*4.096/1.03; //check the battery voltage (1.03 is a correction for measurement error, you need to check this works for you)
      V_A = 2*analogRead(A3)*4.096/1.03-V_B;
      if ((V_A > 3700 || V_B > 3700 || V_A < 2400 || V_B < 2400)) { //Checking for Error states (just battery voltage for now)
          state_num = 5; //go directly to jail
          next_state = 5; // stay in jail
          digitalWrite(7,true); //turn on the red LED
          current_ref = 0; // no current
      }
      current_measure = (ina219.getCurrent_mA()); // sample the inductor current (via the sensor chip)
      if(V_A>V_B){
        ev = (vref - V_A)/1000;  //voltage error at this time
      }else{
        ev = (vref - V_B)/1000;  //voltage error at this time
      }
      cv=pidv(ev);  //voltage pid

      error_amps = (current_ref - current_measure) / 1000; //PID error calculation
      cc = pidi(error_amps); //Perform the PID controller calculation
      if(state_num==2){
        pwm_out=saturation(cv,0.99,0.01);  //duty_cycle saturation
      }else if(state_num!=1){
        pwm_out = saturation(cc, 0.99, 0.01); //duty_cycle saturation
      }
      analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
      int_count++; //count how many interrupts since this was last reset to zero
      loop_trigger = 0; //reset the trigger and move on with life
      meancurrent+=current_measure/1000;
      meanAvoltage+=V_A/1000;
      meanBvoltage+=V_B/1000;
  }

  if (int_count==1000){
    if(meanAvoltage>(meanBvoltage+20)){
      digitalWrite(5,true);
      digitalWrite(6,false);
      dischA=true;
      dischB=false;      
    }else if(meanBvoltage>(meanAvoltage+20)){
      digitalWrite(5,false);
      digitalWrite(6,true);
      dischA=false;
      dischB=true;
    }else{
      digitalWrite(5,false);
      digitalWrite(6,false);
      dischA=false;
      dischB=false;
    }
  }
  
  if (int_count==1000) { // SLOW LOOP (1Hz)
    input_switch = digitalRead(2); //get the OL/CL switch status
    switch (state_num) { // STATE MACHINE (see diagram)
      case 0:{ // Start state (no current, no LEDs)
        current_ref = 0;
        if (input_switch == 1 & rest_timer>500) { // if switch, move to charge
          next_state = 1;
//          pwm_out=0;
          P_old=0;
          old_pwm=0;
          old2_pwm=0;

          for (int lookup=0; lookup<sizeof(OCVA); lookup++){
            if(V_A>OCVA[lookup]){//OCV and SOC_lookup in increasing order
              SOCA=lookup*5;
            }
            if(V_B>OCVB[lookup]){//OCV and SOC_lookup in increasing order
              SOCB=lookup*5;
            }
            if(V_C>OCVC[lookup]){//OCV and SOC_lookup in increasing order
              SOCC=lookup*5;
            }
          }
          digitalWrite(8,true);
        } else { // otherwise stay put
          next_state = 0;
          digitalWrite(8,false);
          rest_timer++;
        }
        break;
      }
      case 1:{ // Charge state (250mA and a green LED)
        old2_pwm=old_pwm;
        old_pwm=pwm_out;
        P_old=P_new;
        P_new=meancurrent*(V_B+V_A);
        if(meancurrent>250){
          if(warning==true){
            pwm_out=0.2;
            P_old=0;
            old_pwm=0;
            old2_pwm=0;
            next_state==1;
          }else{
            next_state=1;
            warning=true;
            if(pwm_out-old2_pwm>0){
              pwm_out-=0.05;
            }else{
              pwm_out+=0.05;
            }
          }
        }else{
          warning=false;
          if(abs(P_new-P_old)<0.1){
            digitalWrite(8,true);
          }else if(P_new-P_old>0){         
            if(pwm_out-old2_pwm>0){
              pwm_out+=0.01;
            }else{
              pwm_out-=0.01;
            }
          }else if( P_new-P_old<0){ 
            if(pwm_out-old2_pwm>0){
              pwm_out-=0.01;
            }else{
              pwm_out+=0.01;
            }
          }else{
            pwm_out+=0.01;
          }  
          next_state=1;      
        }
        pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
        if (meanAvoltage < 3600 and meanBvoltage < 3600) { // if not charged, stay put
          next_state = 1;
          digitalWrite(8,true);          
        } else { // otherwise go to charge rest
          next_state = 2;
          rest_timer=0;
          digitalWrite(8,false);
        }
        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(8,false);
        }
        break;
      }

      case 2:{ // Charge state (3.6V and a green LED)  CV
        vref=3600;
        current_limit=250;
        current_ref = 250;
        if (abs(meancurrent)<(0.03*abs(current_limit)) and rest_timer>60*10) { // Stay here if timer < 30
          next_state = 3;
          digitalWrite(8,false);
          rest_timer=0;
          if(meanAvoltage>meanBvoltage){
            SOCA=100;
          }else{
            SOCB=100;
          }
        } else { // Or move to discharge (and reset the timer)
          next_state = 2;
          digitalWrite(8,false);
          rest_timer++;
          if(rest_timer%2==0) digitalWrite(8,true);
        }
        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(8,false);
        }
        break;        
      }
      
      case 3:{ // Charge Rest, green LED is off and no current
        current_ref = 0;
        if (rest_timer < 30) { // Stay here if timer < 30
          next_state = 3;
          digitalWrite(8,false);
          rest_timer++;
        } else { // Or move to discharge (and reset the timer)
          next_state = 4;
          if(cycle>0){
            SOH=100*(charge/1000)/nomcap;
            charge=0;
          }
          digitalWrite(8,false);
          rest_timer = 0;
          CHA_STATE=false;
        }
        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(8,false);
        }
        break;        
      }

      case 4:{// STICK HERE UNTIL FULLY DISCHARGED
        current_ref = 0;
        CHA_STATE=get_charge_instruction();
        meancurrent=-1*currentout;
        if(meanAvoltage<2500){
          SOCA=0;
        }
        if(meanBvoltage<2500){
          SOCB=0;  
        }
        if(CHA_STATE==true){
          next_state==1;
          cycle+=1;
          SOH=100*(charge/1000)/nomcap;
          charge=0;
        }
      }
      
      case 5: { // ERROR state RED led and no current
        current_ref = 0;
        next_state = 5; // Always stay here
        digitalWrite(7,true);
        digitalWrite(8,false);
        if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(7,false);
        }
        break;
      }

      default :{ // Should not end up here ....
        Serial.println("Boop");
        current_ref = 0;
        next_state = 5; // So if we are here, we go to error
        digitalWrite(7,true);
      }
      
    }
    
    dataString = String(P_new) + "," + String(V_Bat) + "," + String(current_measure) + "," + String(pwm_out)+","+String(state_num)+","+String(V_A)+","+String(V_B)+","+String(SOCA)+","+String(SOCB); //build a datastring for the CSV file
    Serial.println(dataString); // send it to serial as well in case a computer is connected
    File dataFile = SD.open("BatCycle.csv", FILE_WRITE); // open our CSV file
    if (dataFile){ //If we succeeded (usually this fails if the SD card is out)
      dataFile.println(dataString); // print the data
    } else {
      Serial.println("File not open"); //otherwise print an error
    }
    dataFile.close(); // close the file
  }
  
  if (int_count==1000){
    int_count=0;

    range=mtrspeed*min(SOCA*CapA,SOCB*CapB)/currentout;
    
    if(state_num!=0){
      SOCA=SOCA+((meancurrent/1000-dischA*(meanAvoltage)/(1500*1000))/(CapA))*100;
      SOCB=SOCB+((meancurrent/1000-dischB*(meanBvoltage)/(1500*1000))/(CapB))*100;
      if(state_num!=4){
        SOC=max(SOCA,SOCB);//When charging the SOC is the larger value
        if(cycle>0){            //If not on first partial charge...
          charge+=meancurrent;  //Then integrate the charge entering the cells
        }
      }else{
        SOC=min(SOCA,SOCB);//When discharging we are concerned with the smaller SOC
        charge+=currentout;     //On discharge integrate the charge leaving the cells
      }
    }
  }
    meancurrent=0;
    meanAvoltage=0;
    meanBvoltage=0;
    mtrspeed=0;
    currentout=0;
}

// Timer A CMP1 interrupt. Every 1000us the program enters this interrupt. This is the fast 1kHz loop
ISR(TCA0_CMP1_vect) {
  loop_trigger = 1; //trigger the loop when we are back in normal flow
  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
}

float saturation( float sat_input, float uplim, float lowlim) { // Saturation function
  if (sat_input > uplim) sat_input = uplim;
  else if (sat_input < lowlim ) sat_input = lowlim;
  else;
  return sat_input;
}

float pidi(float pid_input) { // discrete PID function
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

float pidv( float pid_input){
  float e_integration;
  e0v = pid_input;
  e_integration = e0v;
 
  //anti-windup, if last-time pid output reaches the limitation, this time there won't be any intergrations.
  if(u1v >= uv_max) {
    e_integration = 0;
  } else if (u1v <= uv_min) {
    e_integration = 0;
  }

  delta_uv = kpv*(e0v-e1v) + kiv*Ts*e_integration + kdv/Ts*(e0v-2*e1v+e2v); //incremental PID programming avoids integrations.there is another PID program called positional PID.
  u0v = u1v + delta_uv;  //this time's control output

  //output limitation
  saturation(u0v,uv_max,uv_min);
  
  u1v = u0v; //update last time's control output
  e2v = e1v; //update last last time's error
  e1v = e0v; // update last time's error
  return u0v;
}

bool get_charge_instruction(){
  int modulator = (int)(esp_received_data[4]);

  if(modulator == 1){
    return true;
  }

  return false;
}


int get_current(){
  return (int)(word(esp_received_data[3], esp_received_data[2]));
}

int get_speed(){
  return (int)(word(esp_received_data[1], esp_received_data[0]));
}

void send_data(int charge, int charge_rate, int range){

  esp_sending_data[3] = (byte)(charge % 256);
  esp_sending_data[2] = (byte)(charge_rate % 256);  
   
  esp_sending_data[1]= (byte)(range / 256);
  esp_sending_data[0]= (byte)(range % 256);   

  Serial1.write(esp_sending_data, 4);
  
  //Serial.print("data sent:  "); // Test code
  //Serial.write(esp_sending_data, 3);
  //Serial.print("\n");
}
