#include <ESP8266WiFi.h>

extern "C" {
#include "user_interface.h"
}


byte channel;
int wifiCount = 0;
int fullPacket;
int switchGPIO0;
int switchGPIO2;
String wifissid;

bool promiscuousMode = false;

bool mimicMode = false;
bool scan = false;

bool randomMode = false;
int randomStart;

bool listMode = false;
char* networkList[] = {"attwifi", "HomeNetwork", "Starbucks", "guest", "ATT_8765", "Beaches", "Seagulls" };


bool staticMode = false;
String staticNet = "¯\\_(ツ)_/¯";





void setup() {
  delay(1);
  WiFi.mode(WIFI_STA);
  // Initialize the LED_BUILTIN pin as an output
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize the GPIO pins as inputs
  pinMode(0, INPUT);
  pinMode(2, INPUT);
  // Wait 5 seconds to set switches
  for (int i = 0; i < 5; ++i)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
  }

  // Check the GPIO config set up and determine the operation mode being used
  switchGPIO0 = digitalRead(0);
  switchGPIO2 = digitalRead(2);
  delay(1);

  if (switchGPIO0 == HIGH && switchGPIO2 == HIGH)
  {
    mimicMode = true;
    scan = true;
    delay(1);
  }
  else if (switchGPIO0 == LOW && switchGPIO2 == HIGH)
  {
    randomMode = true;
    delay(1);
  }
  else if (switchGPIO0 == HIGH && switchGPIO2 == LOW)
  {
    listMode = true;
    delay(1);
  }
  else //(switchGPIO0 == LOW && switchGPIO2 == LOW)
  {
    staticMode = true;
    delay(1);
  }


}







void loop() {

  // Write blue LED HIGH (off)
  digitalWrite(LED_BUILTIN, HIGH);

  // Do an initial scan to find available network names
  if (scan == true)
  {
    wifiCount = WiFi.scanNetworks();
    // Make sure this doesn't run again now that the scan is done
    scan = false;
    // Wait 3 seconds for the other modules before starting
    for (int i = 0; i < 3; ++i)
    {
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
    }
  }
  else if (randomMode == true)
  {
    wifiCount = 50;
    // So after 50 loops below, pick a new random number
    randomStart = random(23560);
  }
  else if (listMode == true)
  {
    wifiCount = 6;

  }
  else if (staticMode == true)
  {
    wifiCount = 1;
  }




  if (promiscuousMode == false)
  {
    promiscuousMode = true;
    wifi_set_opmode(STATION_MODE);
    wifi_promiscuous_enable(1);
  }


  delay(1);




  // Start looping through SSID's
  for (int i = 0; i < wifiCount; ++i)
  {
    // Get the SSID to use this round (increasing array value changes SSID)
    if (mimicMode == true)
    {
      // Pick a random SSID
      //int wifiCountArray = wifiCount - 1;
      wifissid = WiFi.SSID(random(wifiCount));
    }
    else if (randomMode == true)
    {
      // Just start at the picked random number and add 1 each loop
      int randVal = randomStart + 1;
      wifissid = String(randVal);
    }
    else if (listMode == true)
    {
      wifissid = networkList[i];
    }
    else
    {
      wifissid = staticNet;
    }



    // Beacon Packet buffer ( this resets the variable every loop. weird things happened when it was global... )
    uint8_t packet[128] = { 0x80, 0x00, 0x00, 0x00,
                            /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                            /*10*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                            /*16*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                            /*22*/  0xc0, 0x6c,
                            /*24*/  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00,
                            /*32*/  0x64, 0x00,
                            /*34*/  0x01, 0x04,
                            /* SSID */
                            /*36*/  0x00, 0x01, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                            /*71*/  0x01, 0x08, 0x82, 0x84,
                            /*75*/  0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01,
                            /*83*/  0x05, 0x20
                          };


    // Get the SSID length to modify that length value in the beacon
    int wifiLength = wifissid.length();
    // Array spot to start printing SSID
    int ssidSpot = 38;
    // If the SSID is too long, cut it
    if (wifiLength > 30)
    {
      wifiLength = 30;
    }


    // Start printing the SSID in to the packet array at the start spot stated above
    for (int i = 0; i < wifiLength; ++i)
    {
      packet[ssidSpot] = wifissid[i];
      ++ssidSpot;
    }

    // BROADCAST LOOP. Start with 1 space added to SSID and continue until 30 characters long
    for (int j = wifiLength; j < 30; ++j)
    {
      // Kill the "add a space each iteration" looping here if you want



      // Add a space right after the SSID that was printed above
      packet[ssidSpot] = packet[84];
      // Print the now complete SSID length right before the SSID start spot in packet
      packet[37] = j + 1;



      // Move to the next array spot after the last space was added
      ++ssidSpot;

      // Pick a channel to use
      channel = random(1, 12);
      wifi_set_channel(channel);

      // Change the last array value in the "ending" part of the SSID beacon to the channel being used
      packet[83] = channel;
      // Just copying the left off ssid spot for the second part of the packet to start
      int packetPartTwo = ssidSpot;

      // Print the complete second packet portion in to the complete packet array now that the SSID is printed in
      for (int m = 71; m < 84; ++m)
      {
        packet[packetPartTwo] = packet[m];
        ++packetPartTwo;
      }

      // Random MAC address
      packet[10] = packet[16] = random(256);
      packet[11] = packet[17] = random(256);
      packet[12] = packet[18] = random(256);
      packet[13] = packet[19] = random(256);
      packet[14] = packet[20] = random(256);
      packet[15] = packet[21] = random(256);

      // Broadcast the Beacons!
      fullPacket = packetPartTwo; // + 1;
      // Write blue LED LOW (on)
      digitalWrite(LED_BUILTIN, LOW);
      wifi_send_pkt_freedom(packet, fullPacket, 0);
      wifi_send_pkt_freedom(packet, fullPacket, 0);
      wifi_send_pkt_freedom(packet, fullPacket, 0);


      // Write blue LED HIGH (off)
      digitalWrite(LED_BUILTIN, HIGH);

      delay(10);
    }
    delay(10);
  }
  delay(10);
}


