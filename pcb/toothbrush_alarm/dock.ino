#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <ESP32Time.h>

//ESP32Time rtc;
ESP32Time rtc(3600);  // offset in seconds GMT+1

#define BUTTON_1 33
#define BUTTON_2 34
#define BUTTON_3 35
#define BUTTON_4 36
#define IR_SENSOR_1 21
#define IR_SENSOR_2 22
#define IR_SENSOR_3 23


// #define ACC_PIN 17

byte numbers[10][7] = {
  {0,0,0,0,0,0,1}, // 0
  {1,0,0,1,1,1,1}, // 1
  {0,0,1,0,0,1,0}, // 2
  {0,0,0,0,1,1,0}, // 3
  {1,0,0,1,1,0,0}, // 4
  {0,1,0,0,1,0,0}, // 5
  {0,1,0,0,0,0,0}, // 6
  {0,0,0,1,1,1,1}, // 7
  {0,0,0,0,0,0,0}, // 8
  {0,0,0,0,1,0,0}  // 9
};

// Define segment pins a-g and digit pins 1-4
const int segmentPins[7] = {17, 18, 16, 32, 27, 19, 5};  // GPIOs for segments a-g
const int digitPins[4] = {4, 15, 26, 14}; // GPIOs for digits 1-4
const int allPins[11] = {4, 15, 26, 14, 17, 18, 16, 32, 27, 19, 5};

uint8_t toothbrushAddress[] = {0xD4, 0x8A, 0xFC, 0xD0, 0x5D, 0x9C};
uint8_t alarmAddress[] = {0xD4, 0x8A, 0xFC, 0xCF, 0xFF, 0xA8};

// Define variables to store the current time
int reset_mode;
int currentHour;
int currentMinute;
int currentTime;

// Define variables to store the alarm time
int alarmHour;
int alarmMinute;
int alarmHour_temp;
int alarmMinute_temp;
int alarmRangeStart;
int alarmRangeEnd;

// Define variables to remove debounce button issue
bool button1state;
bool button2state;
bool button3state;
bool button4state;

bool button3state_2;
bool button2state_2;

// Define variables to store pressure sensor readings to be sent
int alarm_on;
long brushing_time;
long brush_start_time;
int brush_time_showing;

// Define variables to store incoming readings
int incomingBrush = 0;

// Variable to store if sending data was successful
String success;

// Define variables to check whether user is in the area or not 
bool user_presence;

//Structure example to send data. Must match the receiver structure
typedef struct struct_message {
    int brush;
    int alarm;
} struct_message;

// Create a struct_message called sending_message to hold sensor readings
struct_message sending_message;

// Create a struct_message to hold incoming sensor readings
struct_message receiving_message;

esp_now_peer_info_t peerInfo;   // for the toothbrush
esp_now_peer_info_t peerInfo_2; // for the alarm


// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}


// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&receiving_message, incomingData, sizeof(receiving_message));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingBrush = receiving_message.brush;
  Serial.println("User pick up brush? ");
  Serial.println(incomingBrush);
}


void setup() {
  pinMode(BUTTON_1, INPUT);   // for reset the current time
  pinMode(BUTTON_2, INPUT);   // for reset the hour
  pinMode(BUTTON_3, INPUT);   // for reset the minute
  pinMode(BUTTON_4, INPUT);   // for reset the minute

  pinMode(IR_SENSOR_1, INPUT);  // set each IR sensor at input so it can read the data
  pinMode(IR_SENSOR_2, INPUT);  
  pinMode(IR_SENSOR_3, INPUT);  

  // initialization for the current and alarm time
  currentHour = 0;
  currentMinute = 0;
  reset_mode = 0;
  currentTime = 0;

  // healthy wake-up time
  alarmHour = 0;
  alarmMinute = 1;
  alarmRangeStart = ((alarmHour+1) * 60) + alarmMinute;
  alarmRangeEnd = alarmRangeStart + 1;
  alarmHour_temp = alarmHour;
  alarmMinute_temp = alarmMinute;
  
  button1state = false;
  button2state = false;
  button3state = false;
  button4state = false;

  button2state_2 = false;
  button3state_2 = false;

  // initially set to false
  user_presence = false;

  // Initialize segment pins
  for (int i = 0; i < sizeof(allPins); i++) {
    pinMode(allPins[i], OUTPUT);
  }
  // Initialize digit pins
  for (int i = 0; i < 4; i++) {
    digitalWrite(digitPins[i], HIGH); // Off for common anode
  }
  // Set time
           // s   m   h   d   m  y
  rtc.setTime(59, 0, 0, 16, 4, 2024);


  // Init Serial Monitor
  Serial.begin(115200);

  // pinMode(ACC_PIN, INPUT);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // initialization for bruhsing time
  brush_start_time = 0;
  brushing_time = 0;
  brush_time_showing = 0;

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, toothbrushAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // Register peer 2
  memcpy(peerInfo_2.peer_addr, alarmAddress, 6);
  peerInfo_2.channel = 0;  
  peerInfo_2.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo_2) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}


bool sep_state = false;
bool brushing = false;

int countdown = 20;
int last_timestamp = rtc.getEpoch();
int a;
int resetCT, resetCHour, resetCMin;
int i, j, k;

long hold_start = -1;


void loop() {
  Serial.println("LOOP START");

  // resetCT = digitalRead(BUTTON_1);
  // Serial.println(resetCT);
  // if(resetCT == 1){
  //   reset_mode = (reset_mode + 1) %3;
  // }

  if (digitalRead(BUTTON_1) == 1 && button1state == false){
    reset_mode = (reset_mode + 1) %3;
    button1state = true;
  }
  else if(digitalRead(BUTTON_1) == 0 && button1state == true){
    button1state = false;
  }

  Serial.println("reset_mode:");
  Serial.println(reset_mode);
  // resetCHour = digitalRead(BUTTON_2);
  // Serial.println(resetCHour);
  // resetCMin = digitalRead(BUTTON_3);
  // Serial.println(resetCMin);
  // a = digitalRead(BUTTON_3);
  // Serial.println(a);

  // i = digitalRead(IR_SENSOR_1);
  // j = digitalRead(IR_SENSOR_2);
  k = digitalRead(IR_SENSOR_3);

  // Serial.println("i");
  // Serial.println(i);
  // Serial.println("j");
  // Serial.println(j);
  Serial.println("k");
  Serial.println(k);
  Serial.println();

  if (k)
    user_presence = true;
  else
    user_presence = false;

  // if (i || j || k) {
  //   user_presence = true;
  // } else {
  //   user_presence = false;
  // }

  // user_presence = true;

  currentTime = ((rtc.getHour(true) * 60) + rtc.getMinute()); 
  Serial.println("debug begin");
  Serial.println(currentTime);
  Serial.println(alarmRangeStart);
  Serial.println(alarmRangeEnd);
  // Serial.println("debug end");


  // Set values to send
  // if((currentTime >= alarmRangeStart) && (currentTime < alarmRangeEnd)) {
  if(currentTime >= alarmRangeStart) {
    // if (digitalRead(BUTTON_2) == 1 && button2state == false) {
    //   button2state = true;
    // }
    // else if(digitalRead(BUTTON_2) == 0 && button2state == true) {
    //   button2state = false;
    // }
    // if (digitalRead(BUTTON_3)== 1 && button3state == false) {
    //   button3state = true;
    // }
    // else if(digitalRead(BUTTON_3) == 0 && button3state == true) {
    //   button3state = false;
    // }

    if (digitalRead(BUTTON_2) == 1 && button2state_2 == false) {
      button2state_2 = true;
    }
    else if(digitalRead(BUTTON_2) == 0 && button2state_2 == true) {
      button2state_2 = false;
    }
    if (digitalRead(BUTTON_3)== 1 && button3state_2 == false) {
      button3state_2 = true;
    }
    else if(digitalRead(BUTTON_3) == 0 && button3state_2 == true) {
      button3state_2 = false;
    }
    
    if ((incomingBrush == 1) && (user_presence == true)) {     // if the user hold the toothbrush && user present
      brushing = true;
      if (brush_start_time == 0) {
        brush_start_time = rtc.getEpoch();
      }
      sending_message.brush = 0;
      alarm_on = 0;
      sending_message.alarm = alarm_on;
    }
    else {
      brushing = false;
      if (brush_start_time == 0) {
        brush_start_time = 0;
        brush_time_showing = 1;
      }
      else {
        brushing_time += (rtc.getEpoch() - brush_start_time);
      }
      brush_start_time = 0;

      if (button2state_2 == true && button3state_2 == true) {
        if (hold_start == -1)
          hold_start = rtc.getEpoch();
        if (rtc.getEpoch() - hold_start >= 2) {   // if hold down for 2 seconds
          countdown = 0;
        }
      }
      else 
        hold_start = -1;

      // if(brushing_time >= 20) {
      if(countdown <= 0) {
        
      // if(countdown <= 0 || (button2state == true && button3state == true)) {
        // if (button2state == true && button3state == true) {
        //   if (hold_start == -1)
        //     hold_start = rtc.getEpoch();
        //   if (rtc.getEpoch() - hold_start >= 5) {   // if hold down for 5 seconds
        //     countdown = 0;
        //     // alarm_on = 0;
        //   }
        // }
        // else 
        //   hold_start = -1;

        Serial.println("debug end");
        sending_message.brush = 0;
        alarm_on = 0;
        sending_message.alarm = alarm_on;
        brush_time_showing = 0;
      }
      else {
        sending_message.brush = 0;
        alarm_on = 1;
        sending_message.alarm = alarm_on;
      }
    }
  }

  if (currentTime >= alarmRangeEnd) {
    sending_message.brush = 0;
    alarm_on = 0;
    sending_message.alarm = alarm_on;
  }


  // for (int number = 0; number < 10; number++) { // Cycle through numbers 0-9
  //    for (int digit = 0; digit < 4; digit++) { // Display on each digit
  //      setNumber(digit, number); // Display the current number
  //      delay(3000); // Adjust for visibility, too fast and all digits will seem lit
  //    }
  // }


  // display time on the 7segment display
  // ****************************************************************************************
  // if (brushing == false || brush_time_showing == 0) {  // for display normal time
  //   if(reset_mode == 1){   // if needs for reset time
  //     if (digitalRead(BUTTON_2) == 1 && button2state == false){
  //       currentHour = (currentHour + 1) % 24;
  //       button2state = true;
  //     }
  //     else if(digitalRead(BUTTON_2) == 0 && button2state == true){
  //       button2state = false;
  //     }

  //     // if (digitalRead(BUTTON_2) == HIGH) {
  //     //   currentHour = (currentHour + 1) % 24;
  //     // }
  //     // if (digitalRead(BUTTON_3) == HIGH) {
  //     //   currentMinute = (currentMinute + 1) % 60;
  //     // }

  //     if (digitalRead(BUTTON_3) == 1 && button3state == false){
  //       currentMinute = (currentMinute + 1) % 60;
  //       button3state = true;
  //     }
  //     else if(digitalRead(BUTTON_3) == 0 && button3state == true){
  //       button3state = false;
  //     }
      
  //     for (int digit = 0; digit < 4; digit++) { // Display on each digit
  //       ResetDigitTime(digit); // Display the current number
  //       delay(5); // Adjust for visibility, too fast and all digits will seem lit
  //     }

  //     // if (digitalRead(BUTTON_4) == HIGH){
  //     //     // Set time
  //     //     // s   m   h   d   m  y
  //     //   rtc.setTime(59, currentMinute, currentHour, 16, 4, 2024);
  //     //   reset_mode = 0;
  //     // }

  //     if (digitalRead(BUTTON_4) == 1 && button4state == false){
  //       rtc.setTime(40, currentMinute, currentHour, 16, 4, 2024);
  //       reset_mode = 0;
  //       button4state = true;
  //     }
  //     else if(digitalRead(BUTTON_4) == 0 && button4state == true){
  //       button4state = false;
  //     }
  //   }

  //   else if (reset_mode == 2){
  //     if (digitalRead(BUTTON_2) == 1 && button2state == false){
  //       alarmHour_temp = (alarmHour_temp + 1) % 24;
  //       button2state = true;
  //     }
  //     else if(digitalRead(BUTTON_2) == 0 && button2state == true){
  //       button2state = false;
  //     }

  //     // if (digitalRead(BUTTON_2) == HIGH) {
  //     //   alarmHour_temp = (alarmHour_temp + 1) % 24;
  //     // }
  //     // if (digitalRead(BUTTON_3) == HIGH) {
  //     //   alarmMinute_temp = (alarmMinute_temp + 1) % 60;
  //     // }

  //     if (digitalRead(BUTTON_3) == 1 && button3state == false){
  //       alarmMinute_temp = (alarmMinute_temp + 1) % 60;
  //       button3state = true;
  //     }
  //     else if(digitalRead(BUTTON_3) == 0 && button3state == true){
  //       button3state = false;
  //     }

  //     for (int digit = 0; digit < 4; digit++) { // Display on each digit
  //       ResetAlarmTime(digit); // Display the current number
  //       delay(5); // Adjust for visibility, too fast and all digits will seem lit
  //     }

  //     // if (digitalRead(BUTTON_4) == HIGH){
  //     //     alarmHour = alarmHour_temp;
  //     //     alarmMinute = alarmMinute_temp;
  //     //     alarmRangeStart = ((alarmHour+1) * 60) + alarmMinute;
  //     //     alarmRangeEnd = alarmRangeStart + 30;
  //     //     reset_mode = 0;
  //     // }

  //     if (digitalRead(BUTTON_4) == 1 && button4state == false){
  //        alarmHour = alarmHour_temp;
  //       alarmMinute = alarmMinute_temp;
  //       alarmRangeStart = ((alarmHour+1) * 60) + alarmMinute;
  //       alarmRangeEnd = alarmRangeStart + 30;
  //       reset_mode = 0;
  //       button4state = true;
  //       countdown = 20;
  //     }
  //     else if(digitalRead(BUTTON_4) == 0 && button4state == true){
  //       button4state = false;
  //     }
  //   }

  //   else {    // display normal time
  //     for (int digit = 0; digit < 4; digit++) { // Display on each digit
  //       setDigitTime(digit); // Display the current number
  //       delay(5); // Adjust for visibility, too fast and all digits will seem lit
  //     }
  //   }

  // }
  // else {  // for display countdown
  //   // if (brush_time_showing == 1){
  //     if (last_timestamp != rtc.getEpoch()) {
  //       last_timestamp = rtc.getEpoch();
  //       countdown--;
  //     }
  
  //     for (int digit = 0; digit < 4; digit++) {   // Display on each digit
  //       setCountdown(digit);
  //       delay(5);
  //     }
  //   // }
  // }
  // ****************************************************************************************

  if (brushing == true && brush_time_showing == 1) {  // for display normal time

    // if (brush_time_showing == 1){
      if (last_timestamp != rtc.getEpoch()) {
        last_timestamp = rtc.getEpoch();
        if (countdown > 0)
          countdown--;
      }
  
      for (int digit = 0; digit < 4; digit++) {   // Display on each digit
        setCountdown(digit);
        delay(5);
      }
    // }

  }
  else {  // for display countdown

    if(reset_mode == 1){   // if needs for reset time
      if (digitalRead(BUTTON_2) == 1 && button2state == false){
        currentHour = (currentHour + 1) % 24;
        button2state = true;
      }
      else if(digitalRead(BUTTON_2) == 0 && button2state == true){
        button2state = false;
      }

      // if (digitalRead(BUTTON_2) == HIGH) {
      //   currentHour = (currentHour + 1) % 24;
      // }
      // if (digitalRead(BUTTON_3) == HIGH) {
      //   currentMinute = (currentMinute + 1) % 60;
      // }

      if (digitalRead(BUTTON_3) == 1 && button3state == false){
        currentMinute = (currentMinute + 1) % 60;
        button3state = true;
      }
      else if(digitalRead(BUTTON_3) == 0 && button3state == true){
        button3state = false;
      }
      
      for (int digit = 0; digit < 4; digit++) { // Display on each digit
        ResetDigitTime(digit); // Display the current number
        delay(5); // Adjust for visibility, too fast and all digits will seem lit
      }

      // if (digitalRead(BUTTON_4) == HIGH){
      //     // Set time
      //     // s   m   h   d   m  y
      //   rtc.setTime(59, currentMinute, currentHour, 16, 4, 2024);
      //   reset_mode = 0;
      // }

      if (digitalRead(BUTTON_4) == 1 && button4state == false){
        rtc.setTime(40, currentMinute, currentHour, 16, 4, 2024);
        reset_mode = 0;
        button4state = true;
      }
      else if(digitalRead(BUTTON_4) == 0 && button4state == true){
        button4state = false;
      }
    }

    else if (reset_mode == 2){
      if (digitalRead(BUTTON_2) == 1 && button2state == false){
        alarmHour_temp = (alarmHour_temp + 1) % 24;
        button2state = true;
      }
      else if(digitalRead(BUTTON_2) == 0 && button2state == true){
        button2state = false;
      }

      // if (digitalRead(BUTTON_2) == HIGH) {
      //   alarmHour_temp = (alarmHour_temp + 1) % 24;
      // }
      // if (digitalRead(BUTTON_3) == HIGH) {
      //   alarmMinute_temp = (alarmMinute_temp + 1) % 60;
      // }

      if (digitalRead(BUTTON_3) == 1 && button3state == false){
        alarmMinute_temp = (alarmMinute_temp + 1) % 60;
        button3state = true;
      }
      else if(digitalRead(BUTTON_3) == 0 && button3state == true){
        button3state = false;
      }

      for (int digit = 0; digit < 4; digit++) { // Display on each digit
        ResetAlarmTime(digit); // Display the current number
        delay(5); // Adjust for visibility, too fast and all digits will seem lit
      }

      // if (digitalRead(BUTTON_4) == HIGH){
      //     alarmHour = alarmHour_temp;
      //     alarmMinute = alarmMinute_temp;
      //     alarmRangeStart = ((alarmHour+1) * 60) + alarmMinute;
      //     alarmRangeEnd = alarmRangeStart + 30;
      //     reset_mode = 0;
      // }

      if (digitalRead(BUTTON_4) == 1 && button4state == false){
         alarmHour = alarmHour_temp;
        alarmMinute = alarmMinute_temp;
        alarmRangeStart = ((alarmHour+1) * 60) + alarmMinute;
        alarmRangeEnd = alarmRangeStart + 1;
        reset_mode = 0;
        button4state = true;
        countdown = 20;
      }
      else if(digitalRead(BUTTON_4) == 0 && button4state == true){
        button4state = false;
      }
    }

    else {    // display normal time
      for (int digit = 0; digit < 4; digit++) { // Display on each digit
        setDigitTime(digit); // Display the current number
        delay(5); // Adjust for visibility, too fast and all digits will seem lit
      }
    }
  }


  Serial.println("alarm_on");
  Serial.println(alarm_on);

  Serial.println("brushing_time");
  Serial.println(brushing_time);
  

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(alarmAddress, (uint8_t *) &sending_message, sizeof(sending_message));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

  Serial.println("LOOP END");
  Serial.println();
  Serial.println();
  Serial.println();

  delay(5);
}


void setCountdown(int digit) {
  int number = countdown;

  if (digit == 0){
    number = 0;
  } else if (digit == 1) {
    number = (number % 1000) / 100;
  } else if (digit == 2) {
    number = (number % 100) / 10;
  } else if (digit == 3) {
    number = number % 10;
  }

  setNumber_2(digit, number);
}

int number_1;

void setDigitTime(int digit) {
  // Serial.println(rtc.getSecond());        //  (int)   (0-59)
  // Serial.println(rtc.getMinute());        //  (int)   (0-59)

  int number = 0;

  if (digit == 0){
    number = rtc.getMinute();
    number = number / 10;
  } else if (digit == 1) {
    number = rtc.getMinute();
    number = number % 10;
  } else if (digit == 2) {
    number = rtc.getSecond();
    number = number / 10;
  } else if (digit == 3) {
    number = rtc.getSecond();
    number = number % 10;
  }

  // if (digit == 0){
  //   number = rtc.getHour(true);
  //   number_1 = number - 1;
  //   number = number_1 / 10;
  // } else if (digit == 1) {
  //   number = rtc.getHour(true);
  //   number_1 = number - 1;
  //   number = number_1 % 10;
  // } else if (digit == 2) {
  //   number = rtc.getMinute();
  //   number = number / 10;
  // } else if (digit == 3) {
  //   number = rtc.getMinute();
  //   number = number % 10;
  // }

  setNumber_2(digit, number);
}


void ResetDigitTime(int digit){
  int number = 0;
  if (digit == 0){
    number = currentHour;
    number = number / 10;
  } else if (digit == 1) {
    number = currentHour;
    number = number % 10;
  } else if (digit == 2) {
    number = currentMinute;
    number = number / 10;
  } else if (digit == 3) {
    number = currentMinute;
    number = number % 10;
  }

  setNumber_2(digit, number);
}


void ResetAlarmTime(int digit) {
  int number = 0;
  if (digit == 0){
    number = alarmHour_temp;
    number = number / 10;
  } else if (digit == 1) {
    number = alarmHour_temp;
    number = number % 10;
  } else if (digit == 2) {
    number = alarmMinute_temp;
    number = number / 10;
  } else if (digit == 3) {
    number = alarmMinute_temp;
    number = number % 10;
  }

  setNumber_2(digit, number);
}


void setNumber(int digit, int number) {
  Serial.println(String(digit) + " " + String(number));
  Serial.println();

  // Turn off all digits
  for (int i = 0; i < 4; i++) {
    // if (i == digit) {
    //   digitalWrite(digitPins[i], HIGH);
    //   continue;
    // }
    digitalWrite(digitPins[i], LOW); // Off for common anode
  }

  // Set segments for this number
  for (int segment = 0; segment < 7; segment++) {
    digitalWrite(segmentPins[segment], numbers[number][segment]);
  }

  // Turn on the current digit
  digitalWrite(digitPins[digit], HIGH); // On for common anode
}


void setNumber_2(int digit, int number) {
  Serial.println(String(digit) + " " + String(number));
  Serial.println();

  // Turn off all digits
  for (int i = 0; i < 4; i++) {
    // if (i == digit) {
    //   digitalWrite(digitPins[i], HIGH);
    //   continue;
    // }
    digitalWrite(digitPins[i], LOW); // Off for common anode
  }

  // Set segments for this number
  for (int segment = 0; segment < 7; segment++) {
    digitalWrite(segmentPins[segment], numbers[number][segment]);
  }

  // Turn on the current digit
  digitalWrite(digitPins[digit], HIGH); // On for common anode
}


void setNumber_3(int digit, int number) {
  Serial.println(String(digit) + " " + String(number));
  Serial.println();

  // Turn off all digits
  for (int i = 0; i < 4; i++) {
    // if (i == digit) {
    //   digitalWrite(digitPins[i], HIGH);
    //   continue;
    // }
    digitalWrite(digitPins[i], LOW); // Off for common anode
  }

  // Set segments for this number
  for (int segment = 0; segment < 7; segment++) {
    digitalWrite(segmentPins[segment], numbers[number][segment]);
  }

  // Turn on the current digit
  digitalWrite(digitPins[digit], HIGH); // On for common anode
}
