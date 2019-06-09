#include "DHT.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// DHT configuration
#define DHTTYPE DHT11
int DHTPIN = 4;
DHT dht(DHTPIN, DHTTYPE);

float temperature;
float humidity;
float oldTemperature;
float oldHumidity;
bool dataChanged = false;

// BLE configuration
BLEServer* pServer = NULL;
BLECharacteristic* pTemperatureCharacteristic = NULL;
BLECharacteristic* pHumidityCharacteristic = NULL;
BLEDescriptor* pTemperatureDescriptor = NULL;
BLEDescriptor* pHumidityDescriptor = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define NOTIFY_DELAY 2000
#define SERVICE_UUID BLEUUID((uint16_t)0x9999)
#define TEMP_UUID BLEUUID((uint16_t)0x2A6E)
#define HUM_UUID BLEUUID((uint16_t)0x2A6F)

class TempServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
  }
};


void setup() {
  Serial.begin(9600);
  Serial.println("Device is running");

  // Start DHT sensor
  Serial.println("Starting DHT Sensor");
  pinMode(DHTPIN, INPUT);
  dht.begin();
  Serial.println("Initial Sensor Reading");
  handleSensorReading();

  // Create the BLE Device
  Serial.println("Initializing ESP32 Bluetooth Device");
  BLEDevice::init("ESP32 TempSensor");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new TempServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE characteristics
  pTemperatureCharacteristic = pService->createCharacteristic(
                        TEMP_UUID,
                        BLECharacteristic::PROPERTY_READ  |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE 
                        );
  pTemperatureDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pTemperatureDescriptor->setValue("Temperature °C");
  pTemperatureCharacteristic->addDescriptor(pTemperatureDescriptor);
  pTemperatureCharacteristic->addDescriptor(new BLE2902());
  pTemperatureCharacteristic->setValue("n/a");
  
  pHumidityCharacteristic = pService->createCharacteristic(
                        HUM_UUID,
                        BLECharacteristic::PROPERTY_READ  |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE 
                        );
  pHumidityDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pHumidityDescriptor->setValue("Relative humidity %");
  pHumidityCharacteristic->addDescriptor(pHumidityDescriptor);
  pHumidityCharacteristic->addDescriptor(new BLE2902());
  pHumidityCharacteristic->setValue("n/a");
  
  // Start the service and begin advertising
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE server started advertising");
}

void loop() {
  delay(200);
  handleSensorReading();
  handleBLEConnection();
}

void handleBLEConnection() {
  if (deviceConnected && dataChanged) {
    pTemperatureCharacteristic->setValue(temperature);
    pTemperatureCharacteristic->notify();
    pHumidityCharacteristic->setValue(humidity);
    pHumidityCharacteristic->notify();
    Serial.println("Sent values");
    delay(NOTIFY_DELAY);
  }

  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("BLE server started advertising");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

void handleSensorReading() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  } else if (temperature != oldTemperature || humidity != oldHumidity)  {
    dataChanged = true;
    oldTemperature = temperature;
    oldHumidity = humidity;
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("°C ");
    Serial.print(" Humidity: ");
    Serial.print(humidity);
    Serial.print("%");
    Serial.print("\n");
  } 
}
