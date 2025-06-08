#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// DFPlayer Mini Library
#include <DFRobotDFPlayerMini.h>

// RFID Reader Connections
#define RST_PIN 22 // Connects to MFRC522 RST pin
#define SS_PIN 21  // Connects to MFRC522 SDA/SS pin

// Buzzer Connection
#define BUZZER 15 // Connects to one leg of the buzzer, other leg to GND

// DFPlayer Mini Serial Connections
// Using Serial2 on ESP32.
// DFPLAYER_RX_PIN (ESP32) connects to DFPlayer Mini's TX pin.
// DFPLAYER_TX_PIN (ESP32) connects to DFPlayer Mini's RX pin (via voltage divider).
#define DFPLAYER_RX_PIN 16
#define DFPLAYER_TX_PIN 17

// Initialize MFRC522 object
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key; // Authentication key for RFID blocks
MFRC522::StatusCode status; // Variable to store status of MFRC522 operations

// Initialize DFPlayer Mini object using HardwareSerial2
HardwareSerial myDFPlayerSerial(2); // Use Serial2 on ESP32 (GPIO 16 and 17)
DFRobotDFPlayerMini myDFPlayer;

// RFID specific variables
int blockNum = 2; // The block number on the RFID card to read data from
byte bufferLen = 18; // Buffer length for reading data from RFID block
byte readBlockData[18]; // Array to store data read from RFID block

// Variables for Google Sheet Integration
String card_holder_name; // Will store the URL for the Google Sheet request
const String sheet_url = "https://script.google.com/macros/s/AKfycbzgAXZ51etHvtqm1WMhtgsb0csW7DCYLhT7dkkmUyy2Thgh-gqXoFZIsj6H9YcodiIi/exec?name="; // Your actual Google Sheet URL

// WiFi credentials
#define WIFI_SSID "bishal_paul"       // Your actual WiFi network name
#define WIFI_PASSWORD "my-Password" // Your actual WiFi password


void setup() {
  Serial.begin(9600); // Initialize serial communication for debugging
  while (!Serial); // Wait for serial port to connect (useful for some boards)
  Serial.println("\nStarting RFID and DFPlayer Mini Project...");

  // --- Initialize DFPlayer Mini ---
  // Begin Serial2 communication for DFPlayer Mini at 9600 baud.
  // SERIAL_8N1: 8 data bits, no parity, 1 stop bit.
  // DFPLAYER_RX_PIN: ESP32's RX pin for Serial2 (connected to DFPlayer's TX).
  // DFPLAYER_TX_PIN: ESP32's TX pin for Serial2 (connected to DFPlayer's RX via voltage divider).
  myDFPlayerSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
  delay(100); // Give DFPlayer module a moment to power up and stabilize

  // Attempt to initialize the DFPlayer Mini module
  if (!myDFPlayer.begin(myDFPlayerSerial)) {
    Serial.println(F("Unable to begin DFPlayer Mini:"));
    Serial.println(F("1. Check wiring (RX->TX, TX->RX with voltage divider on RX)."));
    Serial.println(F("2. Ensure SD card is inserted correctly and formatted FAT32."));
    Serial.println(F("3. Ensure audio files are in correct format (e.g., /01/001.mp3)."));
    while (true); // Halt execution if DFPlayer fails to initialize
  }
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.volume(20);  // Set volume (0-30, 30 is max)
  // Play a startup sound (file 001 from folder 01) to confirm DFPlayer is working.
  // This assumes you have folder '01' with file '001.mp3' on the SD card.
  myDFPlayer.playFolder(1, 1);
  delay(1000); // Allow sound to play

  // --- Initialize WiFi ---
  Serial.print("Connecting to WiFi AP: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500); // Wait 500ms before retrying
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // --- Initialize Buzzer ---
  pinMode(BUZZER, OUTPUT); // Set buzzer pin as an output

  // --- Initialize RFID Reader ---
  SPI.begin();       // Initialize SPI bus (used by MFRC522)
  mfrc522.PCD_Init(); // Initialize MFRC522 reader (ONLY CALL ONCE IN SETUP)
  Serial.println("RFID Reader online. Ready to scan cards.");
}


void loop() {
  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return; // No new card, exit loop early
  }

  // Select the scanned card
  if (!mfrc522.PICC_ReadCardSerial()) {
    return; // Failed to read card serial, exit loop early
  }

  Serial.println("\n--- RFID Card Scanned! ---");
  Serial.println("Reading data from RFID...");

  // Read data from the specified block on the RFID card
  ReadDataFromBlock(blockNum, readBlockData);

  // Print the data read from the block
  Serial.print("Last data in RFID block ");
  Serial.print(blockNum);
  Serial.print(" --> \"");
  for (int j = 0; j < 16; j++) {
    Serial.write(readBlockData[j]);
  }
  Serial.println("\"");

  // --- Play sound from DFPlayer Mini ---
  Serial.println("Playing audio file '001.mp3' from folder 'paul' (mapped to 01)...");
  // Assumes 'paul' folder is mapped to '01' and file '001.mp3' is inside.
  myDFPlayer.playFolder(1, 1);
  // Important: DFPlayer Mini expects folders named 01, 02, etc., and files 001.mp3, 002.mp3 etc.
  // So ensure your "paul" folder is effectively the first folder (01) on the SD card.

  // --- Buzzer feedback ---
  digitalWrite(BUZZER, HIGH); // Turn buzzer ON
  delay(200);                 // Keep it on for 200ms
  digitalWrite(BUZZER, LOW);  // Turn buzzer OFF
  delay(200);                 // Pause for 200ms
  digitalWrite(BUZZER, HIGH); // Turn buzzer ON again
  delay(200);                 // Keep it on for 200ms
  digitalWrite(BUZZER, LOW);  // Turn buzzer OFF
  delay(1000);                // Small delay to allow audio to start and prevent immediate re-scan

  // --- Crucial for multiple scans: Halt PICC ---
  // This command tells the MFRC522 to halt the current card and
  // allow it to detect new cards immediately after.
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD (Power Down Card)
  mfrc522.PCD_StopCrypto1();


  // --- Existing WiFi and HTTP Client code ---
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    // client.setInsecure() is used for development/testing without proper CA certs.
    // FOR PRODUCTION: It is HIGHLY RECOMMENDED to use client.setCACert() with valid certificates
    // to prevent man-in-the-middle attacks.
    client.setInsecure();

    // Construct the full URL with the card holder name
    card_holder_name = sheet_url + String((char*)readBlockData);
    card_holder_name.trim(); // Remove any null terminators or whitespace from the string

    Serial.print("Attempting HTTP GET to: ");
    Serial.println(card_holder_name);

    HTTPClient https;

    if (https.begin(client, card_holder_name)) { // Start connection to the URL
      Serial.print(F("[HTTPS] Performing GET request...\n"));
      int httpCode = https.GET(); // Send the HTTP GET request

      // httpCode will be negative on error, positive for success (e.g., 200)
      if (httpCode > 0) {
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // You can read the server's response here if needed:
        // String payload = https.getString();
        // Serial.println("Response payload:\n" + payload);
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end(); // Close the connection
      delay(1000); // Wait for a second before potentially new operations
    } else {
      Serial.printf("[HTTPS] Unable to connect to the provided URL.\n");
    }
  } else {
    Serial.println("WiFi not connected. Skipping HTTP request.");
  }
}


// Function to read data from a specific block on the RFID card
void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  // Populate the authentication key with default A (0xFF)
  // This is a common default key for new or unprogrammed MIFARE Classic cards.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Authenticate access to the specified block
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return; // Exit if authentication fails
  } else {
    Serial.println("Authentication success.");
  }

  // Read data from the authenticated block
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return; // Exit if reading fails
  } else {
    Serial.println("Block was read successfully.");
  }
}

//https://github.com/sir-bishal
