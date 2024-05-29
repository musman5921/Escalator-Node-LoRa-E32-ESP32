/*
 Project Name ------  FRS
 Task --------------  Escalator Node Firmware with Esp32
 Engineer ----------- Muhammad Usman
 File --------------- Functions Header File
 Company -----------  Machadev Pvt Limited
 */

// Functions.h
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>

// For LoRa Mesh networking
#include <RH_E32.h>
#include <RHMesh.h>
#include <vector>

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

struct NodeStatus {
    uint8_t nodeId;
    unsigned long lastSeen;
    bool isActive;
};

/**
 * @brief Converts a status code to a human-readable error string.
 *
 * @param status The status code to convert.
 * @return A pointer to a Flash string containing the error message.
 */
const __FlashStringHelper* getErrorString(uint8_t status);

/**
 * @brief Initializes the MESH network.
 *
 * @return true if the initialization is successful, false otherwise.
 */
bool initializeMESH();

/**
 * @brief Broadcasts the presence of the current node to other nodes in the network.
 */
void broadcastPresence();

/**
 * @brief Updates the status of a node given its ID.
 *
 * @param nodeId The ID of the node to update.
 */
void updateNodeStatus(uint8_t nodeId);

/**
 * @brief Checks the activity of all nodes in the network.
 *
 * This function verifies which nodes are active and which are inactive,
 * and takes appropriate actions based on the results.
 */
void checkNodeActivity();

/**
 * @brief Listens for incoming messages from other nodes.
 *
 * This function handle communication with other nodes, including receiving
 * and responding to messages.
 */
void listenForNodes();

/**
 * @brief Gets the total number of nodes currently in the network.
 *
 * @return The total number of nodes.
 */
size_t getTotalNodes();

/**
 * @brief Prints the statuses of all nodes in the network.
 *
 * This function output the current status (e.g., active, inactive)
 * of each node, typically for debugging or monitoring purposes.
 */
void printNodeStatuses();

/**
 * @brief Prints statistical information about the network.
 *
 * This may include metrics such as total number of nodes, average latency,
 * packet loss, or other relevant statistics.
 */
void printNetworkStats();

void handleFileRead(String path);
void handleRoot();
void handleSettingsOptionPost();
void handleConnectPost();
void handleClientLoginPost();
void handleAdminLoginPost();
void handleWifiConnectPost();
void handleUniqueKeyPost();
void handleFillDataAutoOrNotPost();
void handleGetCompanyDetailsPost();
void handleGetUnitDetailsPost();
void handleGetManufacturerDetailsPost();
void handleGetWifiAccessPointPost();


#endif // FUNCTIONS_H
