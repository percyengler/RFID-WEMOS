#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

void wifiSetup();
void displayMessage(const char* message);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Ändere zu -1, da 0 ein gültiger Pin sein kann
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Konfiguration der RFID-RC522-Pins
#define SDA D8
#define RST D3

MFRC522 rfid(SDA, RST);

//const char* ssid = "Laborumgebung_JuzB6"; // dein Netzwerk SSID (Name)
//const char* password = "GJ73W5DRG5Q6J3QN";
const char* ssid = "NOKIA 8810"; // dein Netzwerk SSID (Name)
const char* password = "12345678";
const char* server = "http://192.168.9.143/idtime.php?rfid=";

void setup() {
    Serial.begin(9600);
    Serial.println("Hello World");

    // WiFi Verbindung herstellen
    wifiSetup();
    
    // Initialisierung von SPI und RFID
    SPI.begin();
    rfid.PCD_Init();
    
    // Initialisierung des Displays
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Die Standard I2C-Adresse des Displays ist 0x3C
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    
    display.display();
    delay(2000); // Pause für 2 Sekunden
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
}

void wifiSetup() {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    // Einfache Fortschrittsanzeige während wir auf die Verbindung warten
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) { // 20 Versuche, insgesamt ca. 10 Sekunden
        delay(500);
        Serial.print(".");
        attempts++;
    }

    // Überprüfen, ob die Verbindung hergestellt wurde
    if (WiFi.status() == WL_CONNECTED) {
        // Ein paar Sekunden warten, bis das Wi-Fi initialisiert ist
        delay(2000);

        // Debug-Informationen zur Verbindung drucken
        Serial.print("Connected! IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Failed to connect to WiFi");
        displayMessage("WiFi Verbindung fehlgeschlagen");
    }
}

void displayMessage(const char* message) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(message);
    display.display();
}

void loop() {
    if (!rfid.PICC_IsNewCardPresent()) {
        delay(50);
        return;
    }

    if (!rfid.PICC_ReadCardSerial()) {
        delay(50);
        return;
    }

    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    Serial.println("Card detected, UID: " + uid);

    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        String serverPath = String(server) + uid;

        http.begin(client, serverPath.c_str());
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0) {
            String payload = http.getString();
            Serial.println(payload);

            if (payload.indexOf("Erfolgreich") != -1) {
                displayMessage("Buchung erfolgreich");
                delay(3000);
                displayMessage("");
            }
        } else {
            Serial.println("Error on HTTP request");
            displayMessage("HTTP Anfragefehler");
            delay(3000);
            displayMessage("");
        }

        http.end();
    } else {
        Serial.println("No WiFi connection");
        displayMessage("Keine WiFi Verbindung");
        delay(3000);
        displayMessage("");
    }

    rfid.PICC_HaltA();
}
