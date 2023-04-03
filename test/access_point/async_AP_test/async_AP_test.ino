#include "WiFi.h"
#include "ESPAsyncWebServer.h"

#define MAX_CONNECTIONS 1
 
const char* ssid = "ESP32-AccessPoint";
const char* password =  "12345678";
 
AsyncWebServer server(80);
 
void setup(){
  Serial.begin(9600);
 
  // Connect to Wi-Fi network with SSID and password
  Serial.println("Setting AP (Access Point) ...");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  if(!WiFi.softAP(ssid, password, 1, false, MAX_CONNECTIONS)){ Serial.println("Something went wrong!"); }

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
 
  server.on("/text1", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "TEXT 1");
  });

  server.on("/text2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "TEXT 2");
  });
 
  server.begin();
}
 
void loop(){
  // WiFiClient client = server.available();   // Listen for incoming clients

  // if (client) {                             // If a new client connects,
  //   Serial.println("New Client.");          // print a message out in the serial port
  //   String currentLine = "";                // make a String to hold incoming data from the client
  //   while (client.connected()) {            // loop while the client's connected
  //     if (client.available()) {             // if there's bytes to read from the client,
  //       char c = client.read();             // read a byte, then
  //       Serial.write(c);                    // print it out the serial monitor
  //       header += c;
  //       if (c == '\n') {                    // if the byte is a newline character
  //         // if the current line is blank, you got two newline characters in a row.
  //         // that's the end of the client HTTP request, so send a response:
  //         if (currentLine.length() == 0) {
  //           // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  //           // and a content-type so the client knows what's coming, then a blank line:
  //           client.println("HTTP/1.1 200 OK");
  //           client.println("Content-type:text/html");
  //           client.println("Connection: close");
  //           client.println();
            
  //           // Display the HTML web page
  //           client.println("<!DOCTYPE html><html>");
  //           client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  //           client.println("<link rel=\"icon\" href=\"data:,\">");
  //           // CSS to style the on/off buttons 
  //           // Feel free to change the background-color and font-size attributes to fit your preferences
  //           client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  //           client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
  //           client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}</style></head>");
            
  //           // Web Page Heading
  //           client.println("<body><h1>ESP32 Web Server</h1>");
  //           client.println("<p><a href=\"/text1"\><button class=\"button\">Send text-1</button></a></p>");
  //           client.println("<p><a href=\"/text2"\><button class=\"button\">Send text-2</button></a></p>");
  //           client.println("</body></html>");
            
  //           // The HTTP response ends with another blank line
  //           client.println();
  //           // Break out of the while loop
  //           break;
  //         } else { // if you got a newline, then clear currentLine
  //           currentLine = "";
  //         }
  //       } else if (c != '\r') {  // if you got anything else but a carriage return character,
  //         currentLine += c;      // add it to the end of the currentLine
  //       }
  //     }
  //   }
  //   // Clear the header variable
  //   header = "";
  //   // Close the connection
  //   client.stop();
  //   Serial.println("Client disconnected.");
  //   Serial.println("");
  // }
}