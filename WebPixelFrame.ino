/*
  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done

  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit
*/

// To build a clock, look at:
// http://www.esp8266.com/viewtopic.php?f=32&t=2881

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
//#include <ArduinoJson.h> 
#include "NewBitmap.h"
#include "DisplayPixelsLive.h"
#include "DisplayPixelsText.h"
#include "DisplayPixelsAnimatedGIF.h"

#include "CaptivePortalAdvanced.h"

#define DBG_OUTPUT_PORT Serial


const char* host = "WebPixelFrame";


NeoPixelBus<MyPixelColorFeature, Neo800KbpsMethod> *strip = new NeoPixelBus<MyPixelColorFeature, Neo800KbpsMethod> (PixelCount, 2);


DisplayPixelsText *pixelText = new DisplayPixelsText();
DisplayPixelsAnimatedGIF *pixelGIF = new DisplayPixelsAnimatedGIF();
DisplayPixelsLive * pixelLive = new DisplayPixelsLive();
DisplayPixels *curPixel = pixelText;



uint16_t ourLayoutMapCallback(int16_t x, int16_t y)
{

  return mosaic.Map(x, y);

}


const uint16_t AnimCount = 1; // we only need one

//String message = "Hello World";


NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object

// our NeoBitmapFile will use the same color feature as NeoPixelBus and
// we want it to use the SD File object
NeoBitmapFileAl<MyPixelColorFeature, File> image;
RgbColor red(128, 0, 0);
RgbColor green(0, 128, 0);
RgbColor blue(0, 0, 128);
RgbColor white(128);
// if using NeoRgbwFeature above, use this white instead to use
// the correct white element of the LED
//RgbwColor white(128);
RgbColor black(0);

ESP8266WebServer server(80);

void updateScreenCallbackS()
{
  server.handleClient();
}

//holds the current upload
File fsUploadFile;

//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".bmp")) return "image/x-windows-bmp";
  return "text/plain";
}


bool handleClearScreen()
{
  pixelLive->Clear();
  curPixel = pixelLive;
  
  server.send(200, "text/plain", "");
  return true;
}

unsigned long hex2int(char *a, unsigned int len)
{
   int i;
   unsigned long val = 0;

   for(i=0;i<len;i++)
      if(a[i] <= 57)
       val += (a[i]-48)*(1<<(4*(len-1-i)));
      else
       val += (a[i]-55)*(1<<(4*(len-1-i)));
   return val;
}
bool handleSetPixels()
{
 /*
  int numArgs = server.args();
  for (int i=0;i<numArgs;i++)
  {
     DBG_OUTPUT_PORT.println("arg: " + server.argName(i) + " val:" +server.arg(i));
  }
*/
  String pixels = server.arg("pixels");
  pixels.toUpperCase();
  int pos=0;
  // let's pull the hex values out
  for (int y=0;y<8;y++)
  {
    for (int x=0;x<8;x++)
    {
       char hr[4];
       hr[0]=pixels[pos++];
       hr[1]=pixels[pos++];
       hr[2]=0;
       int r = hex2int(hr,2);
       char hg[4];       
       hg[0]=pixels[pos++];
       hg[1]=pixels[pos++];
       hg[2]=0;
       int g = hex2int(hg,2);
       char hb[4];
       hb[0]=pixels[pos++];
       hb[1]=pixels[pos++];
       hb[2]=0;
       int b = hex2int(hb,2);
       /*
       DBG_OUTPUT_PORT.print("setpixels: ");
       DBG_OUTPUT_PORT.print(x);
       DBG_OUTPUT_PORT.print(" ");
       DBG_OUTPUT_PORT.print(y);
       DBG_OUTPUT_PORT.print(" (");
       DBG_OUTPUT_PORT.print(hr);
       DBG_OUTPUT_PORT.print(")");
       DBG_OUTPUT_PORT.print(r);
       DBG_OUTPUT_PORT.print(" (");
       DBG_OUTPUT_PORT.print(hg);
       DBG_OUTPUT_PORT.print(")");
       DBG_OUTPUT_PORT.print(g);
       DBG_OUTPUT_PORT.print(" (");
       DBG_OUTPUT_PORT.print(hb);
       DBG_OUTPUT_PORT.print(")");
       DBG_OUTPUT_PORT.println(b);
       */
       
       pixelLive->SetPixel(x,y,r,g,b);
    }
  }
  

  curPixel = pixelLive;
  
  server.send(200, "text/plain", "");
  return true;  
}

bool handleSetPixel()
{
   if (!server.hasArg("x") || !server.hasArg("y") || !server.hasArg("r") || !server.hasArg("g") ||!server.hasArg("b")) {
    server.send(500, "text/plain", "BAD ARGS");
    return false;
  }
  int x = server.arg("x").toInt();
  int y = server.arg("y").toInt();
  int r = server.arg("r").toInt();
  int g = server.arg("g").toInt();
  int b = server.arg("b").toInt();
  pixelLive->SetPixel(x,y,r,g,b);
  curPixel = pixelLive;
  
  server.send(200, "text/plain", "");
  return true;
}

bool handleShowGIF(String path)
{
  DBG_OUTPUT_PORT.println("handleShowGIF: " + path);


  pixelGIF->SetGIF(path);
  curPixel = pixelGIF;

  server.send(200, "text/plain", "");
  return true;
}

bool handleShow(String path) {
  DBG_OUTPUT_PORT.println("handleShow: " + path);
  ESP.wdtFeed();
  File bitmapFile = SPIFFS.open(path, "r");
  if (!bitmapFile)
  {
    Serial.println("File open fail, or not present");
    // don't do anything more:
    return false;
  }
  ESP.wdtFeed();
  ESP.wdtFeed();
  // initialize the image with the file
  if (!image.Begin(bitmapFile))
  {
    Serial.println("File format fail, not a supported bitmap");
    // don't do anything more:
    return false;
  }
  server.send(200, "text/plain", "");
  return true;

}
bool handleFileReadPO(String path) {
String short_name = "";
int found=0;

// read the file from disk that is our index:
File file = SPIFFS.open("/pindex.txt", "r");
String l_line = "";
//open the file here
while (file.available() != 0) {  
    //A inconsistent line length may lead to heap memory fragmentation        
    l_line = file.readStringUntil('\n');        
    if (l_line == "") //no blank lines are anticipated        
      break;      
    //  
    int pipePos = l_line.indexOf("|");
    String long_name = l_line.substring(pipePos+1);
    long_name="/"+long_name;
    short_name = l_line.substring(0, pipePos);
    
   //parse l_line here
   //DBG_OUTPUT_PORT.println("got line: "+l_line);
   //DBG_OUTPUT_PORT.println("["+long_name+"]["+short_name+"]");

   if (path == long_name)
   {
      DBG_OUTPUT_PORT.println("***** FOUND IT ***");
      DBG_OUTPUT_PORT.println("["+long_name+"]["+short_name+"]");
      found = 1;
      short_name="/po/"+short_name;
      break;
   }
}
file.close();

if (found==0) return false;


  String contentType = getContentType(path);
  String pathWithGz = short_name + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(short_name)) {
    if (SPIFFS.exists(pathWithGz))
      short_name += ".gz";
    File file = SPIFFS.open(short_name, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
   


}
bool handleFileRead(String path) {
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);

  if (path.startsWith("/p/")) return handleFileReadPO(path);
  
  if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  if (contentType == "image/x-windows-bmp")
    return handleShow(path);
  else if (contentType == "image/gif")
    return handleShowGIF(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
  if (server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}
void handlePiskelFileUpload() {
  DBG_OUTPUT_PORT.println("Piskel Upload");
HTTPUpload& upload = server.upload();
DBG_OUTPUT_PORT.println(upload.status);
DBG_OUTPUT_PORT.println(upload.filename);
DBG_OUTPUT_PORT.println(upload.type);
DBG_OUTPUT_PORT.println(upload.totalSize);
DBG_OUTPUT_PORT.println(upload.currentSize);

 
  int numArgs = server.args();
  for (int i=0;i<numArgs;i++)
  {
     DBG_OUTPUT_PORT.println("arg: " + server.argName(i) + " val:" +server.arg(i));
  }
  
 server.send(200, "text/plain", "");
}


int findNextFileNumber()
{
  int largestnumber=0;
  Dir dir = SPIFFS.openDir("/piskeldata/");
  while (dir.next()) {
    int pos = dir.fileName().lastIndexOf(".json");
    int slashpos = dir.fileName().lastIndexOf("/");
     DBG_OUTPUT_PORT.println("filename:"+dir.fileName());
     DBG_OUTPUT_PORT.println(pos);
    String filenumberS = dir.fileName().substring(slashpos+1,pos);
   DBG_OUTPUT_PORT.println(filenumberS);
     int filenumber = filenumberS.toInt();
     filenumber++;
    if (filenumber>largestnumber) largestnumber=filenumber;
  }
  DBG_OUTPUT_PORT.print("largestnumber: ");
 DBG_OUTPUT_PORT.println(largestnumber);
  return largestnumber;
}

void handlePiskelSave() {
  DBG_OUTPUT_PORT.println("handlePiskelSave");

  int numArgs = server.args();
  for (int i=0;i<numArgs;i++)
  {
     DBG_OUTPUT_PORT.println("arg: " + server.argName(i) + " val:" +server.arg(i));
  }
  int filenumber = findNextFileNumber();
  DBG_OUTPUT_PORT.println("filenumber:"+filenumber);
  String filename = "/piskeldata/"+ String(filenumber)+".json";

  DBG_OUTPUT_PORT.println("filename ["+filename+"]");
  File file = SPIFFS.open(filename,"w");

  //String result = String("window.pskl.appEnginePiskelData_ = {\r\n \"piskel\" :");
  String result = String("{\r\n \"piskel\" :");
  result += server.arg("framesheet");
  result += ",\r\n";
  result +="\"isLoggedIn\": \"true\",";
  result +="\"fps\":"+server.arg("fps")+",\r\n";
  result +="\"descriptor\" : {";
  result +="\"name\": \""+server.arg("name")+"\",";
  result +="\"description\": \""+server.arg("description")+"\",";
  result +="\"isPublic\": \"false\"";
  result +="}\r\n}";
   DBG_OUTPUT_PORT.println(result);
  file.write((const uint8_t *)result.c_str(),result.length());
  if (file)
    file.close();

  
 server.send(200, "text/plain", "");
}



void handleFileDelete() {
  if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if (server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if (file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}


void showImageList()
{
  Dir dir = SPIFFS.openDir("/");


  String output = "";
  while (dir.next()) {
    File entry = dir.openFile("r");

    String contentType = getContentType(entry.name());
    if (contentType == "image/x-windows-bmp")
    {
      output += String("<a href=\"") + String(entry.name()) + "\">" + String(entry.name()) + String("</a><br/>");
    }
    entry.close();
  }
  server.send(200, "text/html", output);
}

void setScrollText()
{
  if (!server.hasArg("text")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }


  pixelText->SetText(server.arg("text"));

  String output = "Setting scroll to: ";
  output += server.arg("text");

  curPixel = pixelText;

  server.send(200, "text/html", output);

}

void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
}

void setup(void) {

  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);


  strip->Begin();
  strip->Show();

  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }


  //WIFI INIT
  //DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
  //if (String(WiFi.SSID()) != String(ssid)) {
  //  WiFi.begin(ssid, password);
  //}

  //while (WiFi.status() != WL_CONNECTED) {
  //  delay(500);
  //  DBG_OUTPUT_PORT.print(".");
 // }
  //DBG_OUTPUT_PORT.println("");
  //DBG_OUTPUT_PORT.print("Connected! IP address: ");
  //DBG_OUTPUT_PORT.println(WiFi.localIP());

  //MDNS.begin(host);
  //DBG_OUTPUT_PORT.print("Open http://");
  //DBG_OUTPUT_PORT.print(host);
  //DBG_OUTPUT_PORT.println(".local/edit to see the file browser");




  //SERVER INIT
  setupCaptive(&server);

  server.on("/setpixels",HTTP_POST,handleSetPixels);

  server.on("/clearscreen",HTTP_GET,handleClearScreen);
  server.on("/setpixel", HTTP_GET, handleSetPixel);
  server.on("/scroll", HTTP_GET, setScrollText);
  server.on("/image", HTTP_GET, showImageList);
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  server.on("/piskelupload",HTTP_POST,  handlePiskelFileUpload, handlePiskelFileUpload);
  server.on("/piskelsave",HTTP_POST,  handlePiskelSave);
  
//handleShowGIF("/pac1/out8.gif");
  
  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
      if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

}


void loop(void) {
  loopCaptive(); 
  server.handleClient();
  MDNS.update();
  if (curPixel) curPixel->UpdateAnimation();
  //image.Blt(*strip,0,0,0,0,8,8,ourLayoutMapCallback);


  //strip.SetPixelColor(mosaic.Map(0, 0), red);
  //strip.Show();
  strip->Show();

}



