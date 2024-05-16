/*
Developing a mesh network using Ebyte LoRa E32 with ESP32
The logic is same for each node

Flow:
The program is same for all nodes in the network
  Each node listen to other nodes in a loop
  Each node update activity of other nodes after every 10 seconds
  Each node broadcast message after every 30 seconds
  Each node print stats of network after every 60 seconds

Tests:
The network contains 4 nodes
  Each node knows how many nodes are available, active and dead in his network
  Each node successfully updates its node info container after every 60 seconds
  
platformio:
  board selected: espressif esp32 dev module 
  monitor speed: 115200

*/
/*

Escalator node:
AUX pin of LoRa module is NOT connected with ESP32 
Relay is active Low
and an led is connected in series with relay

no ack if message is broadcasted

*/

#include <Arduino.h>
#include <RHMesh.h>
#include <RH_E32.h>
#include <vector>

#define RH_HAVE_SERIAL
#define MOPIN 18
#define M1PIN 19
#define AUXPIN 32
#define RLYPIN  21
#define NODEID  1

struct NodeStatus {
    uint8_t nodeId;
    unsigned long lastSeen;
    bool isActive;
};

std::vector<NodeStatus> nodeStatuses;

RH_E32 driver(&Serial2, MOPIN, M1PIN, AUXPIN); //M0, M1, AUX
RHMesh mesh(driver, NODEID); // Node ID 1 for this example

const __FlashStringHelper* getErrorString(uint8_t status);

void broadcastPresence();
void updateNodeStatus(uint8_t nodeId);
void checkNodeActivity();
void listenForNodes();
size_t getTotalNodes();
void printNodeStatuses();
void printNetworkStats();

void setup() {
  Serial.begin(115200);
  Serial.println("Serial is ready.");

  Serial2.begin(9600);
  Serial.println("Serial2 is ready.");

  Serial.println("Initializing mesh...");
  if (!mesh.init()) {
        Serial.println("Mesh initialization failed");
        return;
    } 
    Serial.println("Mesh initialized successfully.");
}

void loop() {
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
        printNodeStatuses();  // Print the statuses of all nodes
        printNetworkStats(); 
        lastStatusPrintTime = currentMillis;
    }
}

void broadcastPresence() {
    const char* presenceMsg = "Node Present";
    uint8_t status = mesh.sendtoWait((uint8_t*)presenceMsg, strlen(presenceMsg) + 1, RH_BROADCAST_ADDRESS);
    if (status == RH_ROUTER_ERROR_NONE) {
        Serial.println("Message sent successfully");
    } else {
        Serial.print("Failed to send message, error: ");
        Serial.println(status);
        Serial.println((const __FlashStringHelper*)getErrorString(status));
    }
}

// Function to listen for other nodes and update their status
void listenForNodes() {
    uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from;

    if (mesh.recvfromAckTimeout(buf, &len, 2000, &from)) {
        Serial.print("Received message from node ");
        Serial.print(from);
        Serial.print(": ");
        Serial.println((char*)buf);

        // Update node information or add new node
        updateNodeStatus(from);
    }
}

// Function to update or add a node status in the list
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

// Function to check and update the activity status of nodes
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

// Function to get the total number of known nodes
size_t getTotalNodes() {
  return nodeStatuses.size();
}

// Function to print the status of all nodes known to this node
void printNodeStatuses() {
  size_t totalNodes = getTotalNodes();
    Serial.print("Total Nodes: ");
    Serial.println(totalNodes);

    for (const auto& status : nodeStatuses) { // range based loop
        Serial.print("Node ");
        Serial.print(status.nodeId);
        Serial.print(": ");
        Serial.println(status.isActive ? "Active" : "Dead");
    }
}

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
