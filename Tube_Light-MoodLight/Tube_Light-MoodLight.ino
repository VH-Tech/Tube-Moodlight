/////////////////////////////////////////////////////////////////////////
/*MoodLight
 * Run different patterns on start
 * Open webpage and select a specific color
 * 
 * 
 * WiFi Manager added
 * Reset on ping fail added
 * OTA added
 * Hostname added (mDNS)
 * 
*/
////////////////////////////////////////////////////////////////////////





#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Espalexa.h>   //espalxa

//needed for wifi manager library
#include <DNSServer.h>
#include <ESP8266mDNS.h>          //You can type hostnames in browser
#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <ESP8266Ping.h>          
#include <WS2812FX.h>
// Turn on debug statements to the serial output
#define  DEBUG  1

#if  DEBUG
#define  PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTLN(x) Serial.println(F(x))
#define PRINTD(x) Serial.println(x, DEC)

#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTLN(x)
#define PRINTD(x)

#endif


#if  DEBUG
//include ESP8266 SDK C functions to get heap
extern "C" {                                          // ESP8266 SDK C functions
#include "user_interface.h"
} 
#endif

///////////////////////////////////////////

//espalexa callback functions
void firstLightChanged(uint8_t brightness);
Espalexa espalexa;



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//OTA Section
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <ESP8266HTTPUpdateServer.h>
//const char* otahost = "MoodLight-webupdate";
const char* otaPort = "90";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "admin";

ESP8266WebServer httpServer(atoi(otaPort));
ESP8266HTTPUpdateServer httpUpdater;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////







time_t prevEvtTime = 0;
#define TIMER_MS 60000    //Interval between LED effects

unsigned long last_change = 0;      //ws2812fx
unsigned long now = 0;              //ws2812fx

//define your default values here, if there are different values in config.json, they are overwritten.
//length should be max size + 1 
//default custom static IP
char host_name[20] = "moodlight";
int port = 80;
char static_ip[16] = "192.168.1.155";
char static_gw[16] = "192.168.1.1";
char static_sn[16] = "255.255.255.0";

#define PIN D5

#define NUM_LEDS 30 // used in some effects
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN);
WS2812FX ws2812fx = WS2812FX(NUM_LEDS, PIN, NEO_RGB + NEO_KHZ800);

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)


//const char* ssid = "Suresh";
//const char* password = "mvls$1488";
ESP8266WebServer server(port);

byte R = 255;
byte G = 255;
byte B = 255;
bool randomFunction = true;
bool ws2812fxRun = true;



//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

////////////////////////////////////////////////////////////////////////////
//Handle root
////////////////////////////////////////////////////////////////////////////
void handleRoot() {
  Serial.println(server.arg(0) + "##" + server.arg(1) + "##" + server.arg(2) + "##" + server.arg("Random")); 

  
  R = server.arg("r").toInt();          // read RGB arguments
  G = server.arg("g").toInt();
  B = server.arg("b").toInt();

  if(server.arg("Random") == "On"){
  Serial.println("\nRandom On");
  randomFunction=true;}
  else {
  Serial.println("\nRandom Off");
  randomFunction=false;}

  if(server.arg("ws2812fx") == "On"){
  Serial.println("\ws2812fx On");
  ws2812fxRun=true;}
  else {
  Serial.println("\nRandom Off");
  ws2812fxRun=false;}

  
  
 // String green = server.arg(1);
  //String blue = server.arg(2);
  
//  analogWrite(R, red.toInt());
//  analogWrite(G, green.toInt());
//  analogWrite(B, blue.toInt());

   Serial.println(R);   // for TESTING
   Serial.println(G); // for TESTING
   Serial.println(B);  // for TESTING

  
  String webpage;     
      webpage += "<!DOCTYPE HTML>\r\n";
      webpage += "<html>\r\n";
      //webpage += "<header><title>ESP8266 RGB LED</title><h1>ESP8266 RGBLED</h1></header>";
      webpage += "<header><title>ESP8266 Web Server</title><h1>ESP8266 Web Server</h1></header>";

      
      webpage += "<p>Random #1 <a href=\"?Random=On\"><button>ON</button></a>&nbsp;<a href=\"?Random=Off\"><button>OFF</button></a></p>";
      webpage += "<p>WS2812FX #2 <a href=\"?ws2812fx=On\"><button>ON</button></a>&nbsp;<a href=\"?ws2812fx=Off\"><button>OFF</button></a></p>";

      webpage += "<h1>ESP8266 Mood Light - Select Color</h1>";
      
      webpage += "<head>";    
      webpage += "<meta name='mobile-web-app-capable' content='yes' />";
      webpage += "<meta name='viewport' content='width=device-width' />";
      webpage += "</head>";

      webpage += "<body style='margin: 0px; padding: 0px;'>";
      webpage += "<canvas id='colorspace'></canvas></body>";
     
      webpage += "<script type='text/javascript'>";
      webpage += "(function () {";
      webpage += " var canvas = document.getElementById('colorspace');";
      webpage += " var ctx = canvas.getContext('2d');";
      webpage += " function drawCanvas() {";
      webpage += " var colours = ctx.createLinearGradient(0, 0, window.innerWidth, 0);";
      webpage += " for(var i=0; i <= 360; i+=10) {";
      webpage += " colours.addColorStop(i/360, 'hsl(' + i + ', 100%, 50%)');";
      webpage += " }";
      
      webpage += " ctx.fillStyle = colours;";
      webpage += " ctx.fillRect(0, 0, window.innerWidth, window.innerHeight);";
      webpage += " var luminance = ctx.createLinearGradient(0, 0, 0, ctx.canvas.height);";
      webpage += " luminance.addColorStop(0, '#ffffff');";
      webpage += " luminance.addColorStop(0.05, '#ffffff');";
      webpage += " luminance.addColorStop(0.5, 'rgba(0,0,0,0)');";
      webpage += " luminance.addColorStop(0.95, '#000000');";
      webpage += " luminance.addColorStop(1, '#000000');";
      webpage += " ctx.fillStyle = luminance;";
      webpage += " ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);";
      webpage += " }";
      webpage += " var eventLocked = false;";
      
      webpage += " function handleEvent(clientX, clientY) {";
      webpage += " if(eventLocked) {";
      webpage += " return;";
      webpage += " }";
      
      webpage += " function colourCorrect(v) {";
      webpage += " return Math.round(1023-(v*v)/64);";
      webpage += " }";
      webpage += " var data = ctx.getImageData(clientX, clientY, 1, 1).data;";
      webpage += " var params = [";
    //  webpage += " 'r=' + colourCorrect(data[0]),";
    //  webpage += " 'g=' + colourCorrect(data[1]),";
   //   webpage += " 'b=' + colourCorrect(data[2])";

      webpage += " 'r=' + data[0],";
      webpage += " 'g=' + data[1],";
      webpage += " 'b=' + data[2]";

      
      webpage += " ].join('&');";
      webpage += " var req = new XMLHttpRequest();";
      webpage += " req.open('POST', '?' + params, true);";
      webpage += " req.send();";
      webpage += " eventLocked = true;";
      webpage += " req.onreadystatechange = function() {";
      webpage += " if(req.readyState == 4) {";
      webpage += " eventLocked = false;";
      webpage += " }";
      webpage += " }";
      webpage += " }";
      webpage += " canvas.addEventListener('click', function(event) {";
      webpage += " handleEvent(event.clientX, event.clientY, true);";
      webpage += " }, false);";
      webpage += " canvas.addEventListener('touchmove', function(event){";
      webpage += " handleEvent(event.touches[0].clientX, event.touches[0].clientY);";
      webpage += "}, false);";
      webpage += " function resizeCanvas() {";
      webpage += " canvas.width = window.innerWidth;";
      webpage += " canvas.height = window.innerHeight;";
      webpage += " drawCanvas();";
      webpage += " }";
      
      webpage += " window.addEventListener('resize', resizeCanvas, false);";
      webpage += " resizeCanvas();";
      webpage += " drawCanvas();";
      webpage += " document.ontouchmove = function(e) {e.preventDefault()};";
      webpage += " })();";   
      webpage += "</script><html>\r\n";

      server.send(200, "text/html", webpage);    
}




void handleNotFound(){
  //digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  //digitalWrite(led, 0);
}


////////////////////////////////////////////////////////////////////////////////////////





void setup() {
  #if  DEBUG
  Serial.begin(115200);
#endif

  strip.begin();
  strip.show(); 

 /* Before wifi manager 
  WiFi.begin(ssid, password);
  Serial.println("");
    // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
*/

WiFi.hostname(host_name);
////////////////////////////////////////////////////////////////////////////////////////////////
//WiFi Manager setup
////////////////////////////////////////////////////////////////////////////////////////////////

    //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

           if(json["ip"]) {
            Serial.println("setting custom ip from config");
            //static_ip = json["ip"];
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            //strcat(static_ip, json["ip"]);
            //static_gw = json["gateway"];
            //static_sn = json["subnet"];
            Serial.println(static_ip);
/*            Serial.println("converting ip");
            IPAddress ip = ipFromCharArray(static_ip);
            Serial.println(ip);*/
          } else {
            Serial.println("no custom ip in config");
          }
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  Serial.println(static_ip);


  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  IPAddress _ip,_gw,_sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);

  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");


  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();

    json["ip"] = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"] = WiFi.subnetMask().toString();

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
//////////////////////////////////////////////////////////////////////////////////////////////
  // Configure mDNS
  if (MDNS.begin(host_name)) {
    Serial.println("mDNS started. Hostname is set to " + String(host_name) + ".local");
  }
MDNS.addService("http", "tcp", port); // Announce the ESP as an HTTP service


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//OTA Code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MDNS.begin(otahost);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();
  //MDNS.addService("http", "tcp", 90);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local:%s%s in your browser and login with username '%s' and password '%s'\n", host_name,otaPort, update_path, update_username, update_password);
 //OTA code end
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  server.on("/", handleRoot);
////  server.onNotFound(handleNotFound); //espalexa
    server.onNotFound([](){
      Serial.println("Message Received");
      Serial.println(server.uri());
      Serial.println(server.arg(0));
      Serial.println(server.arg("plain"));
      if (!espalexa.handleAlexaApiCall(server.uri(),server.arg(0))) //if you don't know the URI, ask espalexa whether it is an Alexa control request
      {
        //whatever you want to do with 404s
        server.send(404, "text/plain", "Not found");
      }
    });
    


  
  server.begin();
  Serial.println("HTTP server started");

   // Define your devices here.
    espalexa.addDevice("Red Moodlight", firstLightChanged); //simplest definition, default state off
    espalexa.begin(&server); //give espalexa a pointer to your server object so it can use your server instead of creating its own

  

  ws2812fx.init();
  ws2812fx.setBrightness(255);
  ws2812fx.setSpeed(1000);
  ws2812fx.setColor(0x007BFF);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.start();
  
}

// *** REPLACE FROM HERE ***
void loop() {
loopStart:   
    ////server.handleClient();    // Listen for HTTP requests from clients. you can omit this line from your code since it will be called in espalexa.loop()
     espalexa.loop();
     
    httpServer.handleClient(); // OTA code
    setAll(R,G,B);
//    delay(100);
////////////////////////////////////////////////////////////////////////////////////////////////
//Reset if gw not pinging
////////////////////////////////////////////////////////////////////////////////////////////////
if ((millis()-prevEvtTime) > TIMER_MS){             //every 1 min or when loop starts
//Serial.println(ESP.getResetInfo());
espRestartOnNoPing();    //Restart ESP if GW is not reachable
  prevEvtTime=millis();
}
/////////////////////////////////////////////////////////////////////////////////////////////////

if (ws2812fxRun) {
 now = millis();
  ws2812fx.service();
  if(now - last_change > TIMER_MS) {
    ws2812fx.setMode((ws2812fx.getMode() + 1) % ws2812fx.getModeCount());
    last_change = now;
  }
}
else { delay(100);}


if (randomFunction) {
 //uncomment for animations
long timeStart = millis();


timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
 theaterChase(0xff,0,0,50);
 server.handleClient();
 if (!randomFunction) {goto loopStart;}  
 Serial.println("function 1 started");
 }
 

timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
 theaterChaseRainbow(50);
 server.handleClient();
 if (!randomFunction) {goto loopStart;} 
 Serial.println("function 2 started");
 }
        
timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
Strobe(0xff, 0xff, 0xff, 10, 50, 1000);
 server.handleClient();
 if (!randomFunction) {goto loopStart;} 
Serial.println("function 3 started");
}


timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
NewKITT(0xff, 0, 0, 8, 10, 50);
 server.handleClient();
 if (!randomFunction) {goto loopStart;} 
Serial.println("function 4 started");
}


timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
 TwinkleRandom(20, 100, false);
  server.handleClient();
 if (!randomFunction) {goto loopStart;} 
 Serial.println("function 5 started");
 }
 

timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
 SnowSparkle(0x10, 0x10, 0x10, 20, random(100,1000));
  server.handleClient();
 if (!randomFunction) {goto loopStart;} 
 Serial.println("function 6 started");
 }
 

timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
 colorWipe(0x00,0xff,0x00, 50);
  colorWipe(0x00,0x00,0x00, 50);
   server.handleClient();
 if (!randomFunction) {goto loopStart;} 
  Serial.println("function 7 started");
  }
  

timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
   rainbowCycle(20);
    server.handleClient();
 if (!randomFunction) {goto loopStart;} 
   Serial.println("function 8 started");}
   

timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
 theaterChase(0xff,0,0,50);
  server.handleClient();
 if (!randomFunction) {goto loopStart;}   
 Serial.println("function 9 started");
 }
 

timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
 theaterChaseRainbow(50);
  server.handleClient();
 if (!randomFunction) {goto loopStart;} 
 Serial.println("function 10 started");
 }
 

timeStart = millis();
  while ((timeStart + TIMER_MS) > millis()) {
 Fire(55,120,15);
  server.handleClient();
 if (!randomFunction) {goto loopStart;} 
 Serial.println("function 11 started");
  }
 
Serial.println("function 11 Ended");

timeStart = millis();
  while ((timeStart + 2*TIMER_MS) > millis()) {
 byte colors[3][3] = { {0xff, 0,0}, 
                        {0xff, 0xff, 0xff}, 
                        {0   , 0   , 0xff} };

  BouncingColoredBalls(3, colors);
  Serial.println("BouncingColoredBalls Ended");
 BouncingBalls(0xff,0,0, 3);
  server.handleClient();
 if (!randomFunction) {goto loopStart;} 
  Serial.println("function 12 ended");
  }
 // ---> here we call the effect function <---
 
}
}


void CenterToOutside(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i =((NUM_LEDS-EyeSize)/2); i>=0; i--) {
    setAll(0,0,0);
    
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    
    setPixel(NUM_LEDS-i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(NUM_LEDS-i-j, red, green, blue); 
    }
    setPixel(NUM_LEDS-i-EyeSize-1, red/10, green/10, blue/10);
    
    showStrip();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void OutsideToCenter(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = 0; i<=((NUM_LEDS-EyeSize)/2); i++) {
    setAll(0,0,0);
    
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    
    setPixel(NUM_LEDS-i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(NUM_LEDS-i-j, red, green, blue); 
    }
    setPixel(NUM_LEDS-i-EyeSize-1, red/10, green/10, blue/10);
    
    showStrip();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void LeftToRight(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = 0; i < NUM_LEDS-EyeSize-2; i++) {
    setAll(0,0,0);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    showStrip();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void RightToLeft(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = NUM_LEDS-EyeSize-2; i > 0; i--) {
    setAll(0,0,0);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    showStrip();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void NewKITT(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay){
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
}

void RGBLoop(){
  for(int j = 0; j < 3; j++ ) { 
    // Fade IN
    for(int k = 0; k < 256; k++) { 
      switch(j) { 
        case 0: setAll(k,0,0); break;
        case 1: setAll(0,k,0); break;
        case 2: setAll(0,0,k); break;
      }
      showStrip();
      delay(3);
    }
    // Fade OUT
    for(int k = 255; k >= 0; k--) { 
      switch(j) { 
        case 0: setAll(k,0,0); break;
        case 1: setAll(0,k,0); break;
        case 2: setAll(0,0,k); break;
      }
      showStrip();
      delay(3);
    }
  }
}

void Strobe(byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause){
  for(int j = 0; j < StrobeCount; j++) {
    setAll(red,green,blue);
    showStrip();
    delay(FlashDelay);
    setAll(0,0,0);
    showStrip();
    delay(FlashDelay);
  }
 
 delay(EndPause);
}

void BouncingColoredBalls(int BallCount, byte colors[][3]) {
  float Gravity = -9.81;
  int StartHeight = 1;
  
  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * StartHeight );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];

  long timeStart1 = millis();
Serial.println("BouncingColoredBalls variable assignment done");
  
  for (int i = 0 ; i < BallCount ; i++) {   
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = StartHeight;
    Position[i] = 0; 
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.90 - float(i)/pow(BallCount,2); 
  }

Serial.println("BouncingColoredBalls for loop Ended");

  //while (true) {
  while ((timeStart1 + TIMER_MS) > millis()) {
    for (int i = 0 ; i < BallCount ; i++) {
      TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i]/1000 , 2.0 ) + ImpactVelocity[i] * TimeSinceLastBounce[i]/1000;
  
      if ( Height[i] < 0 ) {                      
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();
  
        if ( ImpactVelocity[i] < 0.01 ) {
          ImpactVelocity[i] = ImpactVelocityStart;
        }
      }
      Position[i] = round( Height[i] * (NUM_LEDS - 1) / StartHeight);
    }
  
    for (int i = 0 ; i < BallCount ; i++) {
      setPixel(Position[i],colors[i][0],colors[i][1],colors[i][2]);
    }
    
    showStrip();
    yield();
    server.handleClient();
    if (!randomFunction) {return;} 
    setAll(0,0,0);
  }
}

void BouncingBalls(byte red, byte green, byte blue, int BallCount) {
  float Gravity = -9.81;
  int StartHeight = 1;
  
  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * StartHeight );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];

  long timeStart1 = millis();
  
  for (int i = 0 ; i < BallCount ; i++) {   
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = StartHeight;
    Position[i] = 0; 
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.90 - float(i)/pow(BallCount,2); 
  }

   while ((timeStart1 + TIMER_MS) > millis()) {
    for (int i = 0 ; i < BallCount ; i++) {
      TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i]/1000 , 2.0 ) + ImpactVelocity[i] * TimeSinceLastBounce[i]/1000;
  
      if ( Height[i] < 0 ) {                      
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();
  
        if ( ImpactVelocity[i] < 0.01 ) {
          ImpactVelocity[i] = ImpactVelocityStart;
        }
      }
      Position[i] = round( Height[i] * (NUM_LEDS - 1) / StartHeight);
    }
  
    for (int i = 0 ; i < BallCount ; i++) {
      setPixel(Position[i],red,green,blue);
    }
    
    showStrip();
    yield();
    server.handleClient();
    if (!randomFunction) {return;} 
    setAll(0,0,0);
  }
}

void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[NUM_LEDS];
  int cooldown;
  
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUM_LEDS; i++) {
    cooldown = random(0, ((Cooling * 10) / NUM_LEDS) + 2);
    
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
  
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
    
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for( int j = 0; j < NUM_LEDS; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  showStrip();
  delay(SpeedDelay);
}

void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature/255.0)*191);
 
  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  // figure out which third of the spectrum we're in:
  if( t192 > 0x80) {                     // hottest
    setPixel(Pixel, 255, 255, heatramp);
  } else if( t192 > 0x40 ) {             // middle
    setPixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0);
  }
}

void theaterChaseRainbow(int SpeedDelay) {
  byte *c;
  
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < NUM_LEDS; i=i+3) {
          c = Wheel( (i+j) % 255);
          setPixel(i+q, *c, *(c+1), *(c+2));    //turn every third pixel on
        }
        showStrip();
       
        delay(SpeedDelay);
       
        for (int i=0; i < NUM_LEDS; i=i+3) {
          setPixel(i+q, 0,0,0);        //turn every third pixel off
        }
    }
  }
}

void theaterChase(byte red, byte green, byte blue, int SpeedDelay) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < NUM_LEDS; i=i+3) {
        setPixel(i+q, red, green, blue);    //turn every third pixel on
      }
      showStrip();
     
      delay(SpeedDelay);
     
      for (int i=0; i < NUM_LEDS; i=i+3) {
        setPixel(i+q, 0,0,0);        //turn every third pixel off
      }
    }
  }
}

void rainbowCycle(int SpeedDelay) {
  byte *c;
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< NUM_LEDS; i++) {
      c=Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      setPixel(i, *c, *(c+1), *(c+2));
    }
    showStrip();
    delay(SpeedDelay);
  }
}

byte * Wheel(byte WheelPos) {
  static byte c[3];
  
  if(WheelPos < 85) {
   c[0]=WheelPos * 3;
   c[1]=255 - WheelPos * 3;
   c[2]=0;
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   c[0]=255 - WheelPos * 3;
   c[1]=0;
   c[2]=WheelPos * 3;
  } else {
   WheelPos -= 170;
   c[0]=0;
   c[1]=WheelPos * 3;
   c[2]=255 - WheelPos * 3;
  }

  return c;
}

void colorWipe(byte red, byte green, byte blue, int SpeedDelay) {
  for(uint16_t i=0; i<NUM_LEDS; i++) {
      setPixel(i, red, green, blue);
      showStrip();
      delay(SpeedDelay);
  }
}

void SnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay) {
  setAll(red,green,blue);
  
  int Pixel = random(NUM_LEDS);
  setPixel(Pixel,0xff,0xff,0xff);
  showStrip();
  delay(SparkleDelay);
  setPixel(Pixel,red,green,blue);
  showStrip();
  delay(SpeedDelay);
}


void TwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0,0,0);
  
  for (int i=0; i<Count; i++) {
     setPixel(random(NUM_LEDS),random(0,255),random(0,255),random(0,255));
     showStrip();
     delay(SpeedDelay);
     if(OnlyOne) { 
       setAll(0,0,0); 
     }
   }
  
  delay(SpeedDelay);
}




// ---> here we define the effect function <---
// *** REPLACE TO HERE ***

void showStrip() {
 #ifdef ADAFRUIT_NEOPIXEL_H 
   // NeoPixel
   strip.show();  
 #endif
 #ifndef ADAFRUIT_NEOPIXEL_H
   // FastLED
   FastLED.show();
 #endif
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
 #ifdef ADAFRUIT_NEOPIXEL_H 
   // NeoPixel
   strip.setPixelColor(Pixel, strip.Color(red, green, blue));
 #endif
 #ifndef ADAFRUIT_NEOPIXEL_H 
   // FastLED
   leds[Pixel].r = red;
   leds[Pixel].g = green;
   leds[Pixel].b = blue;
 #endif
}

void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue); 
  }
  showStrip();
}


//===============================================
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void espRestartOnNoPing() {
if(Ping.ping(WiFi.gatewayIP())) {
    Serial.print("\nGW Pinging!!");
  } 
else {
    Serial.print("\nGW not Pinging, Restarting Now :(");
    ESP.restart();
      }
  }
  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//espalexa functions
//our callback functions
void firstLightChanged(uint8_t brightness) {
    Serial.print("Device 1 changed to ");
    
    //do what you need to do here

    //EXAMPLE
    if (brightness == 255) {
      Serial.println("Red Moodlight Full ON");
    }
    else if (brightness == 0) {
      Serial.println("Red Moodlight OFF");
    }
    else {
      Serial.print("Red Moodlight DIM "); Serial.println(brightness);
      setAll(brightness,0,0);
    }
}
