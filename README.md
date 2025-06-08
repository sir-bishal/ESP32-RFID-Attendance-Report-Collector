# ESP32-RFID-Attendance-Report-Collector
An ESP32-based RFID Attendance system featuring card scanning, audio feedback via DFPlayer Mini, real-time date/time display on an OLED, and data logging to Google Sheets.



#define WIFI_SSID "bishal_paul"
// Your actual WiFi network name, remove "bishal_paul" and put your Wifi Name like "xyz"


#define WIFI_PASSWORD "my-Password"
// Your actual WiFi password, remove "my-Password" and pur your Wifi Password.

const String SHEET_URL = "URL"; 
// Your actual Google Sheet URL, after your google sheet url add "?name=", 
like https://script.google.com/macros/s/xxxxxxxxxxxxxxxxxxxxx/exec?name="


In google sheet code add your time zone like "Asia/Kolkata"
your google sheed id https://docs.google.com/spreadsheets/d/XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX/edit?gid=0#gid=0
your id will be in XXXXXXXXXXX copy that and paste 
