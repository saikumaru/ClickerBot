/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string.h>


BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic1 = NULL;//write count of images
BLECharacteristic* pCharacteristic2 = NULL;//read click- notified
BLECharacteristic* pCharacteristic3 = NULL;//read write stop
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID1 "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID2 "fac0db31-730a-4c90-9424-3f802a47c021"
#define CHARACTERISTIC_UUID3 "4892d430-5a19-43ef-a5ce-d62ab8d43f50"

uint8_t numberOfImages = 0; //integer written by app
uint8_t clickNow = 0; //1 for click, bot resumes once this is made 0 by app
uint8_t startBot = 0; //1 for moving 0 for stop

uint8_t PREV_numberOfImages = 0; //integer written by app
uint8_t PREV_clickNow = 0; //1 for click, bot resumes once this is made 0 by app
uint8_t PREV_startBot = 0; //1 for moving 0 for stop


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


//ref https://platformio.org/lib/show/1841/ESP32%20BLE%20Arduino/examples?file=BLE_uart.ino
//ref https://esp32.com/viewtopic.php?t=12955
class MyCallbacks1: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");


        clickNow = atoi(rxValue.c_str());

        Serial.print("numberOfImages is :");
        Serial.println(numberOfImages);
      }

    }
};
class MyCallbacks2: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");


        clickNow = atoi(rxValue.c_str());

        Serial.print("value clickNow is :");
        Serial.println(clickNow);
      }
    }
};
class MyCallbacks3: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");



        startBot = atoi(rxValue.c_str());

        Serial.print("value startBot is :");
        Serial.println(startBot);
      }
    }
};

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic1 = pService->createCharacteristic(
                       CHARACTERISTIC_UUID1,
                       BLECharacteristic::PROPERTY_READ   |
                       BLECharacteristic::PROPERTY_WRITE  |
                       BLECharacteristic::PROPERTY_NOTIFY |
                       BLECharacteristic::PROPERTY_INDICATE
                     );
  pCharacteristic1->setCallbacks(new MyCallbacks1());

  // Create a BLE Characteristic
  pCharacteristic2 = pService->createCharacteristic(
                       CHARACTERISTIC_UUID2,
                       BLECharacteristic::PROPERTY_READ   |
                       BLECharacteristic::PROPERTY_WRITE  |
                       BLECharacteristic::PROPERTY_NOTIFY |
                       BLECharacteristic::PROPERTY_INDICATE
                     );
  pCharacteristic2->setCallbacks(new MyCallbacks2());

  // Create a BLE Characteristic
  pCharacteristic3 = pService->createCharacteristic(
                       CHARACTERISTIC_UUID3,
                       BLECharacteristic::PROPERTY_READ   |
                       BLECharacteristic::PROPERTY_WRITE  |
                       BLECharacteristic::PROPERTY_NOTIFY |
                       BLECharacteristic::PROPERTY_INDICATE
                     );
  pCharacteristic3->setCallbacks(new MyCallbacks3());


  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic1->addDescriptor(new BLE2902());
  pCharacteristic2->addDescriptor(new BLE2902());
  pCharacteristic3->addDescriptor(new BLE2902());

  char bufferx[3];
  sprintf(bufferx, "%d", numberOfImages);
  std::string myStringForUnit8(bufferx);
  pCharacteristic1->setValue(myStringForUnit8);

  sprintf(bufferx, "%d", clickNow);
  myStringForUnit8.assign(bufferx);
  pCharacteristic2->setValue(myStringForUnit8);

  sprintf(bufferx, "%d", startBot);
  myStringForUnit8.assign(bufferx);
  pCharacteristic3->setValue(myStringForUnit8);





  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}


void loop() {
  // notify changed value
  if (deviceConnected) {
    //pCharacteristic->setValue((uint8_t*)&value, 4);
    pCharacteristic1->notify();
    pCharacteristic2->notify();
    pCharacteristic3->notify();
    delay(10); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }




  while (Serial.available() > 0) {
    clickNow = Serial.parseInt();
    Serial.print("Clicked");
    Serial.println(clickNow);

    char bufferx[3];
    sprintf(bufferx, "%d", clickNow);
    std::string myStringForUnit8(bufferx);
    pCharacteristic2->setValue(myStringForUnit8);
    pCharacteristic2->notify();
  }


}

void clickCamera() {

}
