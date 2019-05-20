#include "BLEDevice.h"
#include <WiFi.h>
#include <PubSubClient.h>
#define wifi_ssid "Aintrz"
#define wifi_password "aaaaaaab"
const char* mqttServer = "demo.thingsboard.io";
const int mqttPort = 1883;
const char* mqttUser = "Yq416CnSzpVJtYRGvuuK";
const char* mqttPassword = "";

int status = WL_IDLE_STATUS;
WiFiClient espClient;
PubSubClient client(espClient);
static BLEUUID serviceUUID("97121d34-7ae0-11e9-8f9e-2a86e4085a59");
static BLEUUID charUUID("97122036-7ae0-11e9-8f9e-2a86e4085a59");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
String pData;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pDataT,
  size_t length,
  bool isNotify) {
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.println(length);
  Serial.print("data: ");
  pData = (char*)pDataT;
  Serial.println(pData);
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      Serial.print ("Mengkoneksi");
    }

    void onDisconnect(BLEClient* pclient) {
      connected = false;
      Serial.println("Terdiskonek");
    }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    pClient->disconnect();
    return false;
  }
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);

  if (pRemoteCharacteristic == nullptr) {
    pClient->disconnect();
    return false;
  }

  if (pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
    Serial.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;
      }
    }
};


void setup() {
  Serial.begin(115200);
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  client.setServer(mqttServer, mqttPort);
}

void reconnect() {
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(wifi_ssid, wifi_password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected Wifi");
    }
    Serial.print("Connecting to ThingsBoard node ...");
    if ( client.connect("ESPThingsboard", mqttUser, NULL) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED]" );
      Serial.println( client.state() );
      delay(1000);
    }
  }
}

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("YES");
    } else {
      Serial.println("FAIL");
    }
    doConnect = false;
  }
  if (connected) {
    String newValue = "Time since boot: " + String(millis() / 1000);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } else if (doScan) {
    BLEDevice::getScan()->start(0);
  }
  delay(5000);
  if ( !client.connected() ) {
    reconnect();
  }
  char attr[50];
  pData.toCharArray(attr, 50);
  client.publish( "v1/devices/me/telemetry", attr);
  delay(2000);
  client.loop();
}
