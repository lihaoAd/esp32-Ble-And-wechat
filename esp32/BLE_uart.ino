
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

// 继电器IN的输入口
#define JQS_PIN 12 

//String chipId;
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("onConnect");
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("onDisconnect");
  }
};
String resStr;
String chipId;
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    Serial.println("onWrite");
    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
        resStr += rxValue[i];
      }
      Serial.println();
      Serial.println("*********");
      if (resStr == "getid") {
        pTxCharacteristic->setValue(chipId.c_str());
        pTxCharacteristic->notify();
      } else if (resStr == "GREEN") {
        Serial.println("GREEN");
        digitalWrite(JQS_PIN, HIGH);
      } else if (resStr == "BLUE") {
        Serial.println("BLUE");
        digitalWrite(JQS_PIN, LOW);
      } 
      resStr = "";
    }
  }
};


void setup() {
  Serial.begin(115200);
  pinMode(JQS_PIN, OUTPUT);
  chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();
  Serial.println(chipId);
  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY);

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  // pServer->getAdvertising()->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Waiting a client connection to notify...");
}
String readString;
void loop() {

  if (deviceConnected) {
    //        pTxCharacteristic->setValue(&txValue, 1);
    //        pTxCharacteristic->notify();
    //        txValue++;
    //    delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }
  while (Serial.available() > 0) {
    if (deviceConnected) {
      delay(3);
      readString += Serial.read();
      pTxCharacteristic->setValue(chipId.c_str());
      //      pTxCharacteristic->setValue((uint32_t)ESP.getEfuseMac());
      pTxCharacteristic->notify();
      Serial.println(chipId);
    }
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
