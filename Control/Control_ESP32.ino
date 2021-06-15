//load the wifi library
#include <WiFi.h>


//network credentials
const char* ssid     = "/*insert SSID*/";
const char* password = "/*insert wifi password*/";

//set web server to port 4000
WiFiServer wifiServer(4000);

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 84);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional


//drive variables
byte drive_received_data[10] = {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000};
byte drive_sending_data[3] = {B00000000,B00000000,B00000000};

byte energy_received_data[4] = {B00000000,B00000000,B00000000,B00000000};
byte energy_sending_data[5] = {B00000000,B00000000,B00000000,B00000000,B00000000};

bool instruction_flag_placeholder = false; //changes on every new instruction, when this changes drive knows to begin a new instruction
bool drive_turn_placeholder = false; ////0 = Drive, 1 = Turn
bool FBLR_placeholder = false; //0 = Forwards/Left, 1 = Backwards/Right
int amount_placeholder = 0; // mm for drive, degrees for turn

bool charge_instruction_placeholder = false;
int current_placeholder = 100;
int speed_mm_placeholder = 456;


//vision variables
byte vision_received_data[24];
 
void setup() {

  Serial2.begin(115200);
  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to WiFi... ");
    Serial.println(ssid);
  }
 
  Serial.println("Connected to the WiFi network");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  wifiServer.begin();
}

 
void loop() {
 
  WiFiClient client = wifiServer.available();
  
  if (client) {
    Serial.print("Client connected ");
    Serial.println(client.remoteIP());
    String currentLine = "";
    while (client.connected()) {
 
      if (client.available()) {
        char c = client.read();
        if (c=='/') {
          break;
        }
        currentLine += c;
      }
 
      delay(10);
    }

    

    Serial.print("Command from Command subsystem: ");
    Serial.println(currentLine);
    
    int opcode = currentLine.toInt() & 0b11110000000000000000;
    int amount = (currentLine.toInt() & 0b1111111111111111);
    if (opcode == 655360) {
      Serial.println("info");
      amount = 0;
    } else if (opcode == 524288) {
      Serial.println("move");
      drive_turn_placeholder = false;
    } else if (opcode == 589824) {
      Serial.println("rotate");
      drive_turn_placeholder = true;
    }
    if (amount > 32767) {
      amount = -32768+(amount&0b111111111111111);
    }
    
    if (amount >= 0) {
      FBLR_placeholder = true;
    } else {
      FBLR_placeholder = false;
    }
    if (opcode == 589824) {
      amount /= 10;
    }
    amount_placeholder = abs(amount);
    Serial.println(amount);

    if (amount != 0 ) {
      instruction_flag_placeholder = !instruction_flag_placeholder;
      send_drive_data(instruction_flag_placeholder, drive_turn_placeholder, FBLR_placeholder, amount_placeholder);
  
      delay(100);
      instruction_flag_placeholder = !instruction_flag_placeholder;
      send_drive_data(instruction_flag_placeholder, drive_turn_placeholder, FBLR_placeholder, amount_placeholder);
      delay(100);
      Serial.println("Writing to Drive");
    }
    
    /////////////////////////////////////////////////////////////////////////////////////
    //Send rover position info to the Command system/////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    String pos_send = "";

    serial2Flush();
    delay(1000);
  
    if (Serial2.available() > 0) {
      Serial2.readBytes(drive_received_data, 10);
      Serial.println("x pos "+String(get_rover_x()));
      Serial.println("y pos "+String(get_rover_y()));
      Serial.println("angle "+String(get_rover_angle()));
    }
    
    int x_pos = get_rover_x();
    if (x_pos<0) {
      pos_send += '-';
    } else if (x_pos<10000) { 
      pos_send += '0';
    }
    if (x_pos<1000) pos_send += '0';
    if (x_pos<100) pos_send += '0';
    if (x_pos<10) pos_send += '0';
    pos_send += String(x_pos)+',';
    
    int y_pos = get_rover_y();
    if (y_pos<0) {
      pos_send += '-';
    } else if (y_pos<10000) { 
      pos_send += '0';
    }
    if (y_pos<1000) pos_send += '0';
    if (y_pos<100) pos_send += '0';
    if (y_pos<10) pos_send += '0';
    pos_send += String(y_pos)+',';

    int rot_pos = get_rover_angle();
    if (rot_pos<0) {
      pos_send += '-';
    } else if (rot_pos<1000) { 
      pos_send += '0';
    }
    if (rot_pos<100) pos_send += '0';
    if (rot_pos<10) pos_send += '0';
    pos_send += String(rot_pos)+',';

    int current = get_rover_current();
    if (current<10000) pos_send += '0';
    if (current<1000) pos_send += '0';
    if (current<100) pos_send += '0';
    if (current<10) pos_send += '0';
    pos_send += String(current)+',';

    int speed_mm = get_rover_speed_mm();
    if (speed_mm<10000) pos_send += '0';
    if (speed_mm<1000) pos_send += '0';
    if (speed_mm<100) pos_send += '0';
    if (speed_mm<10) pos_send += '0';
    pos_send += String(speed_mm)+'\0';
    
    client.println(pos_send);
    /////////////////////////////////////////////////////////////////////////////////////


    /////////////////////////////////////////////////////////////////////////////////////
    //Send rover energy info to the Command system///////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    send_energy_data(0,1,2);

    serial2Flush();
    delay(100);
    if (Serial2.available() > 0){
      Serial2.readBytes(energy_received_data, 4);
    }
    
    int charge = get_charge();
    int charge_rate = get_charge_rate();
    int range = get_range();

    String energy_send = "";
    if (charge<100) energy_send += '0';
    if (charge<10) energy_send += '0';
    energy_send += String(charge)+',';
    if (charge_rate<100) energy_send += '0';
    if (charge_rate<10) energy_send += '0';
    energy_send += String(charge_rate)+',';
    if (range<10000) energy_send += '0';
    if (range<1000) energy_send += '0';
    if (range<100) energy_send += '0';
    if (range<10) energy_send += '0';
    energy_send += String(range)+'\0';
    client.println(energy_send);
    /////////////////////////////////////////////////////////////////////////////////////


    /////////////////////////////////////////////////////////////////////////////////////
    //Send object pos x5 info to the Command system//////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    serialFlush();
    delay(100);
    if (Serial.available() > 0) {
      Serial.readBytes(vision_received_data,24);
    }

    String object_pos1 = "";
    bool object1_seen = see_red(); //get from vision
    int object1_x = get_red_x(); //get from vision
    int object1_y = get_red_y(); //get from vision

    if (object1_seen ) {
      object_pos1 += "1,";
    } else {
      object_pos1 += "0,";
    }
    if (object1_x<0) {
      object_pos1 += '-';
    } else if (object1_x<10000) { 
      object_pos1 += '0';
    }
    if (abs(object1_x)<1000) object_pos1 += '0';
    if (abs(object1_x)<100) object_pos1 += '0';
    if (abs(object1_x)<10) object_pos1 += '0';
    object_pos1 += String(abs(object1_x))+',';
    if (object1_y<0) {
      object_pos1 += '-';
    } else if (object1_y<10000) { 
      object_pos1 += '0';
    }
    if (abs(object1_y)<1000) object_pos1 += '0';
    if (abs(object1_y)<100) object_pos1 += '0';
    if (abs(object1_y)<10) object_pos1 += '0';
    object_pos1 += String(abs(object1_y))+'\0';
    client.println(object_pos1);
    /////////////////////////////////////////////////////////////////////////////////////
    String object_pos2 = "";
    bool object2_seen = see_yellow(); //get from vision
    int object2_x = get_yellow_x(); //get from vision
    int object2_y = get_yellow_y(); //get from vision
    
    if (object2_seen) {
      object_pos2 += "1,";
    } else {
      object_pos2 += "0,";
    }
    if (object2_x<0) {
      object_pos2 += '-';
    } else if (object2_x<10000) { 
      object_pos2 += '0';
    }
    if (abs(object2_x)<1000) object_pos2 += '0';
    if (abs(object2_x)<100) object_pos2 += '0';
    if (abs(object2_x)<10) object_pos2 += '0';
    object_pos2 += String(abs(object2_x))+',';
    if (object2_y<0) {
      object_pos2 += '-';
    } else if (object2_y<10000) { 
      object_pos2 += '0';
    }
    if (abs(object2_y)<1000) object_pos2 += '0';
    if (abs(object2_y)<100) object_pos2 += '0';
    if (abs(object2_y)<10) object_pos2 += '0';
    object_pos2 += String(abs(object2_y))+'\0';
    client.println(object_pos2);
    /////////////////////////////////////////////////////////////////////////////////////
    String object_pos3 = "";
    bool object3_seen = see_green(); //get from vision
    int object3_x = get_green_x(); //get from vision
    int object3_y = get_green_y(); //get from vision
    
    if (object3_seen) {
      object_pos3 += "1,";
    } else {
      object_pos3 += "0,";
    }
    if (object3_x<0) {
      object_pos3 += '-';
    } else if (object3_x<10000) { 
      object_pos3 += '0';
    }
    if (abs(object3_x)<1000) object_pos3 += '0';
    if (abs(object3_x)<100) object_pos3 += '0';
    if (abs(object3_x)<10) object_pos3 += '0';
    object_pos3 += String(abs(object3_x))+',';
    if (object3_y<0) {
      object_pos3 += '-';
    } else if (object3_y<10000) { 
      object_pos3 += '0';
    }
    if (abs(object3_y)<1000) object_pos3 += '0';
    if (abs(object3_y)<100) object_pos3 += '0';
    if (abs(object3_y)<10) object_pos3 += '0';
    object_pos3 += String(abs(object3_y))+'\0';
    client.println(object_pos3);
    /////////////////////////////////////////////////////////////////////////////////////
    String object_pos4 = "";
    bool object4_seen = see_blue(); //get from vision
    int object4_x = get_blue_x(); //get from vision
    int object4_y = get_blue_y(); //get from vision
    
    if (object4_seen) {
      object_pos4 += "1,";
    } else {
      object_pos4 += "0,";
    }
    if (object4_x<0) {
      object_pos4 += '-';
    } else if (object4_x<10000) { 
      object_pos4 += '0';
    }
    if (abs(object4_x)<1000) object_pos4 += '0';
    if (abs(object4_x)<100) object_pos4 += '0';
    if (abs(object4_x)<10) object_pos4 += '0';
    object_pos4 += String(abs(object4_x))+',';
    if (object4_y<0) {
      object_pos4 += '-';
    } else if (object4_y<10000) { 
      object_pos4 += '0';
    }
    if (abs(object4_y)<1000) object_pos4 += '0';
    if (abs(object4_y)<100) object_pos4 += '0';
    if (abs(object4_y)<10) object_pos4 += '0';
    object_pos4 += String(abs(object4_y))+'\0';
    client.println(object_pos4);
    /////////////////////////////////////////////////////////////////////////////////////
    String object_pos5 = "";
    bool object5_seen = see_pink(); //get from vision
    int object5_x = get_pink_x(); //get from vision
    int object5_y = get_pink_y(); //get from vision
    
    if (object5_seen) {
      object_pos5 += "1,";
    } else {
      object_pos5 += "0,";
    }
    if (object5_x<0) {
      object_pos5 += '-';
    } else if (object5_x<10000) { 
      object_pos5 += '0';
    }
    if (abs(object5_x)<1000) object_pos5 += '0';
    if (abs(object5_x)<100) object_pos5 += '0';
    if (abs(object5_x)<10) object_pos5 += '0';
    object_pos5 += String(abs(object5_x))+',';
    if (object5_y<0) {
      object_pos5 += '-';
    } else if (object5_y<10000) { 
      object_pos5 += '0';
    }
    if (abs(object5_y)<1000) object_pos5 += '0';
    if (abs(object5_y)<100) object_pos5 += '0';
    if (abs(object5_y)<10) object_pos5 += '0';
    object_pos5 += String(abs(object5_y))+'\0';
    client.println(object_pos5);
    /////////////////////////////////////////////////////////////////////////////////////
    
    client.stop();
    Serial.println("Client disconnected");
  }

  
}






int get_charge(){
  return (int)(energy_received_data[3]);
}

int get_charge_rate(){
  return (int)(energy_received_data[2]);
}

int get_range(){
  return (int)(word(energy_received_data[1], energy_received_data[0]));
}

int get_rover_speed_mm(){
  return (int)(word(drive_received_data[9], drive_received_data[8]));
}

int get_rover_current(){
  return (int)(word(drive_received_data[7], drive_received_data[6]));
}

int get_rover_x(){
  return (int)(word(drive_received_data[5], drive_received_data[4]));
}

int get_rover_y(){
  return (int)(word(drive_received_data[3], drive_received_data[2]));
}

float get_rover_angle(){
  return ( (int)(word(drive_received_data[1], drive_received_data[0])) / 100 );
}

void send_drive_data(bool instruction_flag, bool drive_turn, bool FBLR, int amount){
  int modulator = 0;

  if(!(instruction_flag) && !(drive_turn) && !(FBLR)){
    modulator = 0;
  }else if(!(instruction_flag) && !(drive_turn) && FBLR){
    modulator = 1;
  }else if(!(instruction_flag) && drive_turn && !(FBLR)){
    modulator = 2;
  }else if(!(instruction_flag) && drive_turn && FBLR){
    modulator = 3;
  }else if(instruction_flag && !(drive_turn) && !(FBLR)){
    modulator = 4;
  }else if(instruction_flag && !(drive_turn) && FBLR){
    modulator = 5;
  }else if(instruction_flag && drive_turn && !(FBLR)){
    modulator = 6;
  }else{
    modulator = 7;
  }
  
  drive_sending_data[2] = (byte)(modulator % 256);    
  drive_sending_data[1] = (byte)(amount / 256);
  drive_sending_data[0] = (byte)(amount % 256);   

  Serial2.write(drive_sending_data, 3);

  //Serial.print("data sent:  ");         //Test code
  //Serial.write(drive_sending_data, 3);
  //Serial.print("\n");
}

void send_energy_data(bool charge_instruction, int current, int speed_mm){

  int modulator = 0;

  if(charge_instruction){
    modulator = 1;
  }
  
  energy_sending_data[4] = (byte)(modulator % 256);  

  energy_sending_data[3] = (byte)(current / 256);
  energy_sending_data[2] = (byte)(current % 256);  
  
  energy_sending_data[1] = (byte)(speed_mm / 256);
  energy_sending_data[0] = (byte)(speed_mm % 256);   

  Serial2.write(energy_sending_data, 5);

  //Serial.print("data sent:  ");         //Test code
  //Serial.write(drive_sending_data, 3);
  //Serial.print("\n");
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// RED

bool see_red(){ //is red ball visible
  return bitRead(vision_received_data[22], 0);
}

int get_red_x(){
  return conv_2_comp(word(vision_received_data[19], vision_received_data[18]));
}

int get_red_y(){
  return conv_2_comp(word(vision_received_data[17], vision_received_data[16]));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// YELLOW

bool see_yellow(){ //is yellow ball visible
  return bitRead(vision_received_data[21], 4);
}

int get_yellow_x(){
  return conv_2_comp(word(vision_received_data[15], vision_received_data[14]));
}

int get_yellow_y(){
  return conv_2_comp(word(vision_received_data[13], vision_received_data[12]));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// GREEN

bool see_green(){ //is green ball visible
  return bitRead(vision_received_data[21], 0);
}

int get_green_x(){
  return conv_2_comp(word(vision_received_data[11], vision_received_data[10]));
}

int get_green_y(){
  return conv_2_comp(word(vision_received_data[9], vision_received_data[8]));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// BLUE

bool see_blue(){ //is blue ball visible
  return bitRead(vision_received_data[20], 4);
}

int get_blue_x(){
  return conv_2_comp(word(vision_received_data[7], vision_received_data[6]));
}

int get_blue_y(){
  return conv_2_comp(word(vision_received_data[5], vision_received_data[4]));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// PINK

bool see_pink(){ //is pink ball visible
  return bitRead(vision_received_data[20], 0);
}

int get_pink_x(){
  return conv_2_comp(word(vision_received_data[3], vision_received_data[2]));
}

int get_pink_y(){
  return conv_2_comp(word(vision_received_data[1], vision_received_data[0]));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int conv_2_comp(word w){
   int total = 0;
   
   for(int i = 0; i < 15; i++){
      if(bitRead(w, i)){
        total = total + pow(2,i);
      }
   }

   if(bitRead(w, 15)){
      total = total - pow(2, 15);
   }
   
   return total;
}


void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void serial2Flush(){
  while(Serial2.available() > 0) {
    char t = Serial2.read();
  }
}
  
