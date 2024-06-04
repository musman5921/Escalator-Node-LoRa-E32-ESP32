/*
 Project Name ------  FRS
 Task --------------  Escalator Node Firmware with Esp32
 Engineer ----------- Muhammad Usman
 File --------------- Main File (Code Exceution Start in this file)
 Company -----------  Machadev Pvt Limited

Mesh Network Development:

- Developed a mesh network using Ebyte LoRa E32 with ESP32.
- The same logic applies to each node.

Flow:

- The mesh network program is identical for all nodes in the network:
  - Each node listens to other nodes in a loop.
  - Each node updates the activity status of other nodes every 10 seconds.
  - Each node broadcasts a message every 30 seconds.
  - Each node prints network stats every 60 seconds.

Tests:

- The network consists of 4 nodes:
  - Each node knows how many nodes are available, active, and dead in its network.
  - Each node successfully updates its node info container every 60 seconds.
  - Note: There is no acknowledgment message for broadcasted messages.

PlatformIO Configuration:

[env:esp32dev]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
lib_deps = mikem/RadioHead@^1.120

Escalator Node:

- The AUX pin of the LoRa module is NOT connected to the ESP32.
- The relay is active low, with an LED connected in series with the relay.
- ssid: FRS-Escalator
- password: FRS12345678
- This node is activated by FyreBox node (it should send its state back, weather active or not)

FyreBox Node:

- The AUX pin of the LoRa module is NOT connected to the ESP32 S3 Mini.
- Serial0: Used for program uploading and debugging.
- Serial1: DWIN LCD is connected at IO15 (TX of LCD) and IO16 (RX of LCD).
- LoRa Module: Connected at IO35 (RX) and IO36 (TX) using software serial.
- SD Card Connection: The SIG pin must be HIGH (IO5) for the SD card to connect with the DWIN LCD.

TODO:
  - Integrate RTOS
  - Integrate web server

*/

// Import Libraries
#include "functions.h"
#include "constants.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Replace with your network credentials
// const char* ssid = "Machadev";
// const char* password = "13060064";
// const char* ssid = "FRS";
// const char* password = "frspassword";
const char* ssid = "Machadev";
const char* password = "Machadev321";

// Create a web server object that listens for HTTP requests on port 80
WebServer server(80);

void setup() {
  Serial.begin(Baud_RATE_SERIAL);
  Serial.println("Serial is ready.");

  Serial2.begin(Baud_RATE_LORA);
  Serial.println("Serial2 is ready.");

  Serial.println("Initializing mesh...");
  while(! initializeMESH()){  // stays in a loop until LoRa found 
    Serial.println("Mesh initialization failed");
    Serial.println("Retyring...");
    delay(3000);
   }
  Serial.println("Mesh initialized successfully.");

  xTaskCreatePinnedToCore(LoRatask, "LoRatask", 4096, NULL, 1, &xHandleLoRa, 1);

  // xTaskCreate(dateTimeTask, "DateTimeTask", 2048, NULL, 4, &xHandledatetime);
  // vTaskSuspend(xHandledatetime);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Set the ESP32 as a Wi-Fi Station and Access Point
  WiFi.mode(WIFI_AP_STA);
  
  // Connect to Wi-Fi network with SSID and password
  WiFi.begin(ssid, password);

  // Start the Access Point
  WiFi.softAP("FRS-Escalator", "FRS12345678");

  // Print ESP32 Local IP Address
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Define routes
  server.on("/", HTTP_GET, handleRoot);
//  server.on("/connect", HTTP_GET, handleConnectGet);

  server.on("/settings_option", HTTP_POST, handleSettingsOptionPost);
  
  server.on("/connect", HTTP_POST, handleConnectPost);

  server.on("/client_login", HTTP_POST, handleClientLoginPost);
  server.on("/admin_login", HTTP_POST, handleAdminLoginPost);

  server.on("/connect_wifi", HTTP_POST, handleWifiConnectPost);

  server.on("/connect_with_unique_key", HTTP_POST, handleUniqueKeyPost);

  server.on("/fill_data_auto_or_not", HTTP_POST, handleFillDataAutoOrNotPost);

  server.on("/send_company_details", HTTP_POST, handleGetCompanyDetailsPost);

  server.on("/send_unit_details", HTTP_POST, handleGetUnitDetailsPost);

  server.on("/send_manufacturer_details", HTTP_POST, handleGetManufacturerDetailsPost);

  server.on("/wifi_access_point", HTTP_POST, handleGetWifiAccessPointPost);

  // Serve all files from SPIFFS
  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();

}

void loop() {

    // Handle client requests
    server.handleClient();

    static unsigned long lastBroadcastTime = 0;
    static unsigned long lastCheckTime = 0;
    static unsigned long lastStatusPrintTime = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - lastBroadcastTime > 30000) {  // Every 30 seconds
        broadcastPresence();
        lastBroadcastTime = currentMillis;
    }

    listenForNodes();

    if (currentMillis - lastCheckTime > 10000) {  // Every 10 seconds
        checkNodeActivity();
        lastCheckTime = currentMillis;
    }

    if (currentMillis - lastStatusPrintTime > 60000) {  // Every 60 seconds
        // printNodeStatuses();  // Print the statuses of all nodes
        printNetworkStats();
        lastStatusPrintTime = currentMillis;
    }
}
