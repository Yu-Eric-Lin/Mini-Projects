#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

#define LED 2
#define ACC_PIN 17
#define SPEAKER 23

// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = {0xD4, 0x8A, 0xFC, 0xC6, 0xc4, 0x90};

// Define variables to store incoming readings
int brush_in;
int alarm_in;

int in_alarm_on;
int in_data2;
int in_data3;

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    int brush;
    int alarm;
} struct_message;

// Create a struct_message called send_data to hold sensor readings
struct_message send_data;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;

esp_now_peer_info_t peerInfo;

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
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  brush_in = incomingReadings.brush;
  alarm_in = incomingReadings.alarm;

  in_alarm_on = alarm_in; // test

  Serial.println("Data Received:");
  Serial.println(brush_in);
  Serial.println(alarm_in);
}


void setup() {
  // ACC set up
  // Init Serial Monitor
  Serial.begin(115200);

  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");
  pinMode(ACC_PIN, OUTPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // Try to initialize
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);


  pinMode(SPEAKER, OUTPUT);
  // pinMode(LED, OUTPUT);
 
  // Communication Setup
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}


bool speakerState = false;
bool ledState = false;
int brush_out = 0;
int counter = 0;

float b_thresh = 0.5;


void loop() {
  counter++;
  Serial.println("LOOP START");
  // Serial.println(counter);

  // ACC Reading Loop
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  /* Print out the values */
  // Serial.print("Acceleration X: ");
  // Serial.print(a.acceleration.x);
  // Serial.print(", Y: ");
  // Serial.print(a.acceleration.y);
  // Serial.print(", Z: ");
  // Serial.print(a.acceleration.z);
  // Serial.println(" m/s^2");

  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

  // Serial.print("Temperature: ");
  // Serial.print(temp.temperature);
  // Serial.println(" degC");


  // if (abs(a.acceleration.x + 0.77) >= a_thresh or abs(a.acceleration.y + 0.20) >= a_thresh or abs(a.acceleration.z - 10.44) >= a_thresh) {
  if (abs(g.gyro.x + 0.05) >= b_thresh or abs(g.gyro.y - 0.05) >= b_thresh or abs(g.gyro.z - 0.01) >= b_thresh) {
    digitalWrite(ACC_PIN, HIGH);
    digitalWrite(LED, HIGH);
    Serial.println("MOVED");
    brush_out = 1;
  } else {
    digitalWrite(ACC_PIN, LOW);
    digitalWrite(LED, LOW);
    brush_out = 0;
  }

  Serial.println("");
  // delay(500);


  // Commuincation Loop
  // if (in_alarm_on == 1) {
  //   speakerState = true;
  //   ledState = true;
  // } else if (in_alarm_on == 0) {
  //   speakerState = false;
  //   ledState = false;
  // }

  // speakerState = !speakerState;   // Toggle the state
  // analogWrite(SPEAKER, speakerState ? 150 : LOW);
  // digitalWrite(LED, ledState ? HIGH : LOW);

  // Set values to send
  send_data.brush = brush_out;
  send_data.alarm = 200;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &send_data, sizeof(send_data));
  
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

  Serial.println("LOOP END");

  delay(1000);
}
