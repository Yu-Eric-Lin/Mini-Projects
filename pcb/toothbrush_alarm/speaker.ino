#include <WiFi.h>
#include <esp_now.h>

// Define the MAC address of the sender ESP32 (ESP32 A)
uint8_t senderMACAddress[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};

#define SPEAKER 17

// Structure to hold the received data
typedef struct {
  int data;
} MyData;

// Create a variable to store the received data
MyData receivedData;

void setup() {
  Serial.begin(115200);

  pinMode(SPEAKER, OUTPUT);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register the sender's MAC address
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, senderMACAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  // Register the receive callback function
  esp_now_register_recv_cb(receiveCallback);
}

void loop() {
  // Code in the loop will run after receiving each data packet
  if (data == 1)
    digitalWrite(SPEAKER, HIGH);
  else
    digitalWrite(SPEAKER, LOW);
}

// Callback function to handle received data
void receiveCallback(uint8_t *macAddr, uint8_t *data, int dataLen) {
  // Check if the received data is the expected size
  if (dataLen == sizeof(MyData)) {
    // Copy the received data into the appropriate structure
    memcpy(&receivedData, data, sizeof(MyData));
    
    // Print the received data
    Serial.print("Received data: ");
    Serial.println(receivedData.data);
  } else {
    Serial.println("Invalid data received");
  }
}
