/*
 Project Name ------  FRS
 Task --------------  Escalator Node Firmware with Esp32
 Engineer ----------- Muhamamd Usman
 File --------------- Functions Source File
 Company -----------  Machadev Pvt Limited
 */

// Import Libraries
#include "functions.h"
#include "constants.h"
#include <EEPROM.h>

RH_E32 driver(&Serial2, MOPIN, M1PIN, AUXPIN); //M0, M1, AUX
RHMesh mesh(driver, NODEID); // Node ID 2 for this example

std::vector<NodeStatus> nodeStatuses;

// Create a web server object that listens for HTTP requests on port 80
extern WebServer server;

// Converts a status code to a human-readable error string.
const __FlashStringHelper* getErrorString(uint8_t status) {
  switch (status) {
    case 1: return F("invalid length");
    case 2: return F("no route");
    case 3: return F("timeout");
    case 4: return F("no reply");
    case 5: return F("unable to deliver");
    default: return F("unknown");
  }
}

// For LoRa Mesh
void LoRatask(void* parameter){
    Serial.println("LoRatask Started");
    for (;;) {
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
    Serial.println("LoRatask completed");
    Serial.println("LoRatask Suspended");
    vTaskSuspend(xHandleLoRa); // Suspend the task
}

// Initializes the MESH network.
bool initializeMESH() {
if (!mesh.init()) {
        // Serial.println("Mesh initialization failed");
        return false;
    }
    return true;
}

// Broadcasts the presence of the current node to other nodes in the network.
void broadcastPresence() {
  const char* presenceMsg = "Node Present";
  uint8_t status = mesh.sendtoWait((uint8_t*)presenceMsg, strlen(presenceMsg) + 1, RH_BROADCAST_ADDRESS);
  if (status == RH_ROUTER_ERROR_NONE) {
      Serial.println("Presence message sent successfully");
  } else {
      Serial.print("Failed to send presence message, error: ");
      Serial.println(status);
      Serial.println((const __FlashStringHelper*)getErrorString(status));
  }
}

// Broadcasts active message 
void activeState() {
  if(activateRelayonce) {
      activateRelayonce = false;
      digitalWrite(RLYPIN, HIGH); // turn on the relay
    }

  const char* activeMsg = "Actived";
  uint8_t status = mesh.sendtoWait((uint8_t*)activeMsg, strlen(activeMsg) + 1, RH_BROADCAST_ADDRESS);
  if (status == RH_ROUTER_ERROR_NONE) {
      Serial.println("Active message sent successfully");
  } else {
      Serial.print("Failed to send active message, error: ");
      Serial.println(status);
      Serial.println((const __FlashStringHelper*)getErrorString(status));
  }
}

// Broadcasts inactive message 
void inactiveState() {
  digitalWrite(RLYPIN, LOW); // turn off the relay

  const char* inactiveMsg = "Deactived";
  uint8_t status = mesh.sendtoWait((uint8_t*)inactiveMsg, strlen(inactiveMsg) + 1, RH_BROADCAST_ADDRESS);
  if (status == RH_ROUTER_ERROR_NONE) {
      Serial.println("Inactive message sent successfully");
  } else {
      Serial.print("Failed to send inactive message, error: ");
      Serial.println(status);
      Serial.println((const __FlashStringHelper*)getErrorString(status));
  }
}

// Listens for incoming messages from other nodes.
void listenForNodes() {
    uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from;

    if (mesh.recvfromAckTimeout(buf, &len, 2000, &from)) {
        Serial.print("Received message from node ");
        Serial.print(from);
        Serial.print(": ");
        Serial.println((char*)buf);

        if((char*)buf == "Active") {
          activeState(); // Broadcast active message
          activateRelayonce = true;
        }
        else if((char*)buf == "Inactive") {
          inactiveState(); // Broadcast inactive message
        }
        else if((char*)buf == "Node Present") {
          updateNodeStatus(from); // Update node information
        }
    }
}

// Updates the status of a node given its ID.
void updateNodeStatus(uint8_t nodeId) {
    bool nodeFound = false;
    unsigned long currentTime = millis();
    for (auto& status : nodeStatuses) {
        if (status.nodeId == nodeId) {
            status.lastSeen = currentTime;
            status.isActive = true;
            nodeFound = true;
            break;
        }
    }
    if (!nodeFound) {
        nodeStatuses.push_back({nodeId, currentTime, true});
    }
}

// Checks the activity of all nodes in the network.
void checkNodeActivity() {
    unsigned long currentTime = millis();
    const unsigned long timeout = 60000; // 1 minute timeout to consider a node as dead
    for (auto& status : nodeStatuses) {
        if (status.isActive && (currentTime - status.lastSeen > timeout)) {
            status.isActive = false;
            Serial.print("Node ");
            Serial.print(status.nodeId);
            Serial.println(" is now considered dead.");
        }
    }
}

// Gets the total number of nodes currently in the network.
size_t getTotalNodes() {
    return nodeStatuses.size();
}

// Prints the statuses of all nodes in the network.
void printNodeStatuses() {
    size_t totalNodes = getTotalNodes();  // Get the total number of nodes
    Serial.print("Total Nodes: ");
    Serial.println(totalNodes);

    for (const auto& status : nodeStatuses) {
        Serial.print("Node ");
        Serial.print(status.nodeId);
        Serial.print(": ");
        Serial.println(status.isActive ? "Active" : "Dead");
    }
}

// Prints statistical information about the network.
void printNetworkStats() {
    int totalNodes = nodeStatuses.size();  // Total number of nodes is the size of the vector
    int activeNodes = 0;
    int deadNodes = 0;

    // Count active and dead nodes
    for (const auto& status : nodeStatuses) {
        if (status.isActive) {
            activeNodes++;  // Increment active node count
        } else {
            deadNodes++;    // Increment dead node count
        }
    }

    // Print the results
    Serial.print("Total Nodes: ");
    Serial.println(totalNodes);
    Serial.print("Active Nodes: ");
    Serial.println(activeNodes);
    Serial.print("Dead Nodes: ");
    Serial.println(deadNodes);
}


void handleFileRead(String path) {
  String contentType = "text/html";
  if (path.endsWith(".html")) contentType = "text/html";
  else if (path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js")) contentType = "application/javascript";
  else if (path.endsWith(".png")) contentType = "image/png";
  else if (path.endsWith(".jpg")) contentType = "image/jpeg";
  else if (path.endsWith(".gif")) contentType = "image/gif";
  else if (path.endsWith(".ico")) contentType = "image/x-icon";
  else if (path.endsWith(".xml")) contentType = "text/xml";
  else if (path.endsWith(".pdf")) contentType = "application/pdf";
  else if (path.endsWith(".zip")) contentType = "application/zip";

  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return;
  }
  server.send(404, "text/plain", "File Not Found");
}

void handleRoot() {
  handleFileRead("/P0.html");
}

void handleSettingsOptionPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    
    const char* selectedOption = doc["selectedOption"];
    
    Serial.print("selectedOption: ");
    Serial.println(selectedOption);

    if (strcmp(selectedOption, "DeviceConfiguartion") == 0) {
      Serial.println("Redirecting to /P1.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P1.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }

    else if (strcmp(selectedOption, "ChangeDeviceLocalWiFiCredentials") == 0) {
      Serial.println("Redirecting to /P11.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P11.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
    
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleConnectPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    
    const char* get_data = doc["get_data"];
    
    Serial.print("get_data: ");
    Serial.println(get_data);

    if (strcmp(get_data, "terms accepted") == 0) {
      Serial.println("Redirecting to /P2.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P2.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }

    else if (strcmp(get_data, "go to admin login") == 0) {
      Serial.println("Redirecting to /P3.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P3.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }

    else if (strcmp(get_data, "go to client login") == 0) {
      Serial.println("Redirecting to /P2.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P2.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleClientLoginPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    
    const char* client_username = doc["client_username"];
    const char* client_password = doc["client_password"];
    const char* client_rememberMe = doc["client_rememberMe"];
    
    Serial.print("client_username: ");
    Serial.println(client_username);
    Serial.print("client_password: ");
    Serial.println(client_password);
    Serial.print("client_rememberMe: ");
    Serial.println(client_rememberMe);

    if(false)  // if login failed
    {
      Serial.println("Client Login Failed!!!");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "Login failed!!!";
      responseDoc["reason"] = "username or password is incorrect";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
    else {
      Serial.println("Redirecting to /P4.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P4.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleAdminLoginPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    
    const char* admin_username = doc["admin_username"];
    const char* admin_password = doc["admin_password"];
    const char* admin_rememberMe = doc["admin_rememberMe"];
    
    Serial.print("admin_username: ");
    Serial.println(admin_username);
    Serial.print("admin_password: ");
    Serial.println(admin_password);
    Serial.print("admin_rememberMe: ");
    Serial.println(admin_rememberMe);

    if(false)  // if login failed
    {
      Serial.println("Admin Login Failed!!!");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "Login failed!!!";
      responseDoc["reason"] = "username or password is incorrect";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
    else {
      Serial.println("Redirecting to /P4.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P4.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleWifiConnectPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    
    const char* wifi_ssid = doc["ssid"];
    const char* wifi_password = doc["password"];
    
    Serial.print("wifi_ssid: ");
    Serial.println(wifi_ssid);
    Serial.print("wifi_password: ");
    Serial.println(wifi_password);

    if(false)  // if Connection failed
    {
      Serial.println("WiFi Connection failed!!!");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "WiFi Connection failed!!!";
      responseDoc["reason"] = "ssid or password is incorrect";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
    else {
      Serial.println("Redirecting to /P5.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P5.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleUniqueKeyPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    
    const char* uniqueKey = doc["uniqueKey"];
    
    Serial.print("uniqueKey: ");
    Serial.println(uniqueKey);

    if(false)  // if Unique Key is not valid
    {
      Serial.println("Invalid Unique Key!!!");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "Invalid Unique Key!!!";
      responseDoc["reason"] = "";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
    else if(true) // if company details are already present in the web cloud
    {
      Serial.println("Redirecting to /P6.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P6.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
    else if(false) // if company details are not present in the web cloud
    {
      Serial.println("Redirecting to /P7.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P7.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleFillDataAutoOrNotPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    
    const char* fillDataOrNot = doc["selectedOption"];
    
    Serial.print("fillDataOrNot: ");
    Serial.println(fillDataOrNot);

    if (strcmp(fillDataOrNot, "yes") == 0)  // if User selected Yes
    {
      Serial.println("Redirecting to /P8.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P8.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
    if (strcmp(fillDataOrNot, "no") == 0) // if User selected No
    {
      Serial.println("Redirecting to /P7.html");
      DynamicJsonDocument responseDoc(1024);
      responseDoc["status"] = "redirect";
      responseDoc["url"] = "/P7.html";
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleGetCompanyDetailsPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, postBody);

    const char* companyName = doc["company-name"];
    const char* companyAddress = doc["company-address"];
    const char* keyPerson = doc["key-person"];
    const char* contactDetails = doc["contact-details"];
    const char* altPerson1 = doc["alt-person-1"];
    const char* altContactDetails1 = doc["alt-contact-details-1"];
    const char* altPerson2 = doc["alt-person-2"];
    const char* altContactDetails2 = doc["alt-contact-details-2"];
    const char* altPerson3 = doc["alt-person-3"];
    const char* altContactDetails3 = doc["alt-contact-details-3"];
    const char* altPerson4 = doc["alt-person-4"];
    const char* altContactDetails4 = doc["alt-contact-details-4"];
    const char* localFireDepartment = doc["local-fire-department"];
    const char* localFireDepartmentContactDetails = doc["local-fire-department-contact-details"];

    // Print data to Serial Monitor for debugging
    Serial.print("Company Name: ");
    Serial.println(companyName);
    Serial.print("Company Address: ");
    Serial.println(companyAddress);
    Serial.print("Key Responsible Person: ");
    Serial.println(keyPerson);
    Serial.print("Contact Details: ");
    Serial.println(contactDetails);
    Serial.print("Alt Responsible Person (1): ");
    Serial.println(altPerson1);
    Serial.print("Contact Details (1): ");
    Serial.println(altContactDetails1);
    Serial.print("Alt Responsible Person (2): ");
    Serial.println(altPerson2);
    Serial.print("Contact Details (2): ");
    Serial.println(altContactDetails2);
    Serial.print("Alt Responsible Person (3): ");
    Serial.println(altPerson3);
    Serial.print("Contact Details (3): ");
    Serial.println(altContactDetails3);
    Serial.print("Alt Responsible Person (4): ");
    Serial.println(altPerson4);
    Serial.print("Contact Details (4): ");
    Serial.println(altContactDetails4);
    Serial.print("Local Fire Department: ");
    Serial.println(localFireDepartment);
    Serial.print("Local Fire Department Contact Details: ");
    Serial.println(localFireDepartmentContactDetails);

    Serial.println("Redirecting to /P8.html");
    DynamicJsonDocument responseDoc(1024);
    responseDoc["status"] = "redirect";
    responseDoc["url"] = "/P8.html";
    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);
    return;
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleGetUnitDetailsPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, postBody);

    // Extract data from the JSON document
    const char* location = doc["location"];
    const char* unitNumber = doc["unitNumber"];
    const char* date = doc["date"];
    const char* installer = doc["installer"];
    const char* contact = doc["contact"];
    const char* ipAddress = doc["ipAddress"];

    // Print the extracted data to the Serial for debugging
    Serial.print("Location: ");
    Serial.println(location);
    Serial.print("Unit Number: ");
    Serial.println(unitNumber);
    Serial.print("Date: ");
    Serial.println(date);
    Serial.print("Installer: ");
    Serial.println(installer);
    Serial.print("Contact: ");
    Serial.println(contact);
    Serial.print("IP Address: ");
    Serial.println(ipAddress);

    Serial.println("Redirecting to /P9.html");
    DynamicJsonDocument responseDoc(1024);
    responseDoc["status"] = "redirect";
    responseDoc["url"] = "/P9.html";
    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);
    return;
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleGetManufacturerDetailsPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, postBody);

    // Extract data from the JSON document
    const char* manufacturer_name = doc["name"];
    const char* contact = doc["contact"];
    const char* email = doc["email"];
    const char* date_of_manufacture = doc["date"];
    const char* serial = doc["serial"];

    Serial.print("Manufacturer Name: ");
    Serial.println(manufacturer_name);
    Serial.print("Contact: ");
    Serial.println(contact);
    Serial.print("Email: ");
    Serial.println(email);
    Serial.print("Date of Manufacture: ");
    Serial.println(date_of_manufacture);
    Serial.print("Serial: ");
    Serial.println(serial);

    Serial.println("Redirecting to /P10.html");
    DynamicJsonDocument responseDoc(1024);
    responseDoc["status"] = "redirect";
    responseDoc["url"] = "/P10.html";
    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);
    return;
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleGetWifiAccessPointPost() {
  if (server.method() == HTTP_POST) {
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    
    const char* wifi_ssid_access_point = doc["ssid"];
    const char* wifi_password_access_point = doc["password"];
    
    Serial.print("wifi_ssid_access_point: ");
    Serial.println(wifi_ssid_access_point);
    Serial.print("wifi_password_access_point: ");
    Serial.println(wifi_password_access_point);

    Serial.println("Redirecting to /P0.html");
    DynamicJsonDocument responseDoc(1024);
    responseDoc["status"] = "redirect";
    responseDoc["url"] = "/P0.html";
    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);

    //Change Wifi access point credentials here and save them in EEPROM and then restart

  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}
