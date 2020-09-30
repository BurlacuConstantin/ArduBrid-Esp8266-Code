#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

//Static IP address configuration
/*
IPAddress staticIP(192, 172, 0, 101); //ESP static ip
IPAddress gateway(192, 172, 0, 2);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
*/

/* Made with esp 

/*
 relay1 -> D5
 relay2 -> D6
 relay3 -> D7
 relay4 -> D8
*/
enum
{
  D0   =  16,
  D1   =  5,
  D2   =  4,
  D3   =  0,
  D4   =  2,
  D5   =  14,
  D6   =  12,
  D7   =  13,
  D8   =  15,
  D9   =  3,
  D10  =  1,
};

ESP8266WebServer server(80);
WiFiClient client;

char* ssid;
char* password;

#define ATTEMPTS  26
bool connectedWifi = false;
bool ReadMsg = false;
String Msg = "";
byte connecting_attempts = 0;

byte updateWeb = 0;

enum myStatus
{
  WIFI_STATUS_FAILED = 0,
  WIFI_STATUS_CONNECTED
};

myStatus WifiStatus;

struct Relays
{
  bool relay1;
  bool relay2;
  bool relay3;
  bool relay4;
};

Relays systemRelays;

DynamicJsonDocument doc(1024);

void setup()
{
  Serial.begin(115200);

  pinMode(D5, OUTPUT); 
  pinMode(D6, OUTPUT); 
  pinMode(D7, OUTPUT); 
  pinMode(D8, OUTPUT);

  pinoutInit(); // all outputs are in low state and struct relays are false
}

void loop()
{
    while(Serial.available() > 0) // need to be > 0
    {
      Msg = Serial.readString();
      ReadMsg = true;
    }

    if(ReadMsg)
    {
      DeserializationError error = deserializeJson(doc, Msg);

      if(error)
      { 
        // Do not print message on UART communication
        ReadMsg = false;
      }
      // {"type":"GiveWIFIdata","ssid":"pass","pass":"wifi id"}
      if(doc["type"] == "GiveWIFIdata")
      {
        if(WiFi.status() == WL_CONNECTED) // check to see if is already connected to some network before trying to connect to another one
        {
          WiFi.disconnect(true);
          connectedWifi == false;
        }
        
        String d = doc["ssid"];
        String b = doc["pass"];
        ssid = (char *)malloc(d.length() + 1); // allocate in memory a char pointer with income ssid length
        memcpy(ssid, d.c_str(), d.length() + 1); // copy into ssid, the string with his length
        password = (char *)malloc(b.length() + 1);
        memcpy(password, b.c_str(), b.length() + 1);

        doc.clear(); // clear old doc 

        WiFi.mode(WIFI_STA);
        //WiFi.config(staticIP, subnet, gateway);
        WiFi.begin(ssid, password);   
 
        while(WiFi.status() != WL_CONNECTED)
        {
          if(connecting_attempts >= ATTEMPTS)
          {
            WifiStatus = WIFI_STATUS_FAILED;
            connecting_attempts = 0; // reset the attempts counter 
            break;
          }
          else
          {
            WifiStatus = WIFI_STATUS_CONNECTED;
          }
          
          connecting_attempts++; // increment attempts to create a timeout
          delay(500);
        }
        
        if(WifiStatus == WIFI_STATUS_CONNECTED)
        {
            doc["type"] = "ReturnIP";
            doc["IP"] = WiFi.localIP().toString();
            connectedWifi = true;
            pinoutInit(); // reinitialize variabile and pins if we are connecting to another wifi access point

            
            server.on("/", []() { server.send(200, "text/html", HTML_Page()); });
            server.on("/r1", r1_state);
            server.on("/r1_btn", r1_state_btn);
            server.on("/r2", r2_state);
            server.on("/r2_btn", r2_state_btn);
            server.on("/r3", r3_state);
            server.on("/r3_btn", r3_state_btn);
            server.on("/r4", r4_state);
            server.on("/r4_btn", r4_state_btn);
            server.on("/update_val", checkUpdate);


            // auxilary for android app
            server.on("/vc_r1_off", HTTP_GET, voice_r1_off);
            server.on("/vc_r1_on", HTTP_GET,  voice_r1_on);
            server.on("/vc_r2_off", HTTP_GET, voice_r2_off);
            server.on("/vc_r2_on", HTTP_GET,  voice_r2_on);
            server.on("/vc_r3_off", HTTP_GET, voice_r3_off);
            server.on("/vc_r3_on", HTTP_GET,  voice_r3_on);
            server.on("/vc_r4_off", HTTP_GET, voice_r4_off);
            server.on("/vc_r4_on", HTTP_GET,  voice_r4_on);
           
            server.begin();
            
            serializeJson(doc, Serial);
        }
        else
        {
            doc["type"] = "ReturnIP";
            doc["IP"] = "Failed";
            connectedWifi = false;

            serializeJson(doc, Serial);
        }
        
        ReadMsg = false; // we've read the message succesfuly
        free(ssid); // delete from memory the allocation cells
        free(password); // delete from memory the allocation cells
        doc.clear(); // clear again doc
    }
    if(doc["Relay1"] == "ON")
    {
      if(systemRelays.relay1 != true)
      {
        systemRelays.relay1 = true;
        digitalWrite(D5, HIGH);
        updateWeb = 1;
        ReadMsg = false;
      }
    }
    if(doc["Relay1"] == "OFF")
    {
      if(systemRelays.relay1 != false)
      {
        systemRelays.relay1 = false;
        digitalWrite(D5, LOW);
        updateWeb = 1;
        ReadMsg = false;
      }
    }
    if(doc["Relay2"] == "ON")
    {
      if(systemRelays.relay2 != true)
      {
        systemRelays.relay2 = true;
        digitalWrite(D6, HIGH);
        updateWeb = 1;
        ReadMsg = false;
      }
    }
    if(doc["Relay2"] == "OFF")
    {
      if(systemRelays.relay2 != false)
      {
        systemRelays.relay2 = false;
        digitalWrite(D6, LOW);
        updateWeb = 1;
        ReadMsg = false;
      }
    }
    if(doc["Relay3"] == "ON")
    {
      if(systemRelays.relay3 != true)
      {
        systemRelays.relay3 = true;
        digitalWrite(D7, HIGH);
        updateWeb = 1;
        ReadMsg = false;
      }
    }
    if(doc["Relay3"] == "OFF")
    {
      if(systemRelays.relay3 != false)
      {
        systemRelays.relay3 = false;
        digitalWrite(D7, LOW);
        updateWeb = 1;
        ReadMsg = false;
      }
    }
    if(doc["Relay4"] == "ON")
    {
      if(systemRelays.relay4 != true)
      {
        systemRelays.relay4 = true;
        digitalWrite(D8, HIGH);
        updateWeb = 1;
        ReadMsg = false;
      }
    }
    if(doc["Relay4"] == "OFF")
    {
      if(systemRelays.relay4 != false)
      {
        systemRelays.relay4 = false;
        digitalWrite(D8, LOW);
        updateWeb = 1;
        ReadMsg = false;
      }
    }
  }
  
  if(connectedWifi)
  {
    server.handleClient();
  }
}


void voice_r1_off()
{
   if(systemRelays.relay1 == false)
   {
      server.send(200, "text/plain", "0");
   }
   else
   {
      systemRelays.relay1 = false;
      server.send(200, "text/plain", "1");
      digitalWrite(D5, LOW);
      doc["Relay1"] = "OFF";
  }

  serializeJson(doc, Serial);
  doc.clear();
   
  updateWeb = 1;

}

void voice_r1_on()
{
   if(systemRelays.relay1 == true)
   {
      server.send(200, "text/plain", "0");
   }
   else
   {
      systemRelays.relay1 = true;
      server.send(200, "text/plain", "1");
      digitalWrite(D5, HIGH);
      doc["Relay1"] = "ON";
   }

   serializeJson(doc, Serial);
   doc.clear();

   updateWeb = 1;

}

void voice_r2_off()
{
   if(systemRelays.relay2 == false)
   {
      server.send(200, "text/plain", "0");
   }
   else
   {
      systemRelays.relay2 = false;
      server.send(200, "text/plain", "1");
      digitalWrite(D6, LOW);
      doc["Relay2"] = "OFF";
   }
   
   serializeJson(doc, Serial);
   doc.clear();
   
   updateWeb = 1;
}

void voice_r2_on()
{
   if(systemRelays.relay2 == true)
   {
      server.send(200, "text/plain", "0");
   }
   else
   {
      systemRelays.relay2 = true;
      server.send(200, "text/plain", "1");
      digitalWrite(D6, HIGH);
      doc["Relay2"] = "ON";
   }

   serializeJson(doc, Serial);
   doc.clear();

   updateWeb = 1;
}

void voice_r3_off()
{
   if(systemRelays.relay3 == false)
   {
      server.send(200, "text/plain", "0");
   }
   else
   {
      systemRelays.relay3 = false;
      server.send(200, "text/plain", "1");
      digitalWrite(D7, LOW);
      doc["Relay3"] = "OFF";
   }

   serializeJson(doc, Serial);
   doc.clear();

   updateWeb = 1;
}

void voice_r3_on()
{
   if(systemRelays.relay3 == true)
   {
      server.send(200, "text/plain", "0");
   }
   else
   {
      systemRelays.relay3 = true;
      server.send(200, "text/plain", "1");
      digitalWrite(D7, HIGH);
      doc["Relay3"] = "ON";
   }

  serializeJson(doc, Serial);
   doc.clear();
   
   updateWeb = 1;

}

void voice_r4_off()
{
   if(systemRelays.relay4 == false)
   {
      server.send(200, "text/plain", "0");
   }
   else
   {
      systemRelays.relay4 = false;
      server.send(200, "text/plain", "1");
      digitalWrite(D8, LOW);
      doc["Relay4"] = "OFF";
   }

   serializeJson(doc, Serial);
   doc.clear();

   updateWeb = 1;
}

void voice_r4_on()
{
   if(systemRelays.relay4 == true)
   {
      server.send(200, "text/plain", "0");
   }
   else
   {
      systemRelays.relay4 = true;
      server.send(200, "text/plain", "1");
      digitalWrite(D8, HIGH);
      doc["Relay4"] = "ON";
   }

  serializeJson(doc, Serial);
   doc.clear();
   
   updateWeb = 1;

}

void checkUpdate()
{
  server.send(200, "text/plain", String(updateWeb));
}

void r1_state_btn()
{
  if(systemRelays.relay1)
  {
    systemRelays.relay1 = false;
    digitalWrite(D5, LOW);
    doc["Relay1"] = "OFF";
  }
  else
  {
    systemRelays.relay1 = true;
    digitalWrite(D5, HIGH);
    doc["Relay1"] = "ON";
  }

  serializeJson(doc, Serial);
  doc.clear();
  
  r1_state();
}
void r1_state()
{
  if(systemRelays.relay1)
  {
     server.send(200, "text/html", "<i class=\"fas fa-power-off\" style=\"color:#00cc00;\"></i><p><b>Relay 1 Status: ON</b></p><a class=\"button button-off\" onClick=\"relay1_stats()\" id=\"btn1\">OFF</a>\n");
  }
  else if(!systemRelays.relay1)
  {
     server.send(200, "text/html", "<i class=\"fas fa-power-off\" style=\"color:#828282;\"></i><p><b>Relay 1 Status: OFF</b></p><a class=\"button button-on\" onClick=\"relay1_stats()\" id=\"btn1\">ON</a>\n");
  }

  updateWeb = 0;
}

void r2_state_btn()
{
  if(systemRelays.relay2)
  {
    systemRelays.relay2 = false;
    digitalWrite(D6, LOW);
    doc["Relay2"] = "OFF";
    
  }
  else
  {
    systemRelays.relay2 = true;
    digitalWrite(D6, HIGH);
    doc["Relay2"] = "ON";
  }

  serializeJson(doc, Serial);
  doc.clear();
  
  r2_state();
}
void r2_state()
{
  if(systemRelays.relay2)
  {
    server.send(200, "text/html", "<i class=\"fas fa-power-off\" style=\"color:#00cc00;\"></i><p><b>Relay 2 Status: ON</b></p><a class=\"button button-off\" onClick=\"relay2_stats()\" id=\"btn2\">OFF</a>\n");
  }
  else
  {
    server.send(200, "text/html", "<i class=\"fas fa-power-off\" style=\"color:#828282;\"></i><p><b>Relay 2 Status: OFF</b></p><a class=\"button button-on\" onClick=\"relay2_stats()\" id=\"btn2\">ON</a>\n");
  }

  updateWeb = 0;
}

void r3_state_btn()
{
  if(systemRelays.relay3)
  {
    systemRelays.relay3 = false;
    digitalWrite(D7, LOW);
    doc["Relay3"] = "OFF";
  }
  else
  {
    systemRelays.relay3 = true;
    digitalWrite(D7, HIGH);
    doc["Relay3"] = "ON";
  }

  serializeJson(doc, Serial);
  doc.clear();
  
  r3_state();
}
void r3_state()
{
  if(systemRelays.relay3)
  {
    server.send(200, "text/html", "<i class=\"fas fa-power-off\" style=\"color:#00cc00;\"></i><p><b>Relay 3 Status: ON</b></p><a class=\"button button-off\" onClick=\"relay3_stats()\" id=\"btn3\">OFF</a>\n");
  }
  else
  {
    server.send(200, "text/html", "<i class=\"fas fa-power-off\" style=\"color:#828282;\"></i><p><b>Relay 3 Status: OFF</b></p><a class=\"button button-on\" onClick=\"relay3_stats()\" id=\"btn3\">ON</a>\n");
  }

  updateWeb = 0;
}

void r4_state_btn()
{
  if(systemRelays.relay4)
  {
    systemRelays.relay4 = false;
    digitalWrite(D8, LOW);
    doc["Relay4"] = "OFF";
  }
  else
  {
    systemRelays.relay4 = true;
    digitalWrite(D8, HIGH);
    doc["Relay4"] = "ON";
  }

  serializeJson(doc, Serial);
  doc.clear();
  
  r4_state();
}
void r4_state()
{
  if(systemRelays.relay4)
  {
    server.send(200, "text/html", "<i class=\"fas fa-power-off\" style=\"color:#00cc00;\"></i><p><b>Relay 4 Status: ON</b></p><a class=\"button button-off\" onClick=\"relay4_stats()\" id=\"btn4\">OFF</a>\n");
  }
  else
  {
    server.send(200, "text/html", "<i class=\"fas fa-power-off\" style=\"color:#828282;\"></i><p><b>Relay 4 Status: OFF</b></p><a class=\"button button-on\" onClick=\"relay4_stats()\" id=\"btn4\">ON</a>\n");
  }

  updateWeb = 0;
}


String HTML_Page()
{
  String page = "<!DOCTYPE html> <html>\n";
  page += "<head><meta name=\"viewport\" content=\"width = device-width, initial-scale=1.0, user-scalable=no\">\n";
  page += "<link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\" integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">\n";
  
  page += "<title>Relay Management</title>\n";
  page += "<style>html { font-family:Arial; display: inline-block; margin:0px auto; text-align:center; background-color:#1f1b24; }\n";
  
  page += "body{margin: 0; padding: 0;} h1 {color: #E8E8E8;margin: 50px auto 30px;} h3 {color: #D3D3D3;margin-bottom: 50px;}\n";  
  
  page += ".button {display: block; width: 80px; background-color: #444444;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  
  page += ".button-on {background-color: #FF8C00;}\n";
  page += ".button-on:active {background-color: #e37d00;}\n";
  page += ".button-off {background-color: #444444;}\n";
  page += ".button-off:active {background-color: #6e6e6e;}\n";
  page += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";

  page += ".container { position:absolute; right: 50%; transform: translateX(50%); animation: shake 0.5s;} \n"; /**/

  page += "span.initcap { display: none; }\n";
  
  page += "fieldset { border: none; }\n";
  page += ".effect { animation: shake 0.6s; }\n";
  page += "@keyframes shake { 0% { margin-left: 0 } 12% { margin-left: 0 } 25% { margin-left: -10px } 50% { margin-left: 10px } 75% { margin-left: -10px } 87% { margin-left: 0 } 100% { margin-left: 0 } }\n";
  page += "</style>\n";
  
  page += "</head>\n";
  page += "<body>\n";
  
  page += "<h1> Welcome to ArduBrid ! </h1>\n";
  page += "<h3> System Management WebPage </h3>\n";

  page += "<span id=\"update\" style=\"display: none\"> </span>\n";

  page += "<div id=\"myblock\" class=\"container\">\n";
  page += "<fieldset>\n";
  page += "<div id=\"r1_state\"> </div>\n";
  page += "<div id=\"r2_state\"> </div>\n";
  page += "<div id=\"r3_state\"> </div>\n";
  page += "<div id=\"r4_state\"> </div>\n";
  page += "</fieldset>\n";
  page += "</div>\n";

  page += "<script>\n";

  
  page += "function create_update()\n";
  page += "{ var xhr = new XMLHttpRequest();\n";
  page += "var url = \"update_val\";\n";
  page += "xhr.onreadystatechange = function() {\n";
  page += "if(this.readyState == 4 && this.status == 200) {\n";
  page += "document.getElementById(\"update\").innerText = this.responseText;\n";
  page += "var txt = document.getElementById(\"update\").innerText;\n";
  page += "if(txt == \"1\")\n";
  page += "{\n";
  page += "for(let i = 1; i <= 4; i++)\n";
  page += "{\n";
  page += "anim(i);\n";
  page += "}\n";
  page += "}\n";
  page += "}\n";
  page += "};\n";
  page += "xhr.open(\"GET\", url, true);\n";
  page += "xhr.send();\n";
  page += "}\n";
  

  page += "function anim(i)\n";
  page += "{\n";
  page += "var btn;\n";
  page += "fVar = setTimeout(function()\n";
  page += "{\n";
  page += "btn = document.getElementById(\"btn\" + (i));\n";
  page += "btn.classList.remove(\"effect\");\n";
  page += "setTimeout(function() { btn.classList.add(\"effect\"); }, 1);\n";
  page += "}, 3);\n";
  page += "}\n";
  
  page += "function relay1_func()\n";
  page += "{ var xhr = new XMLHttpRequest();\n";
  page += "var url = \"r1\";\n";
  page += "xhr.onreadystatechange = function() {\n";
  page += "if(this.readyState == 4 && this.status == 200) {\n";
  page += "document.getElementById(\"r1_state\").innerHTML = this.responseText;\n";
  page += "}\n";
  page += "};\n";
  page += "xhr.open(\"GET\", url, true);\n";
  page += "xhr.send();\n";
  page += "}\n";
  page += "document.addEventListener('DOMContentLoaded', relay1_func, true);\n";

  page += "function relay1_stats()\n";
  page += "{ var xhr = new XMLHttpRequest();\n";
  page += "var url = \"r1_btn\";\n";
  page += "xhr.onreadystatechange = function() {\n";
  page += "if(this.readyState == 4 && this.status == 200) {\n";
  page += "document.getElementById(\"r1_state\").innerHTML = this.responseText;\n";
  page += "}\n";
  page += "};\n";
  page += "xhr.open(\"GET\", url, true);\n";
  page += "xhr.send();\n";
  page += "}\n";

  page += "function relay2_func()\n";
  page += "{ var xhr = new XMLHttpRequest();\n";
  page += "var url = \"r2\";\n";
  page += "xhr.onreadystatechange = function() {\n";
  page += "if(this.readyState == 4 && this.status == 200) {\n";
  page += "document.getElementById(\"r2_state\").innerHTML = this.responseText;\n";
  page += "}\n";
  page += "};\n";
  page += "xhr.open(\"GET\", url, true);\n";
  page += "xhr.send();\n";
  page += "}\n";
  page += "document.addEventListener('DOMContentLoaded', relay2_func, true);\n";

  page += "function relay2_stats()\n";
  page += "{ var xhr = new XMLHttpRequest();\n";
  page += "var url = \"r2_btn\";\n";
  page += "xhr.onreadystatechange = function() {\n";
  page += "if(this.readyState == 4 && this.status == 200) {\n";
  page += "document.getElementById(\"r2_state\").innerHTML = this.responseText;\n";
  page += "}\n";
  page += "};\n";
  page += "xhr.open(\"GET\", url, true);\n";
  page += "xhr.send();\n";
  page += "}\n";

  page += "function relay3_func()\n";
  page += "{ var xhr = new XMLHttpRequest();\n";
  page += "var url = \"r3\";\n";
  page += "xhr.onreadystatechange = function() {\n";
  page += "if(this.readyState == 4 && this.status == 200) {\n";
  page += "document.getElementById(\"r3_state\").innerHTML = this.responseText;\n";
  page += "}\n";
  page += "};\n";
  page += "xhr.open(\"GET\", url, true);\n";
  page += "xhr.send();\n";
  page += "}\n";
  page += "document.addEventListener('DOMContentLoaded', relay3_func, true);\n";

  page += "function relay3_stats()\n";
  page += "{ var xhr = new XMLHttpRequest();\n";
  page += "var url = \"r3_btn\";\n";
  page += "xhr.onreadystatechange = function() {\n";
  page += "if(this.readyState == 4 && this.status == 200) {\n";
  page += "document.getElementById(\"r3_state\").innerHTML = this.responseText;\n";
  page += "}\n";
  page += "};\n";
  page += "xhr.open(\"GET\", url, true);\n";
  page += "xhr.send();\n";
  page += "}\n";

  page += "function relay4_func()\n";
  page += "{ var xhr = new XMLHttpRequest();\n";
  page += "var url = \"r4\";\n";
  page += "xhr.onreadystatechange = function() {\n";
  page += "if(this.readyState == 4 && this.status == 200) {\n";
  page += "document.getElementById(\"r4_state\").innerHTML = this.responseText;\n";
  page += "}\n";
  page += "};\n";
  page += "xhr.open(\"GET\", url, true);\n";
  page += "xhr.send();\n";
  page += "}\n";
  page += "document.addEventListener('DOMContentLoaded', relay4_func, true);\n";

  page += "function relay4_stats()\n";
  page += "{ var xhr = new XMLHttpRequest();\n";
  page += "var url = \"r4_btn\";\n";
  page += "xhr.onreadystatechange = function() {\n";
  page += "if(this.readyState == 4 && this.status == 200) {\n";
  page += "document.getElementById(\"r4_state\").innerHTML = this.responseText;\n";
  page += "}\n";
  page += "};\n";
  page += "xhr.open(\"GET\", url, true);\n";
  page += "xhr.send();\n";
  page += "}\n";

  page += "setInterval(function() { create_update(); }, 1100);\n";
  page += "setInterval(function() { relay1_func(); },   2000);\n";
  page += "setInterval(function() { relay2_func(); },   2000);\n";
  page += "setInterval(function() { relay3_func(); },   2000);\n";
  page += "setInterval(function() { relay4_func(); },   2000);\n";
  page += "</script>\n";
  
  page += "</body>\n";
  page += "</html>\n";

  return page;
}

void pinoutInit()
{
  digitalWrite(D5, LOW); 
  digitalWrite(D6, LOW); 
  digitalWrite(D7, LOW); 
  digitalWrite(D8, LOW);
  
  systemRelays.relay1 = false;
  systemRelays.relay2 = false;
  systemRelays.relay3 = false;
  systemRelays.relay4 = false;
}




