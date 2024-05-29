/*
 Project Name ------  FRS
 Task --------------  Escalator Node Firmware with Esp32
 Engineer ----------- Muhammad Usman
 File --------------- Constants
 Company -----------  Machadev Pvt Limited
 */

// constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <Arduino.h>

// Baud Rates
extern const int Baud_RATE_SERIAL;
extern const int Baud_RATE_LORA;

//  LoRa Pins
extern const int MOPIN;
extern const int M1PIN;
extern const int AUXPIN;
extern const int NODEID;
extern const int MAX_NODES; // not currently used

#endif // CONSTANTS_H
