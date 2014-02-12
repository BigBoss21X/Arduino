/*
 The Sensor Net Arduino library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>
	
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

#ifndef Sensor_h
#define Sensor_h

#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <Arduino.h>
#include <stddef.h>
#include <SPI.h>
#include <EEPROM.h>
#include <avr/progmem.h>
#include <stdarg.h>

//#define DEBUG

#ifdef DEBUG
#define debug(x,...) debugPrint(x, ##__VA_ARGS__)
#else
#define debug(x,...)
#endif


#define LIBRARY_VERSION "1.4"
#define PROTOCOL_VERSION 2
#define BAUD_RATE 115200

#define AUTO 0xFF // 0-254. Id 255 is reserved for auto initialization of radioId.
#define NODE_CHILD_ID 0xFF // Node child id is always created for when a new sensor is detected
#define EEPROM_RADIO_ID_ADDRESS 0 // Where to store radio id in EEPROM
#define EEPROM_RELAY_ID_ADDRESS 1 // Where to store relay id in EEPROM
#define EEPROM_DISTANCE_ADDRESS 2 // Where to store distance to gateway in EEPROM

// This is the radioId for sensor net gateway receiver sketch (where all sensors should send their data).
// This is also act as base value for sensor radioId
#define BASE_RADIO_ID ((uint64_t)0xABCDABC000LL)
#define GATEWAY_ADDRESS ((uint8_t)0)
#define BROADCAST_ADDRESS ((uint8_t)0xFF)
#define TO_ADDR(x) (BASE_RADIO_ID + x)

#define WRITE_PIPE ((uint8_t)0)
#define CURRENT_NODE_PIPE ((uint8_t)1)
#define BROADCAST_PIPE ((uint8_t)2)


#define WRITE_RETRY 5
#define FIND_RELAY_RETRIES 20


#define MAX_MESSAGE_LENGTH 32

// Message types
typedef enum {
	M_PRESENTATION = 0,
    M_SET_VARIABLE = 1,
    M_REQ_VARIABLE = 2,
    M_ACK_VARIABLE = 3,
    M_INTERNAL = 4
} messageType;

// Sensor variables that can be used in sketches
typedef enum {
	V_TEMP,V_HUM, V_LIGHT, V_DIMMER, V_PRESSURE, V_FORECAST, V_RAIN,
	V_RAINRATE, V_WIND, V_GUST, V_DIRECTION, V_UV, V_WEIGHT, V_DISTANCE,
	V_IMPEDANCE, V_ARMED, V_TRIPPED, V_WATT, V_KWH, V_SCENE_ON, V_SCENE_OFF,
	V_HEATER, V_HEATER_SW, V_LIGHT_LEVEL, V_VAR1, V_VAR2, V_VAR3, V_VAR4, V_VAR5,
	V_UP, V_DOWN, V_STOP, V_IR_SEND, V_IR_RECEIVE
} variableType;

// Internal messages
typedef enum {
	I_BATTERY_LEVEL, I_BATTERY_DATE, I_LAST_TRIP, I_TIME, I_VERSION, I_REQUEST_ID,
	I_INCLUSION_MODE, I_RELAY_NODE, I_LAST_UPDATE, I_PING, I_PING_ACK,
	I_LOG_MESSAGE, I_CHILDREN, I_UNIT
} internalMessageType;

// Sensor types
typedef enum {
	S_DOOR, S_MOTION, S_SMOKE, S_LIGHT, S_DIMMER, S_COVER, S_TEMP, S_HUM, S_BARO, S_WIND,
	S_RAIN, S_UV, S_WEIGHT, S_POWER, S_HEATER, S_DISTANCE, S_LIGHT_LEVEL, S_ARDUINO_NODE,
	S_ARDUINO_RELAY, S_LOCK, S_IR
} sensor;


/*
// Possible data types when sending variables.
enum {
	T_CUSTOM, T_CHAR, T_INT, T_UINT, T_LONG, T_ULONG, T_FLOAT, T_DOUBLE
} sendDataTypes;

*/

// Possible return values by validate() when doing crc check of received message.
enum {
	VALIDATE_OK, VALIDATE_BAD_CRC, VALIDATE_BAD_VERSION
};

// The message structure
typedef struct {
  uint8_t crc;    // 8 bits crc
  uint8_t version;    // (3 bits) protocol version
  uint8_t binary; 	 // (1 bit). Data is binary and should be encoded when sent to sensor net gateway
  uint8_t from;	 // 8 bits. RadioId of sender node
  uint8_t to;    // 8 bits. RadioId of destination node
  uint8_t last;    // 8 bits. RadioId of last node this message passed
  uint8_t childId;	 // 8 bits. Up to MAX_CHILD_DEVICES child sensors per radioId
  uint8_t messageType;    // (4 bits). Type of message. See messageType

  uint8_t type;	 // 8 bits. variableType or deviceType depending on messageType
} header_s;

typedef struct {
  header_s header;
  char data[MAX_MESSAGE_LENGTH - sizeof(header_s) + 1];  // Each message can transfer a payload. Add one extra byte for \0
} message_s;


class Sensor : public RF24
{
  public:
	/**
	* Constructor
	*
	* Creates a new instance of Sensor class.
	*
	* @param _cepin The pin attached to RF24 Chip Enable on the RF module
	* @param _cspin The pin attached to RF24 Chip Select
	*/
	Sensor(uint8_t _cepin, uint8_t _cspin);

	/**
	* Begin operation of the sensor net library
	*
	* Call this in setup(), before calling any other sensor net library methods.
	* @param _radioId The unique id (1-254) for this sensor. Specify 255 for auto id mode.
	*/
	void begin(uint8_t _radioId=AUTO);

	/**
	* The arduino node must send a presentation of all the sensors connected before any
	* variable changes will be registered on sensor net gateway side.
    * Usually it's good to present all sensors when arduino starts up (setup).
	* Note that gateway must be in include mode to register new devices.
	*
	* @param childId The unique child id for the different sensors connected to this arduino. 0-254.
	* @param sensorType Sensor types to create. They will be numbered from 0 up to 127.
	*/
	void sendSensorPresentation(uint8_t childId, uint8_t sensorType);


	/**
	* Sends a variable change to sensor net gateway (or another sensor if radioId is specified)
	*
	* @param radioId The radioId of other node in radio network (used for communicating between sensors)
	* @param childId The child id for which to update variable. Value can be 0-254.
	* @param variableType The variableType to update
	* @param value  New value of the variable
	*/

	//REMOVE THIS
	void sendVariable(uint8_t childId, uint8_t variableType, const char *value);


	//void sendStructure(uint8_t childId, uint8_t variableType, void *data, uint8_t len);
	void sendString(uint8_t childId, uint8_t variableType, const char *value);
	void sendDouble(uint8_t childId, uint8_t variableType, double value, int decimals);
	void sendULong(uint8_t childId, uint8_t variableType, unsigned long value);
	void sendLong(uint8_t childId, uint8_t variableType, long value);
	void sendUInt(uint8_t childId, uint8_t variableType, unsigned int value);
	void sendInt(uint8_t childId, uint8_t variableType, int value);

	void sendVariableNode(uint8_t radioId, uint8_t childId, uint8_t variableType, void *data, uint8_t length);
	void sendVariableNode(uint8_t radioId, uint8_t childId, uint8_t variableType, const char *value);
	void sendVariableNode(uint8_t radioId, uint8_t childId, uint8_t variableType, double value, int decimals);
	void sendVariableNode(uint8_t radioId, uint8_t childId, uint8_t variableType, unsigned long value);
	void sendVariableNode(uint8_t radioId, uint8_t childId, uint8_t variableType, long value);
	void sendVariableNode(uint8_t radioId, uint8_t childId, uint8_t variableType, unsigned int value);
	void sendVariableNode(uint8_t radioId, uint8_t childId, uint8_t variableType, int value);


	/**
	 * Sends battery status to sensor net gateway for this radio node.
	 * @param value Set a value between 0-100(%)
	 */
	void sendBatteryLevel(int value);

	/**
	* Requests a variable value from sensor net gateway (or another sensor). This method will not wait for an answer.
	* You should use mesageAvailable/getMessage to pick up the response.
	*
	* @param radioId The radioId of other node in radio network (used when fetching from other node)
	* @param childId  The unique child id for the different sensors connected to this arduino. 0-254.
	* @param variableType The variableType to fetch
	*/
	void requestStatus(uint8_t childId, uint8_t variableType);
	void requestStatus(uint8_t radioId, int8_t childId, uint8_t variableType);

	/**
	* Requests status for a sensor variable from sensor net gateway. Waits until message arrives.
	*
	* @param radioId The radioId of other node in radio network (used when fetching from other node)
	* @param childId  The unique child id for the different sensors connected to this arduino. 0-254.
	* @param variableType The variableType to fetch
	* @return The variable value
	*/
	char *getStatus(uint8_t childId, uint8_t variableType);
	char *getStatus(uint8_t radioId, int8_t childId, uint8_t variableType);


	/**
	 * Fetches time from sensor net gateway
	 */
	void requestTime();
	unsigned long getTime();

	/**
	 * Fetches unit setting from sensor net gateway. Returns true if metric system has been selected which means
	 * that sensor should report it's information in: celsius, meter, cm, gram, km/h, m/s etc..
	 * If false is returned the sensor should report data in imperial system which means
	 * fahrenheit, feet, gallon, mph etc...
	 */
	void requestIsMetricSystem();
	bool isMetricSystem();


	/**
	* Busy waits until there is a message for this node available to be read
	*/
	message_s waitForMessage(void);

	/**
	* Returns true if there is a message addressed for this node is available to be read
	*/
	boolean messageAvailable(void);

	/**
	* Returns the last received message
	*/
	message_s getMessage(void);

	/**
	 * Validates consistency of the message including CRC and protocol version
	 */
	uint8_t validate(uint8_t length);

	uint8_t getRadioId();


#ifdef DEBUG
	void debugPrint(const char *fmt, ... );
	int freeRam();
#endif


  protected:
	uint8_t failedTransmissions;
	boolean isRelay;
	uint8_t radioId;
	uint8_t distance; // This nodes distance to sensor net gateway (number of hops)
	uint8_t relayId;
	message_s msg;  // Buffer for incoming messages.
	char convBuffer[20];

	void setupRadio();
	void findRelay();
	boolean send(message_s message, int length);
	boolean sendWrite(uint8_t dest, message_s message, int length);
	boolean readMessage();
	void buildMsg(uint8_t from, uint8_t to, uint8_t childId, uint8_t messageType, uint8_t type, const char *data, uint8_t length, boolean binary);
	void sendInternal(uint8_t variableType, const char *value);
	boolean sendVariableAck();
	boolean sendData(uint8_t from, uint8_t to, uint8_t childId, uint8_t messageType, uint8_t type, const char *data, uint8_t length, boolean binaryMessage);



  private:
	message_s ack;  // Buffer for ack messages.

	void initializeRadioId();
	uint8_t crc8Message(message_s, uint8_t length);
	char* get(uint8_t nodeId, uint8_t childId, uint8_t sendType, uint8_t receiveType, uint8_t variableType);
	char *getInternal(uint8_t variableType);
};

#endif
