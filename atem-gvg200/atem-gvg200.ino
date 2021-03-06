/*****************
   Basis control for the SKAARHOJ C20x series devices
   This example is programmed for ATEM 1M/E versions

   This example also uses several custom libraries which you must install first.
   Search for "#include" in this file to find the libraries. Then download the libraries from http://skaarhoj.com/wiki/index.php/Libraries_for_Arduino

   Works with ethernet-enabled arduino devices (Arduino Ethernet or a model with Ethernet shield)
   Run the example sketch "ConfigEthernetAddresses" to set up Ethernet before installing this sketch.

   - kasper

*/

/*
   Dodano za digole serial - uporablja še vedno SoftwareSerial lib

   popravljene metode in klici
   ni vec command statusa

   - Ferbi
*/

/*
   Dodano čiščenje Value na selektorjih ter pomnenje User Button preseta

   - Anže
*/


// Including libraries:
#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include "WebServer.h"  // For web interface
#include <EEPROM.h>      // For storing IP numbers
#include <MenuBackend.h>  // Library for menu navigation. Must (for some reason) be included early! Otherwise compilation chokes.
#include <SkaarhojTools.h>

SkaarhojTools sTools(1);

static uint8_t default_ip[] = {     // IP for Configuration Mode (192.168.0.99)
  192, 168, 0, 99
};
uint8_t ip[4];        // Will hold the C201 IP address
uint8_t atem_ip[4];  // Will hold the ATEM IP address
uint8_t mac[6];    // Will hold the Arduino Ethernet shield/board MAC address (loaded from EEPROM memory, set with ConfigEthernetAddresses example sketch)


byte ledke = 0;

byte input = 0;

byte ledke_old[4] = {
  0, 0, 0, 0
};

bool pressedCut = false;
bool pressedAuto = false;
long long previousAutoTime = 0;
long long previousCutTime = 0;

//
//Black|0
//ColorBars | 1000
//Color 1|2001
//Color 2|2002
//Media Player 1|3010
//Media Player 1 Key|3011
//Media Player 2|3020
//Media Player 2 Key|3021
//Key Mask 1|4010
//DSK1 Mask|5010
//DSK2 Mask|5020
//Clean Feed 1|7001
//Clean Feed 2|7002
//Program|10010
//Preview|10011

byte izhod = 0;




byte tipkeSelect = 0b00000000;
int AuxTally;
uint16_t AuxTally_Old = 96969;
uint16_t AtemPreviewTally_Old = 96969;
uint16_t AtemProgramTally_Old = 96969;

/* MEMORY USAGE:
   This can be used to track free memory.
   Include "MemoryFree.h" and use the following line
       Serial << F("freeMemory()=") << freeMemory() << "\n";
   in your code to see available memory.
*/
//#include <MemoryFree.h>



// No-cost stream operator as described at
// http://arduiniana.org/libraries/streaming/
template<class T> inline Print &operator <<(Print &obj, T arg)
{
  obj.print(arg);
  return obj;
}


// Include ATEM library and make an instance:
#include <ATEMext.h>
ATEMext AtemSwitcher;

// All related to library "SkaarhojBI8", which controls the buttons:
#include "Wire.h"
#include "MCP23017.h"
#include "PCA9685.h"
#include "SkaarhojBI8.h"
SkaarhojBI8 previewSelect;
SkaarhojBI8 programSelect;
SkaarhojBI8 cmdSelect;
SkaarhojBI8 extraButtons;

// All related to library "SkaarhojUtils". Used for rotary encoder and "T-bar" potentiometer:
#include "SkaarhojUtils.h"
SkaarhojUtils utils;




/*************************************************************


                       MENU SYSTEM


 **********************************************************/


uint8_t userButtonMode = 0;  // 0-3
uint8_t setMenuValues = 0;  //
uint8_t BUSselect = 0;  // Preview/Program by default
uint16_t inputs[21] = {0,1,2,3,4,5,6,7,8,1000,2001,2002,3010,3011,3020,3021,4010,5010,5020,7001,7002};
unsigned long timeoutMillis = 0;
bool timeoutDone = true;










/*************************************************************


                       Webserver


 **********************************************************/


#define PREFIX ""
WebServer webserver(PREFIX, 80);

void logoCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  /* this data was taken from a PNG file that was converted to a C data structure
     by running it through the directfb-csource application.
     (Alternatively by PHPSH script "HeaderGraphicsWebInterfaceInUnitsPNG8.phpsh")
  */
  P(logoData) = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00,
    0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x02, 0xa2,
    0x00, 0x00, 0x00, 0x2b, 0x08, 0x03, 0x00, 0x00, 0x00, 0x94,
    0xff, 0x1a, 0xf8, 0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 0x58,
    0x74, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x00,
    0x41, 0x64, 0x6f, 0x62, 0x65, 0x20, 0x49, 0x6d, 0x61, 0x67,
    0x65, 0x52, 0x65, 0x61, 0x64, 0x79, 0x71, 0xc9, 0x65, 0x3c,
    0x00, 0x00, 0x00, 0x30, 0x50, 0x4c, 0x54, 0x45, 0xe6, 0xe6,
    0xe6, 0x0d, 0x5c, 0xaf, 0x0c, 0x61, 0xbd, 0x57, 0x89, 0xcc,
    0xd7, 0xe2, 0xf2, 0xfa, 0xfa, 0xfa, 0x85, 0xa8, 0xd9, 0xab,
    0xc2, 0xe3, 0xc7, 0xd6, 0xec, 0x0b, 0x51, 0x9d, 0x36, 0x76,
    0xc4, 0x23, 0x68, 0xb9, 0x0a, 0x65, 0xc5, 0x0a, 0x65, 0xc6,
    0x0d, 0x5e, 0xb5, 0xff, 0xff, 0xff, 0xe6, 0xb3, 0x8a, 0x8c,
    0x00, 0x00, 0x07, 0x1e, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda,
    0xec, 0x5d, 0x0d, 0x7b, 0xa3, 0x20, 0x0c, 0x56, 0xa8, 0xc0,
    0xd4, 0xe2, 0xff, 0xff, 0xb7, 0x67, 0x05, 0x42, 0x12, 0x82,
    0xda, 0xae, 0xf7, 0xcc, 0x6e, 0xc4, 0x75, 0x53, 0xf2, 0x8d,
    0xaf, 0xaf, 0xca, 0xed, 0xee, 0xba, 0x25, 0xca, 0xe0, 0xc6,
    0xf9, 0xde, 0xa4, 0xc9, 0x45, 0x44, 0x8d, 0x6e, 0x08, 0xc8,
    0xec, 0x00, 0xa1, 0xaa, 0x4d, 0x4b, 0x93, 0x2b, 0xc9, 0x1c,
    0x31, 0x1a, 0x21, 0x3a, 0x0c, 0x63, 0x9b, 0x93, 0x26, 0x17,
    0xe3, 0x51, 0x06, 0xd1, 0x46, 0xa2, 0x4d, 0xae, 0x86, 0xd1,
    0x61, 0xc0, 0x37, 0xfa, 0xae, 0xcd, 0x48, 0x93, 0xab, 0x49,
    0x87, 0x21, 0x3a, 0x34, 0x88, 0x36, 0xb9, 0x9c, 0x10, 0x16,
    0x1d, 0x86, 0x36, 0x21, 0x4d, 0x2e, 0xce, 0xa2, 0xf3, 0xbc,
    0xbe, 0x43, 0xa5, 0x77, 0xa9, 0xf8, 0xb9, 0xa3, 0x65, 0xa8,
    0xa0, 0x9f, 0x83, 0x6a, 0xfd, 0x31, 0x0b, 0x23, 0xd9, 0x75,
    0x4e, 0xdb, 0x9c, 0x62, 0x04, 0xbb, 0xcd, 0x67, 0xfb, 0x1e,
    0x13, 0x91, 0x48, 0x31, 0x54, 0xb0, 0x03, 0x1b, 0xe4, 0x99,
    0x7d, 0xe6, 0x6c, 0x1f, 0x7d, 0x90, 0xd1, 0x7c, 0xa7, 0xda,
    0x58, 0xfd, 0x7c, 0xc7, 0x3d, 0xcd, 0x2c, 0x4b, 0x2a, 0x1c,
    0x77, 0x9e, 0x5b, 0x40, 0xe3, 0x78, 0x52, 0x50, 0x40, 0xc8,
    0x87, 0xcd, 0x52, 0xc5, 0x77, 0x16, 0x36, 0x55, 0x19, 0x1d,
    0x67, 0xe8, 0xfb, 0x8e, 0x6b, 0x7c, 0x29, 0xda, 0xcb, 0x15,
    0x3c, 0xe9, 0xfa, 0xe6, 0x66, 0xd1, 0x11, 0xc0, 0xa3, 0x23,
    0x8b, 0x4e, 0xdd, 0xfc, 0xa7, 0xe4, 0x7e, 0x46, 0x73, 0xff,
    0xd9, 0x4a, 0x7e, 0x7e, 0x2e, 0x7e, 0x3a, 0x15, 0x7f, 0x16,
    0x9d, 0x9b, 0x34, 0xb9, 0x96, 0x9c, 0x64, 0x51, 0x63, 0x6e,
    0xa5, 0x98, 0xf5, 0x82, 0xd0, 0xdb, 0x9e, 0x4e, 0x76, 0x3a,
    0xab, 0xc2, 0x05, 0x33, 0xd2, 0xe3, 0x2c, 0xa5, 0x42, 0xdf,
    0xa4, 0x1c, 0x63, 0x56, 0x59, 0xe4, 0xee, 0x68, 0xde, 0xd5,
    0x24, 0x97, 0x68, 0x8c, 0x11, 0x2f, 0x5e, 0x53, 0x44, 0x69,
    0xf2, 0x01, 0xb7, 0xba, 0x53, 0x2c, 0x2a, 0xe1, 0xf3, 0x01,
    0x90, 0x7b, 0x02, 0x4f, 0x80, 0x8a, 0x02, 0x94, 0xe9, 0x3b,
    0x77, 0x1d, 0xe7, 0x68, 0x52, 0x51, 0xd4, 0x20, 0x1a, 0x40,
    0x7c, 0x04, 0xd1, 0x89, 0x55, 0xe8, 0x6e, 0x46, 0xe3, 0x06,
    0x14, 0x4a, 0xf9, 0x46, 0x88, 0xaa, 0xff, 0x7e, 0x7e, 0xd4,
    0x5f, 0x02, 0xa3, 0x3a, 0xcb, 0xa2, 0x8a, 0x6d, 0xb3, 0xb2,
    0xb7, 0x0a, 0x78, 0x14, 0x86, 0x28, 0x1c, 0xdc, 0x9c, 0x86,
    0x30, 0x23, 0x02, 0x9a, 0x0a, 0x35, 0x94, 0x8a, 0x38, 0xa4,
    0xeb, 0x69, 0x12, 0x44, 0x73, 0x55, 0x09, 0xa2, 0xdb, 0x91,
    0xa9, 0xb8, 0xa9, 0x14, 0x3a, 0xfc, 0xb4, 0x45, 0x94, 0xdc,
    0x63, 0x65, 0xdb, 0xd3, 0xed, 0xaa, 0x9e, 0x51, 0x9e, 0xb1,
    0x7b, 0x25, 0xd5, 0x37, 0x2a, 0x78, 0x36, 0xdd, 0x7b, 0x9b,
    0x25, 0x87, 0xfc, 0x59, 0x34, 0xe9, 0xb1, 0x69, 0x82, 0x8e,
    0x1d, 0x85, 0x18, 0x09, 0xa2, 0x0a, 0xf6, 0x6f, 0x6e, 0xcc,
    0xc9, 0x32, 0x78, 0x98, 0xb3, 0xa0, 0xe0, 0x38, 0x34, 0x16,
    0x2c, 0xf6, 0x21, 0x6a, 0xc2, 0x81, 0x33, 0x5b, 0x28, 0x70,
    0x33, 0x38, 0x5f, 0x2e, 0xc6, 0xd2, 0xee, 0x4e, 0x6c, 0x6f,
    0x52, 0x3d, 0x99, 0xf5, 0xfb, 0xd1, 0x5e, 0xaf, 0xa0, 0x3c,
    0xf8, 0x6f, 0xa9, 0x8e, 0x37, 0xce, 0xa2, 0x21, 0x3e, 0xc9,
    0xe4, 0x6e, 0xe9, 0x06, 0x29, 0x14, 0xa2, 0x33, 0x54, 0x34,
    0x45, 0x32, 0xe1, 0xca, 0x88, 0x2f, 0xa8, 0x9d, 0x2b, 0x72,
    0x28, 0x8b, 0x2e, 0x01, 0x9b, 0x2c, 0x34, 0xb5, 0x5c, 0x37,
    0x04, 0x51, 0x93, 0x58, 0x13, 0xae, 0xa9, 0x58, 0xb1, 0x56,
    0xa4, 0x97, 0x04, 0x51, 0x85, 0x67, 0x90, 0xce, 0x66, 0x7d,
    0x9e, 0x77, 0xed, 0xe6, 0xea, 0x29, 0x55, 0x95, 0x93, 0xac,
    0x76, 0xd0, 0x50, 0x1f, 0xdf, 0x8f, 0x26, 0x17, 0x7f, 0x5c,
    0x41, 0xe9, 0x52, 0xda, 0xef, 0x17, 0xff, 0xce, 0x66, 0x8b,
    0xf9, 0xbe, 0x0b, 0x2c, 0xaa, 0x48, 0x5c, 0x80, 0xa8, 0x51,
    0x4a, 0x38, 0x07, 0x3a, 0x81, 0x01, 0x10, 0x3a, 0xdd, 0x55,
    0x4c, 0xa0, 0x22, 0x7a, 0x0c, 0x05, 0x8c, 0x02, 0x58, 0x61,
    0x05, 0x62, 0x51, 0x45, 0x79, 0xf5, 0xe6, 0x54, 0x46, 0x6f,
    0xca, 0x1d, 0x21, 0xba, 0xee, 0x4d, 0x0e, 0x41, 0x3b, 0x7c,
    0x8d, 0x37, 0x14, 0x6a, 0xcb, 0x58, 0xb0, 0x28, 0x9a, 0x09,
    0x05, 0x7e, 0xe4, 0x4b, 0x65, 0x0d, 0x0a, 0x2d, 0x1c, 0xce,
    0x6c, 0xbe, 0x58, 0xcc, 0x72, 0xe2, 0x71, 0xdc, 0xe2, 0x33,
    0x2b, 0xa9, 0x92, 0x33, 0xd1, 0xd2, 0x2e, 0xde, 0x29, 0xf0,
    0x26, 0x55, 0xc0, 0xf7, 0x59, 0x6f, 0x47, 0xc5, 0x3f, 0x93,
    0x6a, 0xbf, 0xd9, 0xb9, 0xec, 0xb4, 0x64, 0x51, 0x55, 0x4a,
    0xe6, 0x32, 0x41, 0x99, 0x21, 0x9a, 0x10, 0xaa, 0x90, 0xd9,
    0x18, 0x41, 0xeb, 0x22, 0x7b, 0xed, 0x2b, 0x12, 0x0e, 0xb9,
    0xd9, 0x7a, 0xa7, 0x2f, 0x55, 0x0e, 0xd0, 0x1d, 0x80, 0xe7,
    0xa6, 0x0c, 0x2a, 0x78, 0xbe, 0x98, 0x68, 0xad, 0xb6, 0x88,
    0xd2, 0xe4, 0xfa, 0x52, 0x3e, 0x8b, 0x96, 0x62, 0xd0, 0xb3,
    0x68, 0x15, 0xa2, 0x3a, 0x53, 0x2d, 0xf7, 0xd4, 0x33, 0xb0,
    0xe5, 0xae, 0xa2, 0x0e, 0xd1, 0x69, 0x07, 0xa2, 0x11, 0x78,
    0x86, 0xd4, 0x16, 0x1d, 0xf5, 0x2c, 0x94, 0xd3, 0x20, 0xfa,
    0x61, 0x72, 0xcc, 0xa2, 0x4a, 0x7c, 0xa3, 0xb7, 0x66, 0xc4,
    0x10, 0x15, 0x11, 0x3a, 0x26, 0x2e, 0x1b, 0xe3, 0xfd, 0x7a,
    0x5f, 0x51, 0xe2, 0xd0, 0xf2, 0x1b, 0xbd, 0x04, 0x51, 0x27,
    0x64, 0x16, 0x07, 0xe7, 0x06, 0xd1, 0xdf, 0xca, 0xa2, 0x4a,
    0x5e, 0xd4, 0x59, 0xef, 0xd3, 0x80, 0x2b, 0x73, 0xe3, 0x3c,
    0x99, 0xdd, 0x0c, 0x60, 0x0d, 0x48, 0x4d, 0x56, 0x70, 0x1c,
    0xc6, 0x17, 0xf5, 0x87, 0xba, 0xb6, 0x1e, 0x95, 0x21, 0xca,
    0x08, 0x33, 0x42, 0xd4, 0xd3, 0x7a, 0xba, 0x06, 0xd1, 0x5f,
    0xca, 0xa2, 0x4a, 0x4d, 0x46, 0x64, 0xd2, 0xcc, 0xa2, 0xae,
    0x4e, 0xa2, 0x2b, 0xdb, 0x7a, 0x4d, 0x68, 0xb4, 0xa2, 0xa8,
    0xaf, 0x8b, 0xaa, 0x63, 0x88, 0x4a, 0x2c, 0xea, 0x0c, 0x6b,
    0xa2, 0xb1, 0xe8, 0x47, 0x42, 0x94, 0xb2, 0x68, 0xcd, 0xcc,
    0x63, 0x3e, 0xd2, 0xb0, 0xf2, 0xe8, 0x13, 0x44, 0x47, 0x01,
    0x28, 0x18, 0x11, 0x0e, 0xb3, 0x6c, 0xa9, 0x58, 0x03, 0xf9,
    0x0a, 0x0e, 0xdd, 0x83, 0x0a, 0xab, 0x10, 0xf5, 0x40, 0xc4,
    0xd2, 0x8d, 0x9e, 0xb3, 0x7a, 0x63, 0xd1, 0x5f, 0xc0, 0xa2,
    0xeb, 0x29, 0xf7, 0xb0, 0xa5, 0x9d, 0x00, 0x4f, 0xaf, 0x60,
    0xc8, 0xa7, 0x97, 0xfc, 0x44, 0x83, 0xda, 0x8f, 0x69, 0xe9,
    0xc7, 0xab, 0x68, 0x34, 0x02, 0x8a, 0x32, 0xc6, 0xa6, 0x87,
    0x6e, 0x04, 0x06, 0x04, 0xc5, 0xe3, 0x7d, 0xdc, 0x4b, 0x38,
    0x34, 0xe6, 0x91, 0xdd, 0xc3, 0x33, 0x00, 0x14, 0x01, 0x18,
    0x8c, 0xb5, 0x18, 0x52, 0xf4, 0x94, 0x52, 0xa4, 0x6a, 0xb6,
    0x26, 0xe2, 0xa5, 0x81, 0x1b, 0x83, 0x7d, 0xd6, 0x72, 0x3e,
    0x0c, 0x3f, 0x49, 0xf7, 0x58, 0x81, 0xac, 0xb1, 0x0b, 0x8e,
    0x21, 0xc4, 0x22, 0xf1, 0x7d, 0x51, 0x42, 0x9c, 0x74, 0xea,
    0x57, 0x89, 0xe6, 0x85, 0x13, 0xf6, 0x8a, 0x4f, 0x32, 0x78,
    0xb2, 0xf8, 0x77, 0x36, 0xeb, 0xc5, 0xea, 0x4a, 0x16, 0x65,
    0x73, 0x5e, 0x04, 0xc5, 0x88, 0xbb, 0xc1, 0x8d, 0x1e, 0x80,
    0xb7, 0x62, 0xd4, 0x13, 0x18, 0x53, 0xc4, 0x3d, 0x74, 0xb2,
    0x22, 0x3f, 0x8b, 0x6e, 0x19, 0x52, 0xb8, 0x7e, 0xab, 0x00,
    0xab, 0xc2, 0x16, 0x21, 0xba, 0xea, 0xe2, 0xa2, 0x13, 0x29,
    0x3c, 0x8e, 0x85, 0x03, 0x68, 0xde, 0x42, 0x85, 0xb9, 0x2b,
    0x61, 0x47, 0xda, 0x3c, 0x36, 0x42, 0xc7, 0x82, 0xe5, 0xe9,
    0x61, 0x79, 0x68, 0xb7, 0x8c, 0xe7, 0xa2, 0x7d, 0xc7, 0xe7,
    0x5b, 0xe9, 0xde, 0xd4, 0x6c, 0xfc, 0xc1, 0x9f, 0x45, 0xb7,
    0xe9, 0xf7, 0xf9, 0xcc, 0x86, 0x4f, 0x1c, 0x4d, 0x43, 0x89,
    0xf0, 0x9c, 0xef, 0xd3, 0xa2, 0x93, 0x4f, 0xac, 0xb9, 0xbe,
    0x44, 0x6d, 0x36, 0xe2, 0xbd, 0xd9, 0x4d, 0x87, 0x0a, 0x1b,
    0x61, 0x90, 0xd6, 0x59, 0x7d, 0x3e, 0xb0, 0x3e, 0x57, 0xe7,
    0x20, 0x6f, 0x58, 0x5c, 0xed, 0x34, 0x14, 0xab, 0x7c, 0x9f,
    0x1e, 0x1f, 0x08, 0xa4, 0x26, 0x3c, 0x8a, 0x9a, 0x42, 0x3b,
    0xb9, 0x59, 0x55, 0x34, 0x5e, 0xda, 0xe0, 0x19, 0x52, 0x85,
    0x92, 0x84, 0x57, 0x2c, 0xa2, 0xaa, 0x79, 0xa8, 0x32, 0xdc,
    0xd9, 0x68, 0xd8, 0xee, 0xa4, 0x8f, 0x92, 0x2b, 0xc0, 0xf5,
    0x1d, 0x16, 0xff, 0x6a, 0xb3, 0xd2, 0xd4, 0xd1, 0x70, 0x71,
    0x97, 0xb3, 0x28, 0x3b, 0x13, 0x54, 0x60, 0xc8, 0x00, 0x7e,
    0x22, 0x44, 0x7b, 0x84, 0xdb, 0x69, 0x33, 0x91, 0x7f, 0xf7,
    0xc4, 0xec, 0x28, 0x00, 0x87, 0x41, 0x60, 0x1d, 0xcb, 0x73,
    0xd5, 0x63, 0xc4, 0xe5, 0xbc, 0x69, 0x39, 0x21, 0x6b, 0x6d,
    0xfc, 0x5d, 0x01, 0x5a, 0xbd, 0x05, 0x3b, 0xd2, 0xa5, 0x92,
    0xda, 0x15, 0x31, 0xcc, 0xa7, 0xba, 0x98, 0x1d, 0x55, 0xce,
    0x94, 0x92, 0x6c, 0x4b, 0x00, 0x4a, 0x57, 0x81, 0x90, 0x6d,
    0x37, 0x9a, 0x70, 0xb1, 0x1d, 0xfb, 0x88, 0xa5, 0x3c, 0x59,
    0xfc, 0x6b, 0xcd, 0x2a, 0x5f, 0xe3, 0x09, 0x36, 0x05, 0xec,
    0x6f, 0x80, 0xfa, 0x33, 0x32, 0x1a, 0x78, 0x5b, 0x49, 0x2c,
    0x8a, 0x41, 0x65, 0xc7, 0xbc, 0x9f, 0x51, 0x32, 0x92, 0x3f,
    0x28, 0x12, 0x14, 0x3d, 0x83, 0xe8, 0xa4, 0x01, 0xbc, 0x5c,
    0x95, 0x21, 0x8a, 0xb0, 0xd7, 0x59, 0x33, 0x86, 0xea, 0x1c,
    0xc5, 0x6c, 0xbc, 0xaa, 0xec, 0xad, 0x08, 0xd2, 0xe4, 0x13,
    0xa4, 0xce, 0xa2, 0x59, 0xaa, 0xab, 0x41, 0x9e, 0xb0, 0x28,
    0xf0, 0x99, 0x1b, 0x7b, 0x78, 0x89, 0x41, 0x18, 0x09, 0x23,
    0x55, 0x85, 0x2f, 0x70, 0x98, 0x7e, 0x3f, 0xa4, 0xf7, 0xfb,
    0x10, 0xf5, 0xd2, 0xba, 0xad, 0xd3, 0x22, 0x95, 0xdb, 0xa9,
    0x9d, 0xf3, 0x4f, 0x83, 0xe8, 0x72, 0xcc, 0xa2, 0xba, 0xb6,
    0xe2, 0x93, 0x21, 0xba, 0xd9, 0xf5, 0x3d, 0x60, 0x54, 0xe3,
    0x7b, 0x7e, 0x64, 0x45, 0x47, 0x1f, 0x3d, 0xb9, 0x62, 0x2a,
    0x71, 0x98, 0x30, 0x7a, 0x04, 0x51, 0xaf, 0x0b, 0x2c, 0x9a,
    0x49, 0x7a, 0xda, 0x70, 0xa6, 0x9d, 0xf1, 0x3f, 0xc3, 0xa2,
    0x36, 0x60, 0x80, 0xb2, 0xa8, 0x07, 0x8c, 0x8a, 0xb7, 0x5a,
    0x86, 0x6e, 0xa6, 0xb0, 0x25, 0x0e, 0x01, 0xa3, 0x63, 0x0d,
    0xa2, 0x29, 0xef, 0x7a, 0x75, 0xe0, 0xbf, 0x18, 0xa2, 0xd7,
    0x81, 0x02, 0xa2, 0xeb, 0xb0, 0xef, 0xdb, 0x19, 0xff, 0x74,
    0x16, 0x95, 0x4e, 0x61, 0xef, 0xe5, 0xd1, 0x88, 0x8d, 0xed,
    0x3b, 0x19, 0xed, 0x45, 0x97, 0x68, 0x24, 0xa8, 0x82, 0xa6,
    0xef, 0x45, 0xfc, 0xf4, 0x0f, 0x45, 0xe9, 0xd4, 0x7b, 0x92,
    0xd7, 0x07, 0x9b, 0x14, 0x48, 0x46, 0x62, 0xc3, 0xe7, 0x27,
    0x0a, 0x7f, 0xa3, 0x8f, 0x10, 0xea, 0x85, 0x8d, 0xed, 0x14,
    0x56, 0xdc, 0xb3, 0x74, 0x49, 0x9e, 0xa5, 0x42, 0x74, 0xad,
    0x55, 0x71, 0x30, 0x5a, 0xef, 0x60, 0x57, 0xc1, 0x5b, 0x94,
    0xac, 0x4b, 0xed, 0x5e, 0xd3, 0x72, 0x80, 0x9a, 0x07, 0x9f,
    0xa5, 0xe3, 0xea, 0x05, 0x8f, 0xe3, 0x86, 0x4f, 0x55, 0xf0,
    0x62, 0xba, 0x37, 0x35, 0xcb, 0x5d, 0xf8, 0xb3, 0x68, 0xa0,
    0xac, 0x00, 0xa4, 0xf8, 0x05, 0xc0, 0xea, 0x81, 0xd1, 0x7a,
    0x34, 0xee, 0xc1, 0x10, 0x9b, 0x64, 0x53, 0x50, 0x79, 0x7a,
    0x48, 0xdd, 0x7a, 0xec, 0xe9, 0x79, 0x78, 0x48, 0xeb, 0x69,
    0x9c, 0x22, 0x37, 0xfb, 0x06, 0xd5, 0xa2, 0xa2, 0x41, 0x23,
    0x54, 0x81, 0x83, 0x7a, 0xa2, 0xf5, 0x3d, 0x6d, 0xcd, 0xe7,
    0xc0, 0x24, 0x7b, 0x4e, 0x83, 0x2d, 0x89, 0x0f, 0x9f, 0x87,
    0x5c, 0x10, 0xcb, 0xeb, 0x9f, 0x8d, 0x26, 0xd4, 0xf2, 0x5a,
    0x05, 0xcf, 0xa4, 0x7b, 0x77, 0xb3, 0x6c, 0xaa, 0x8b, 0x67,
    0x51, 0x00, 0x44, 0x93, 0x26, 0x17, 0x11, 0xb6, 0x2e, 0xda,
    0x26, 0xa4, 0xc9, 0xd5, 0xa4, 0x60, 0xd1, 0x26, 0x4d, 0x2e,
    0x06, 0x51, 0xfa, 0x4f, 0xe0, 0x7e, 0xb5, 0x19, 0x69, 0x72,
    0x6d, 0x16, 0x9d, 0xda, 0x8c, 0x34, 0xb9, 0x96, 0x4c, 0x8c,
    0x45, 0xed, 0x57, 0xe3, 0xd1, 0x26, 0x17, 0x92, 0xaf, 0x2f,
    0x3b, 0xd0, 0xff, 0x31, 0xa4, 0xb3, 0xd3, 0x57, 0x93, 0x26,
    0x97, 0x91, 0xc9, 0x76, 0xf4, 0x8d, 0x7e, 0x18, 0xba, 0x26,
    0x4d, 0x2e, 0x25, 0x03, 0x65, 0xd1, 0x15, 0xa3, 0x0d, 0xa4,
    0x4d, 0x2e, 0x05, 0x50, 0xf6, 0x5f, 0x83, 0x3d, 0x40, 0xba,
    0xe1, 0xf4, 0xa1, 0xc9, 0x9f, 0x25, 0x1c, 0x2f, 0x69, 0x7c,
    0x49, 0x7a, 0xf4, 0x73, 0xc1, 0x7b, 0x5c, 0x03, 0x9f, 0x34,
    0xc2, 0x83, 0x0f, 0x52, 0xf0, 0xec, 0x20, 0x8e, 0x0c, 0xb2,
    0x0d, 0x3e, 0x58, 0xb0, 0xeb, 0x52, 0xf5, 0x5c, 0x68, 0x61,
    0xa1, 0x8a, 0x9d, 0x6e, 0x68, 0xc1, 0x03, 0xe9, 0x45, 0x98,
    0x9e, 0x45, 0xc8, 0xb9, 0x94, 0xbd, 0x2c, 0xbc, 0xf6, 0x85,
    0x54, 0xf4, 0x4c, 0xb4, 0xd7, 0x2a, 0x58, 0x4e, 0xba, 0x2e,
    0xc5, 0xdc, 0xbd, 0xad, 0xd9, 0x81, 0xfa, 0x2c, 0x09, 0xa0,
    0xcb, 0xf2, 0x4f, 0x80, 0x01, 0x00, 0x3d, 0x86, 0x58, 0xcf,
    0x39, 0xc2, 0xf5, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
    0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
  };

  if (type == WebServer::POST)
  {
    // ignore POST data
    server.httpFail();
    return;
  }

  /* for a GET or HEAD, send the standard "it's all OK headers" but identify our data as a PNG file */
  server.httpSuccess("image/png");

  /* we don't output the body for a HEAD request */
  if (type == WebServer::GET)
  {
    server.writeP(logoData, sizeof(logoData));
  }
}

// commands are functions that get called by the webserver framework
// they can read any posted data from client, and they output to server
void webDefaultView(WebServer &server, WebServer::ConnectionType type)
{
  P(htmlHead) =
    "<html>"
    "<head>"
    "<title>SKAARHOJ Device Configuration</title>"
    "<style type=\"text/css\">"
    "BODY { font-family: sans-serif }"
    "H1 { font-size: 14pt; }"
    "P  { font-size: 10pt; }"
    "</style>"
    "</head>"
    "<body>"
    "<img src='logo.png'><br/>";

  int i;
  server.httpSuccess();
  server.printP(htmlHead);

  server << "<div style='width:660px; margin-left:10px;'><form action='" PREFIX "/form' method='post'>";

  // Panel IP:
  server << "<h1>SKAARHOJ Device IP Address:</h1><p>";
  for (i = 0; i <= 3; ++i)
  {
    server << "<input type='text' name='IP" << i << "' value='" << EEPROM.read(i + 2) << "' id='IP" << i << "' size='4'>";
    if (i < 3)  server << '.';
  }
  server << "<hr/>";

  // ATEM Switcher Panel IP:
  server << "<h1>ATEM Switcher IP Address:</h1><p>";
  for (i = 0; i <= 3; ++i)
  {
    server << "<input type='text' name='ATEM_IP" << i << "' value='" << EEPROM.read(i + 2 + 4) << "' id='ATEM_IP" << i << "' size='4'>";
    if (i < 3)  server << '.';
  }
  server << "<hr/>";

  server << "<h1>Input Assignment</h1>";
  for (i = 0; i < 8; i++)
  {
    server << "Button " << i + 1 << ":";
    server << "<input type='text' name='INPUT" << i << "' value='" << EEPROM.read(i + 20) << "' id='INPUT" << i << "' size='6'>";
    server << "<br/>";
  }

  server << "For special inputs, please refer to <a href='https://github.com/sestg/atem-c201/blob/master/inputs.md#atem-inputs' target='_blank'>this</a> table.";

  // End form and page:
  server << "</br><input type='submit' value='Submit'/></form></div>";
  server << "<br><i>(Reset / Pull the power after submitting the form successfully)</i>";
  server << "</body></html>";
}

void formCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  if (type == WebServer::POST)
  {
    bool repeat;
    char name[16], value[16];
    do
    {
      repeat = server.readPOSTparam(name, 16, value, 16);
      String Name = String(name);

      // C201 Panel IP:
      if (Name.startsWith("IP"))  {
        int addr = strtoul(name + 2, NULL, 10);
        int val = strtoul(value, NULL, 10);
        if (addr >= 0 && addr <= 3)  {
          EEPROM.write(addr + 2, val); // IP address stored in bytes 0-3
        }
      }

      // ATEM Switcher Panel IP:
      if (Name.startsWith("ATEM_IP"))  {
        int addr = strtoul(name + 7, NULL, 10);
        int val = strtoul(value, NULL, 10);
        if (addr >= 0 && addr <= 3)  {
          EEPROM.write(addr + 2 + 4, val); // IP address stored in bytes 4-7
        }
      }

      // Input assignment:
      if (Name.startsWith("INPUT"))  {
        int input = strtoul(name + 5, NULL, 10);
        int val = strtoul(value, NULL, 10);
        if (input >= 0 && input <= 7)  {
          EEPROM.write(input + 20, val); // IP address stored in bytes 4-7
        }
      }

    }
    while (repeat);

    server.httpSeeOther(PREFIX "/form");
  }
  else
    webDefaultView(server, type);
}
void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  webDefaultView(server, type);
}





/*************************************************************


                       MAIN PROGRAM CODE AHEAD


 **********************************************************/


bool isConfigMode;  // If set, the system will run the Web Configurator, not the normal program
bool shiftState = false;

void setup() {
  Serial.begin(115200);
  Serial << F("\n- - - - - - - -\nSerial Started\n");


  // *********************************
  // Start up BI8 boards and I2C bus:
  // *********************************
  // Always initialize Wire before setting up the SkaarhojBI8 class!
 // Wire.begin();

  // Set up the SkaarhojBI8 boards:
  //previewSelect.begin(0, false);
  //programSelect.begin(2, false);
  //cmdSelect.begin(7, false);
  //extraButtons.begin(1, false);



  // *********************************
  // Mode of Operation (Normal / Configuration)
  // *********************************
  // Determine web config mode
  // This is done by:
  // -  either flipping a switch connecting A1 to GND
  // -  Holding the CUT button during start up.
  pinMode(A1, INPUT_PULLUP);
  delay(100);
  isConfigMode = cmdSelect.buttonIsPressed(1) || (analogRead(A1) > 100) ? true : false;




  // *********************************
  // INITIALIZE EEPROM memory:
  // *********************************
  // Check if EEPROM has ever been initialized, if not, install default IP
  if (EEPROM.read(0) != 12 ||  EEPROM.read(1) != 232)  {  // Just randomly selected values which should be unlikely to be in EEPROM by default.
    // Set these random values so this initialization is only run once!
    EEPROM.write(0, 15);
    EEPROM.write(1, 232);

    // Set default IP address for Arduino/GVG200 panel (88.200.86.111)
    EEPROM.write(2, 88);
    EEPROM.write(3, 200);
    EEPROM.write(4, 86);
    EEPROM.write(5, 111); // Just some value I chose, probably below DHCP range?

    // Set default IP address for ATEM Switcher (88.200.86.111):
    EEPROM.write(6, 88);
    EEPROM.write(7, 200);
    EEPROM.write(8, 86);
    EEPROM.write(9, 110);

    //Set default user button mode
    EEPROM.write(17, 0);

    //Set default button assignments
    for (int i = 0; i < 8; i++) {
      EEPROM.write(i + 20, i + 1);
    }
  }


  // *********************************
  // Setting up IP addresses, starting Ethernet
  // *********************************
  if (isConfigMode)  {
    // Setting the default ip address for configuration mode:
    ip[0] = default_ip[0];
    ip[1] = default_ip[1];
    ip[2] = default_ip[2];
    ip[3] = default_ip[3];
  } else {
    ip[0] = EEPROM.read(0 + 2);
    ip[1] = EEPROM.read(1 + 2);
    ip[2] = EEPROM.read(2 + 2);
    ip[3] = EEPROM.read(3 + 2);
  }

  // Setting the ATEM IP address:
  atem_ip[0] = EEPROM.read(0 + 2 + 4);
  atem_ip[1] = EEPROM.read(1 + 2 + 4);
  atem_ip[2] = EEPROM.read(2 + 2 + 4);
  atem_ip[3] = EEPROM.read(3 + 2 + 4);

  Serial << F("SKAARHOJ Device IP Address: ") << ip[0] << "." << ip[1] << "." << ip[2] << "." << ip[3] << "\n";
  Serial << F("ATEM Switcher IP Address: ") << atem_ip[0] << "." << atem_ip[1] << "." << atem_ip[2] << "." << atem_ip[3] << "\n";

  // Setting MAC address:
  mac[0] = EEPROM.read(10);
  mac[1] = EEPROM.read(11);
  mac[2] = EEPROM.read(12);
  mac[3] = EEPROM.read(13);
  mac[4] = EEPROM.read(14);
  mac[5] = EEPROM.read(15);
  char buffer[18];
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial << F("SKAARHOJ Device MAC address: ") << buffer << F(" - Checksum: ")
         << ((mac[0] + mac[1] + mac[2] + mac[3] + mac[4] + mac[5]) & 0xFF);
  if ((uint8_t)EEPROM.read(16) != ((mac[0] + mac[1] + mac[2] + mac[3] + mac[4] + mac[5]) & 0xFF))  {
    Serial << F("MAC address not found in EEPROM memory!\n") <<
           F("Please load example sketch ConfigEthernetAddresses to set it.\n") <<
           F("The MAC address is found on the backside of your Ethernet Shield/Board\n (STOP)");
    while (true);
  }

  //for (int i = 0; i < 8; i++) {
  //  inputs[i] = EEPROM.read(20 + i);
  //}

  Ethernet.begin(mac, ip);
  
  Serial.println(F("."));
  Serial.println(F("Starting in 4 seconds.."));
  delay(1000);
  Serial.println(F("Starting in 3 seconds.."));
  delay(1000);
  Serial.println(F("Starting in 2 seconds.."));
  delay(1000);
  Serial.println(F("Starting in 1 seconds.."));
  delay(1000);


  // *********************************
  // Final Setup based on mode
  // *********************************
  if (isConfigMode)  {

    // Red by default:
//    previewSelect.setDefaultColor(2);
//    programSelect.setDefaultColor(2);
//    cmdSelect.setDefaultColor(2);
//    extraButtons.setDefaultColor(2);
//
//    previewSelect.setButtonColorsToDefault();
//    programSelect.setButtonColorsToDefault();
//    cmdSelect.setButtonColorsToDefault();
//    extraButtons.setButtonColorsToDefault();

    webserver.begin();
    webserver.setDefaultCommand(&defaultCmd);
    webserver.addCommand("form", &formCmd);
    webserver.addCommand("logo.png", &logoCmd);
  } else {

    // LCD IP info:


    //delay(1000);

    // Colors of buttons:
    //previewSelect.setDefaultColor(0);  // Off by default
    //programSelect.setDefaultColor(0);  // Off by default
    //cmdSelect.setDefaultColor(0);  // Off by default
    //extraButtons.setDefaultColor(0);  // Off by default

    //previewSelect.testSequence(10);
    //programSelect.testSequence(10);
    //cmdSelect.testSequence(10);
    //extraButtons.testSequence(10);

    // Initializing the slider:
    //utils.uniDirectionalSlider_init();
    //utils.uniDirectionalSlider_hasMoved();









    AtemSwitcher.begin(IPAddress(atem_ip[0], atem_ip[1], atem_ip[2], atem_ip[3]));
    // AtemSwitcher.serialOutput(true);
    AtemSwitcher.connect();
  }
  //Tukaj so pinmode nastavitve za PORTA, PORTC, PORTG in PORTL ter animacija
  setupGVGBoard();
}




/*************************************************************
   Loop function (runtime loop)
 *************************************************************/


// AtemOnline is true, if there is a connection to the ATEM switcher
bool AtemOnline = false;
long cas = 0;
long temp = 0;
uint8_t cikel = 0;
// The loop function:
void loop() {
  cikel++;
  if (isConfigMode)  {
    webserver.processConnection();
  } else {

    // Check for packets, respond to them etc. Keeping the connection alive!
    AtemSwitcher.runLoop();

    // If connection is gone, try to reconnect:
    //if (AtemSwitcher.hasInitialized())  {


    //    AtemSwitcher.connect();
    // }

    // If the switcher has been initialized, check for button presses as reflect status of switcher in button lights:
    if (AtemSwitcher.hasInitialized())  {
      if (!AtemOnline)  {
        AtemOnline = true;
        timeoutMillis = millis() + 5000;
        timeoutDone = false;
        //Serial << "Got 'em at " << millis() << ", timeout set to " << timeoutMillis << " " << timeoutDone;



//        previewSelect.setDefaultColor(0);  // Dimmed by default
//        programSelect.setDefaultColor(0);  // Dimmed by default
//        cmdSelect.setDefaultColor(0);  // Dimmed by default
//        extraButtons.setDefaultColor(0);  // Dimmed by default
//
//        previewSelect.setButtonColorsToDefault();
//        programSelect.setButtonColorsToDefault();
//        cmdSelect.setButtonColorsToDefault();
//        extraButtons.setButtonColorsToDefault();
      }


      setButtonColors();
      AtemSwitcher.runLoop();  // Call here and there...

      readingButtonsAndSendingCommands();
      AtemSwitcher.runLoop();  // Call here and there...

//      extraButtonsCommands2();
//      AtemSwitcher.runLoop();  // Call here and there...

    } else {
      if (AtemOnline)  {
        AtemOnline = false;
        //Serial << "Lost it at " << millis();
        timeoutDone = true;



//        previewSelect.setDefaultColor(0);  // Off by default
//        programSelect.setDefaultColor(0);  // Off by default
//        cmdSelect.setDefaultColor(0);  // Off by default
//        extraButtons.setDefaultColor(0);  // Off by default
//
//        previewSelect.setButtonColorsToDefault();
//        programSelect.setButtonColorsToDefault();
//        cmdSelect.setButtonColorsToDefault();
//        extraButtons.setButtonColorsToDefault();
      }
    }
    if (timeoutMillis < millis() && timeoutDone == false ) {
      Serial << millis() << "\n" << timeoutDone << "\n" << timeoutMillis << "\n" ;
      timeoutDone = true;
    }
  }
//  temp = millis();
//  if (cikel%10==0){
//    cas = temp - cas;
//    Serial.println(cas);
//    cas = temp;
//  }
}



/**********************************
   Button State Colors set:
 ***********************************/
void setButtonColors()  {

//  setPreviewTallyLights(AtemSwitcher.getPreviewInputVideoSource(0));
//
//  setProgramTallyLights(AtemSwitcher.getProgramInputVideoSource(0));
//
//  setAuxTallyLights(AtemSwitcher.getAuxSourceInput(0));

//  setDisplayEFF();

//    for(int i=0;i<16;i++){
//      Serial.println(i);
//      for(int j=0;j<8;j++){
//        if(i==4){
//          PORTC = 0b10000000 + i; delay(0.1);
//          PORTL = 0b1 << j; 
//          PORTG = 0b1; delay(0.1); 
//          PORTG = 0b0; 
//          PORTC = 0b10010000;  delay(0.1);
//          PORTL = 0b11; delay(0.1);
//          PORTG = 0b1; delay(2000.1);
//          PORTC = 0b10001011;  delay(0.1);
//          PORTG = 0b0;
//          //ugasni
//          PORTC = 0b10000000 + i; delay(0.1);
//          PORTL = 0b0;
//          PORTG = 0b1; delay(0.1); 
//          PORTG = 0b0; delay(0.1); 
//        } else {
//          PORTC = 0b10000000 + i; delay(0.1);
//          PORTL = 0b1 << j;
//          PORTG = 0b1; delay(2000.1); 
//          PORTG = 0b0;
//          //ugasni
//          PORTL = 0b0;
//          PORTG = 0b1; delay(0.1); 
//          PORTG = 0b0; delay(0.1); 
//        }
//      }
//    }

  // PREGLEJ STANJE TALLY VSEH VHODOV....
//  Serial.println("#########  TALLY-JI ##############");
//  for (uint8_t i = 0; i < 21; i++)  {
//    
//    Serial.print("Tally od inputa ");
//    Serial.print(inputs[i]);
//    Serial.print(" je v BIN ");
//    Serial.println(AtemSwitcher.getTallyBySourceTallyFlags(i),BIN);
//    
//  }

  

//  Serial.print("PreviewTally: ");
//  Serial.println(AtemSwitcher.getPreviewInputVideoSource(0));
  // Setting colors of PREVIEW input select buttons:
//  for (uint8_t i = 0; i < 21; i++)  {
//    if (getAtemPreviewTally(inputs[i]))  {
//      //if (inputs[i] != AtemPreviewTally_Old)  {
//        //setPreviewTallyLights(inputs[i]);
//        Serial.print("PreviewTally: ");
//        Serial.println(inputs[i]);
//        Serial.print("TallyFlags: ");
//        Serial.println(AtemSwitcher.getTallyBySourceTallyFlags(inputs[i]));
//        ;
//      //}
//    }
//  }
    

  
//  AuxTally = AtemSwitcher.getAuxSourceInput(0);
//  if (AuxTally != AuxTally_Old) {
////    Serial.print("AUXtally: ");
////    Serial.println(AuxTally);
//    for (uint8_t i = 0; i < 3; i++){
//      for (uint8_t j = 0; j < 8; j++)  {
//        if (AuxTally == auxInput[i][j]){
////          Serial.print("AUXtally: ");
////          Serial.println(auxInput[i][j]);
////          Serial.println(i);
////          Serial.println(j);
//          setAuxLights(0b01100000,i,j);
//          AuxTally_Old = AuxTally;
//        }
//      }
//    }
//  }
  
  

  // Setting colors of PROGRAM input select buttons:
//  for (uint8_t i = 0; i < 21; i++)  {
//    if ((getAtemProgramTally(inputs[i])) && !getTransitionInTransition(0)){
//      if (inputs[i] != AtemProgramTally_Old)  {
//        setProgramTallyLights(inputs[i]);
//      }
//    }
//  }

  
//  for (uint8_t i = 1; i <= 8; i++)  {
//    if (BUSselect == 0)  { // Normal: PROGRAM
//      if (getAtemProgramTally(inputs[i - 1]))  {
//        programSelect.setButtonColor(i, 2);
//      } else {
//        programSelect.setButtonColor(i, 0);
//      }
//    } else if (BUSselect <= 3)  { // 1-3: AUX bus:
//      if (AtemSwitcher.getAuxSourceInput(BUSselect - 1) == inputs[i - 1])  {
//        programSelect.setButtonColor(i, 4);
//      } else {
//        programSelect.setButtonColor(i, 0);
//      }
//    } else if (BUSselect == 10 || BUSselect == 11)  {
//      if (AtemSwitcher.getMediaPlayerSourceStillIndex(BUSselect - 10) == (shiftState ? i + 7 : i - 1))  {
//        programSelect.setButtonColor(i, 4);
//      } else {
//        programSelect.setButtonColor(i, 0);
//      }
//    }
//  }
//
//  // The user button mode tells us, how the four user buttons should be programmed. This sets the colors to match the function:
//
//  // Setting colors of the command buttons:
//  if (!shiftState) {
//    cmdSelect.setButtonColor(5, getUpstreamKeyerOnNextTransitionStatus(0) ? 4 : 0);
//    cmdSelect.setButtonColor(6, getUpstreamKeyerOnNextTransitionStatus(1) ? 4 : 0);
//    cmdSelect.setButtonColor(3, AtemSwitcher.getDownstreamKeyerOnAir(0) ? (AtemSwitcher.getDownstreamKeyerTie(0) ? 4 : 2) : (AtemSwitcher.getDownstreamKeyerTie(0) ? 3 : 0));
//    cmdSelect.setButtonColor(8, AtemSwitcher.getDownstreamKeyerOnAir(1) ? (AtemSwitcher.getDownstreamKeyerTie(1) ? 4 : 2) : (AtemSwitcher.getDownstreamKeyerTie(1) ? 3 : 0));
//  } else {
//    cmdSelect.setButtonColor(5, AtemSwitcher.getDownstreamKeyerIsAutoTransitioning(0) ? 2 : 0);
//    cmdSelect.setButtonColor(6, AtemSwitcher.getDownstreamKeyerIsAutoTransitioning(1) ? 2 : 0);
//    cmdSelect.setButtonColor(3, AtemSwitcher.getDownstreamKeyerOnAir(0) ? 2 : 0);
//    cmdSelect.setButtonColor(8, AtemSwitcher.getDownstreamKeyerOnAir(1) ? 2 : 0);
//  }
//
//  if (!cmdSelect.buttonIsPressed(1))  {
//    cmdSelect.setButtonColor(1, 0);   // de-highlight CUT button
//  }
//
//  cmdSelect.setButtonColor(2, AtemSwitcher.getTransitionPosition(0) > 0 ? 2 : 0);   // Auto button
//  cmdSelect.setButtonColor(7, AtemSwitcher.getKeyerOnAirEnabled(0, 0) ? 2 : 0); //Sedaj je to KEY1 air. Nedelujoč ugašnjena led, prižgan pa rdeč
//  //AtemSwitcher.getDownstreamKeyerOnAir(1) ? 2 : 0);    // DSK1 button
//  if (!cmdSelect.buttonIsPressed(4))  {
//    cmdSelect.setButtonColor(4, 0);   // de-highlight SHIFT button
//    shiftState = false;
//  }
  //cmdSelect.setButtonColor(4, AtemSwitcher.getDownstreamKeyerOnAir(2) ? 2 : 0);    // DSK2 button
}





/**********************************
   ATEM Commands
 ***********************************/
void readingButtonsAndSendingCommands() {

  readPreviewKeys();
//
  readProgramKeys();
//
  readAuxKeys();
//
  readBigBoardKey();
//
  readEFFKeys();
//
//  readLeverArmKeys();

  readDSKKeys();
  
//  for(int i=0;i<16;i++){
//    Serial.println(i<<4,BIN);
//    PORTC=i<<4;
//    delay(5000);
//  }
  
  // Sending commands for PREVIEW input selection:
  //uint8_t busSelection = previewSelect.buttonDownAll();
  //for (uint8_t i = 1; i <= 8; i++)  {
  //  if (previewSelect.isButtonIn(i, busSelection))  {
  //    AtemSwitcher.setPreviewInputVideoSource(0, inputs[i - 1]);
  //  }
  //}
//  for(uint8_t i=3;i<6;i++){
//      readPreviewKeys(i);
//  }
  
//  for(uint8_t i=0;i<3;i++){
//      readProgramKeys(i);
//  }
//
//  for (uint8_t i = 0; i < 3; i++)  {
//    beriTipkeAux(i);
//  }

  // Sending commands for PROGRAM input selection:
//  busSelection = programSelect.buttonDownAll();
//  if (BUSselect == 0)  {
//    for (uint8_t i = 1; i <= 8; i++)  {
//      if (programSelect.isButtonIn(i, busSelection))  {
//        AtemSwitcher.setProgramInputVideoSource(0, inputs[i - 1]);
//      }
//    }
//  } else if (BUSselect <= 3)  {
//    for (uint8_t i = 1; i <= 8; i++)  {
//      if (programSelect.isButtonIn(i, busSelection))  {
//        AtemSwitcher.setAuxSourceInput(BUSselect - 1, inputs[i - 1]);  // When selecting AUX bus, input 8 is mapped to PROGRAM instead
//      }
//    }
//  } else if (BUSselect == 10 || BUSselect == 11)  {
//    for (uint8_t i = 1; i <= 8; i++)  {
//      if (programSelect.isButtonIn(i, busSelection))  {
//        AtemSwitcher.setMediaPlayerSourceStillIndex(BUSselect - 10, shiftState ? i + 7 : i - 1);
//      }
//    }
//  }

  // "T-bar" slider:
//    if (utils.uniDirectionalSlider_hasMoved())  {
//      AtemSwitcher.setTransitionPosition(0, utils.uniDirectionalSlider_position() * 9.999);
//      lDelay(20);
//      if (utils.uniDirectionalSlider_position() == 1000) {
//        AtemSwitcher.setTransitionPosition(0, 10000);
//        lDelay(5);
//      }
//      else if (utils.uniDirectionalSlider_isAtEnd())  {
//        AtemSwitcher.setTransitionPosition(0, 0);
//        lDelay(5);
//      }
//    }

//  // Cut button:
//  uint8_t cmdSelection = cmdSelect.buttonDownAll();
//  if (cmdSelection & (B1 << 0))  {
//    cmdSelect.setButtonColor(1, 2);    // Highlight CUT button
//    AtemSwitcher.performCutME(0);
//  }
//
//  // Auto button:
//  if (cmdSelection & (B1 << 1))  {
//    AtemSwitcher.performAutoME(0);
//  }
//
//  // TO JE CISTO DESNI ZGORNJI GUMB, MORA BIT KEY1AIR trenutno DSK1 button:
//  if (cmdSelection & (B1 << 6))  {
//    AtemSwitcher.setKeyerOnAirEnabled(0, 0, !AtemSwitcher.getKeyerOnAirEnabled(0, 0));
//    //AtemSwitcher.changeDownstreamKeyOn(1, !AtemSwitcher.getDownstreamKeyerOnAir(1));
//  }
//
//  // TUKAJ BO PA SHIFT KEY;
//  if (cmdSelection & (B1 << 3))  {
//    cmdSelect.setButtonColor(4, 4);    // Highlight CUT button
//    shiftState = true;
//  }

  /*
    if (cmdSelection & (B1 << 3))  {
    shiftState= !shiftState;
    }
    //*/

//  if (!shiftState) {
//    if (cmdSelection & (B1 << 4))  {
//      changeUpstreamKeyNextTransition(0, !getUpstreamKeyerOnNextTransitionStatus(0));
//    }; //tu naj bo BACKGROUND (pri izbirah overlaya), s shiftom je DSK1 CUT
//    if (cmdSelection & (B1 << 5))  {
//      changeUpstreamKeyNextTransition(1, !getUpstreamKeyerOnNextTransitionStatus(1));
//    }; //tu naj bo KEY1 (to je da ga da na preview), s shiftom je DSK2 CUT
//    if (cmdSelection & (B1 << 2))  {
//      AtemSwitcher.setDownstreamKeyerTie(0, !AtemSwitcher.getDownstreamKeyerTie(0));
//    }; //tu naj bo DSK1 TIE, s shiftom DSK1 AUTO
//    if (cmdSelection & (B1 << 7))  {
//      AtemSwitcher.setDownstreamKeyerTie(1, !AtemSwitcher.getDownstreamKeyerTie(1));
//    }; //tu naj bo DSK2 TIE, s shiftom DSK2 AUTO
//  } else {
//    //*
//    if (cmdSelection & (B1 << 4))  {
//      AtemSwitcher.performDownstreamKeyerAutoKeyer(0);
//    }; //tu naj bo DSK1 TIE, s shiftom DSK1 AUTO
//    if (cmdSelection & (B1 << 5))  {
//      AtemSwitcher.performDownstreamKeyerAutoKeyer(1);
//    }; //tu naj bo DSK 2 TIE, s shiftom DSK2 AUTO
//    if (cmdSelection & (B1 << 2))  {
//      AtemSwitcher.setDownstreamKeyerOnAir(0, !AtemSwitcher.getDownstreamKeyerOnAir(0));
//    }; //tu naj bo BACKGROUND (pri izbirah overlaya), s shiftom je DSK1 CUT
//    if (cmdSelection & (B1 << 7))  {
//      AtemSwitcher.setDownstreamKeyerOnAir(1, !AtemSwitcher.getDownstreamKeyerOnAir(1));
//    }; //tu naj bo KEY1 (to je da ga da na preview), s shiftom je DSK2 CUT

    //*/
//  }
} // END OF Reading buttons

//boolean getAtemPreviewTally(uint16_t source) {
//  int tally = AtemSwitcher.getTallyBySourceTallyFlags(source);
//  return (tally & 2) > 0 ? true : false;
//}
//
//boolean getAtemProgramTally(uint8_t source) {
//  int tally = AtemSwitcher.getTallyBySourceTallyFlags(source);
//  return (tally & 1) > 0 ? true : false;
//}

//boolean getAuxTally(uint8_t source) {
//  int tally = AtemSwitcher.getAuxSourceInput(0);
//  return (tally & 1) > 0 ? true : false;
//}

//boolean getUpstreamKeyerOnNextTransitionStatus(uint8_t keyer)
//{
//  uint8_t keyers = AtemSwitcher.getTransitionNextTransition(0);
//  return (keyers & (0x01 << keyer)) ? true : false;
//}

//void changeUpstreamKeyNextTransition(uint8_t keyer, boolean state) {
//  uint8_t keyers = AtemSwitcher.getTransitionNextTransition(0);
//  if (state) {
//    keyers = keyers | (0x01 << keyer);
//  } else {
//    keyers = keyers & (~(0x01 << keyer));
//  }
//  AtemSwitcher.setTransitionNextTransition(0, keyers);
//}

//Local delay

//void lDelay(unsigned long timeout)  {
//  //Serial << timeout;
//  unsigned long thisTime = millis();
//  do {
//    if (isConfigMode)  {
//      webserver.processConnection();
//    } else {
//      AtemSwitcher.runLoop();
//    }
//    Serial << F(".");
//    static int k = 1;
//    k++;
//    if (k > 100) {
//      k = 1;
//      Serial << F("\n");
//    }
//  }
//  while (!sTools.hasTimedOut(thisTime, timeout));
//}
