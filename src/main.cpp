#include <Arduino.h>
#include <DMXSerial.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <SPI.h>

char host[] = "red-old.lambda8.at";

char path[] = "/gw/all/";

// Set the static IP address to use if the DHCP fails to assign
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

EthernetClient client;

void httpRequest(char* host, char* path);

void writeChannels(uint16_t channels[], uint8_t size, uint8_t data, boolean rgb = true);

uint16_t kiosk[] = {106, 113, 120, 127, 134, 141};
uint16_t unten[] = {491};
uint16_t loge[] = {492, 494};
uint16_t treppe[] = {493};

uint16_t* channels[] = {kiosk, unten, loge, treppe};
uint8_t sizes[] = {6, 1, 2, 1};

uint8_t color[] = {255, 117, 17};

void setup() {
  DMXSerial.init(DMXController);

  EEPROM.get(0, color[0]);
  EEPROM.get(1, color[1]);
  EEPROM.get(2, color[2]);

  if (Ethernet.begin(mac) == 0) {
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      while (true) {
        delay(1);  // do nothing, no point running without Ethernet hardware
      }
    }
    Ethernet.begin(mac, ip, myDns);
  }
}

int StrToHex(const char str[]) { return (int)strtol(str, 0, 16); }

uint32_t requestTimer = 0;
void loop() {
  if (millis() - requestTimer > 500) {
    requestTimer = millis();
    if (client.connect(host, 80)) {
      httpRequest(host, path);
    }
  }

  if (client.available()) {
    while (client.read() != ' ');
    if (client.readStringUntil(' ') != "200") {
      return;
    }
    
    while (client.read() != '[');

    String col = client.readStringUntil(',');
    
    for(uint8_t i = 0; i < 3; i++){
      color[i] = StrToHex(col.substring(i + 1, i + 3).c_str());
      EEPROM.update(i, color[i]);
    }

    writeChannels(channels[0], sizes[0], client.readStringUntil(',').toInt(), true);   // kiosk
    writeChannels(channels[2], sizes[2], client.readStringUntil(',').toInt(), false);  // loge
    writeChannels(channels[1], sizes[1], client.readStringUntil(',').toInt(), false);  // unten
    writeChannels(channels[3], sizes[3], client.readStringUntil(']').toInt(), false);  // treppe
    while (client.available()) client.read();

    DMXSerial.write(512, 0);
  }
  delay(1);
}

void writeChannels(uint16_t channels[], uint8_t size, uint8_t data, boolean rgb) {
  for (uint8_t i = 0; i < size; i++) {
    if (rgb) {
      DMXSerial.write(channels[i] + 0, map(data, 0, 255, 0, color[0]));  // R
      DMXSerial.write(channels[i] + 1, map(data, 0, 255, 0, color[1]));  // G
      DMXSerial.write(channels[i] + 2, map(data, 0, 255, 0, color[2]));  // B
    } else {
      DMXSerial.write(channels[i], data);
    }
  }
}

void httpRequest(char* host, char* path) {
  client.print("GET ");
  client.print(path);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(host);
  client.println("Connection: close");
  client.println();
}
