/*
 *  This sketch sends random data over UDP on a ESP32 device
 *
 */
#include <WiFi.h>
#include <WiFiUdp.h>

#include "vrt.h"
#include "tweetnacl.h"

#define CHECK(x)                                                               \
  do {                                                                         \
    int ret;                                                                   \
    if ((ret = x) != VRT_SUCCESS) {                                            \
    fprintf(stderr, "%s:%u: ret %u\n", __func__, __LINE__, ret); \
      return (ret);                                                            \
    }                                                                          \
  } while (0)

struct rt_server {
    const char *host;
    unsigned port;
    int variant;
    uint8_t public_key[32];
};

struct rt_server servers[] = {
    { "172.105.3.84", 2002, 0, { 0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60 } }, /* caesium.tannerryan.ca */
    { "162.159.200.123", 2002, 0, { 0x80,0x3e,0xb7,0x85,0x28,0xf7,0x49,0xc4,0xbe,0xc2,0xe3,0x9e,0x1a,0xbb,0x9b,0x5e,0x5a,0xb7,0xe4,0xdd,0x5c,0xe4,0xb6,0xf2,0xfd,0x2f,0x93,0xec,0xc3,0x53,0x8f,0x1a } }, /* roughtime.cloudflare.com */
    { "35.192.98.51", 2002, 0, { 0x01,0x6e,0x6e,0x02,0x84,0xd2,0x4c,0x37,0xc6,0xe4,0xd7,0xd8,0xd5,0xb4,0xe1,0xd3,0xc1,0x94,0x9c,0xea,0xa5,0x45,0xbf,0x87,0x56,0x16,0xc9,0xdc,0xe0,0xc9,0xbe,0xc1 } }, /* roughtime.int08h.com */
    { "192.36.143.134", 2002, 1, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "193.180.164.51", 2002, 1, { 0x19,0xd1,0xa9,0xf6,0x6e,0x40,0x6f,0x0a,0x82,0x3a,0x94,0xd5,0x62,0xaf,0xb2,0x96,0x20,0x48,0xaf,0x38,0x75,0xc1,0xe7,0x7b,0x55,0x84,0x52,0xe4,0x2c,0x4c,0x63,0x75 } }, /* roughtime.weinigel.se */
    { "194.58.207.198", 2002, 1, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 1, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.196", 2002, 1, { 0xb4,0x03,0xec,0x41,0xcd,0xc3,0xdf,0xa9,0x89,0x3c,0xe5,0xf5,0xfc,0xb2,0xcd,0x6d,0x5d,0x0c,0xdd,0xfb,0x93,0x3e,0x3c,0x16,0xe7,0x89,0x86,0xbf,0x0f,0x95,0xd6,0x11 } }, /* lab.roughtime.netnod.se */
    { "194.58.207.197", 2002, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2000, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2001, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2003, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2004, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2005, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2006, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2007, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2008, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2009, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
};

// WiFi network name and password:
const char * networkName = "Carl-Williams iPhone";
const char * networkPswd = "12345678";

//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;

void setup(){
  // Initilize hardware serial:
  Serial.begin(115200);
  
  //Connect to the WiFi network
  connectToWiFi(networkName, networkPswd);
}

void loop(){
  //only send data when connected
  if(connected){
    //Send a packet
    doit(&servers[1]);
  }
  //Wait for 1 second
  delay(5000);
}

void connectToWiFi(const char * ssid, const char * pwd){
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  
  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          connected = true;
          break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
      default: break;
    }
}

int doit(struct rt_server *server){

  uint32_t recv_buffer[VRT_QUERY_PACKET_LEN / 4] = {0};
  uint8_t query[VRT_QUERY_PACKET_LEN] = {0};
  uint64_t out_midpoint;
  uint32_t out_radii;

  /* prepare query */
  //uint32_t t = esp_random();
  uint8_t nonce[VRT_NONCE_SIZE] = "preferably a random byte buffer";
  CHECK(vrt_make_query(nonce, sizeof(nonce), query, sizeof(query), server->variant));

  /* send query */
  udp.begin(WiFi.localIP(), server->port);
  udp.beginPacket(server->host, server->port);
  udp.write(query, sizeof(query));
  udp.endPacket();

  while(udp.parsePacket() == 0){}
  
  if(udp.read((char*)recv_buffer, sizeof(query)) > 0)
  {
    CHECK(vrt_parse_response(nonce, sizeof(nonce), recv_buffer,
                            sizeof(recv_buffer),
                            server->public_key, &out_midpoint,
                            &out_radii, server->variant));

    printf("midp %" PRIu64 " radi %u\n", out_midpoint, out_radii);
  }
  return 0;
}