#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <iostream>
#include <sstream>
#include <unordered_map>

struct ServoInfo {
  Servo servo;
  int pin;
  String name;
  int initialPosition;
};

struct RecordedStep {
  int servoIndex;
  int value;
  int delayInStep;
};

std::vector<ServoInfo> servoInfos = {
  {Servo(), 27, "Base", 90},
  {Servo(), 4, "Shoulder", 90},
  {Servo(), 17, "Elbow", 90},
  {Servo(), 33, "Gripper", 90},
  {Servo(), 26, "Rotate", 90},
};

std::vector<RecordedStep> recordedSteps;
bool recordSteps = false;
bool playRecordedSteps = false;
unsigned long previousTimeInMilli = millis();

const char* ssid = "RobotArm";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsRobotArmInput("/RobotArmInput");

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>

    input[type=button]
    {
      background-color:orange;color:white;border-radius:30px;width:100%;height:40px;font-size:20px;text-align:center;
    }
        
    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }

    .slidecontainer {
      width: 100%;
    }

    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 20px;
      border-radius: 5px;
      background: green;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }

    .slider:hover {
      opacity: 1;
    }
  
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 50px;
      height: 50px;
      border-radius: 50%;
      background: orange;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 50px;
      height: 50px;
      border-radius: 50%;
      background: Yellow;
      cursor: pointer;
    }

    </style>
  
  </head>
  <body class="noselect" align="center" style="background-color:#d4d4dc">
     
    <h1 style="color:MediumSeaGreen ;font-size: 30px;text-align:center;">KHWAJA YUNUS ALI UNIVERSITY</h1>
    <h1 style="color: Orange;font-size: 25px;text-align:center;">DEPT. OF MECHATRONIC ENGINEERING</h1>
    <h2 style="color: Teal;font-size: 20px;text-align:center;">Robot Arm Control</h2>
    
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr/><tr/>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Gripper:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Gripper" oninput='sendButtonInput("Gripper",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Rotate:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Rotate" oninput='sendButtonInput("Rotate",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Elbow:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Elbow" oninput='sendButtonInput("Elbow",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Shoulder:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Shoulder" oninput='sendButtonInput("Shoulder",value)'>
          </div>
        </td>
      </tr>  
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Base:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Base" oninput='sendButtonInput("Base",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Record:</b></td>
        <td><input type="button" id="Record" value="OFF" ontouchend='onclickButton(this)'></td>
        <td></td>
      </tr>
      <tr/><tr/> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Play:</b></td>
        <td><input type="button" id="Play" value="OFF" ontouchend='onclickButton(this)'></td>
        <td></td>
      </tr>      
    </table>
  
    <script>
      var webSocketRobotArmInputUrl = "ws:\/\/" + window.location.hostname + "/RobotArmInput";      
      var websocketRobotArmInput;
      
      function initRobotArmInputWebSocket() 
      {
        websocketRobotArmInput = new WebSocket(webSocketRobotArmInputUrl);
        websocketRobotArmInput.onopen    = function(event){};
        websocketRobotArmInput.onclose   = function(event){setTimeout(initRobotArmInputWebSocket, 2000);};
        websocketRobotArmInput.onmessage    = function(event)
        {
          var keyValue = event.data.split(",");
          var button = document.getElementById(keyValue[0]);
          button.value = keyValue[1];
          if (button.id == "Record" || button.id == "Play")
          {
            button.style.backgroundColor = (button.value == "ON" ? "green" : "orange");  
            enableDisableButtonsSliders(button);
          }
        };
      }
      
      function sendButtonInput(key, value) 
      {
        var data = key + "," + value;
        websocketRobotArmInput.send(data);
      }
      
      function onclickButton(button) 
      {
        button.value = (button.value == "ON") ? "OFF" : "ON" ;        
        button.style.backgroundColor = (button.value == "ON" ? "green" : "orange");          
        var value = (button.value == "ON") ? 1 : 0 ;
        sendButtonInput(button.id, value);
        enableDisableButtonsSliders(button);
      }
      
      function enableDisableButtonsSliders(button)
      {
        if(button.id == "Play")
        {
          var disabled = "auto";
          if (button.value == "ON")
          {
            disabled = "none";            
          }
          document.getElementById("Rotate").style.pointerEvents = disabled;
          document.getElementById("Gripper").style.pointerEvents = disabled;
          document.getElementById("Elbow").style.pointerEvents = disabled;          
          document.getElementById("Shoulder").style.pointerEvents = disabled;          
          document.getElementById("Base").style.pointerEvents = disabled; 
          document.getElementById("Record").style.pointerEvents = disabled;
        }
        if(button.id == "Record")
        {
          var disabled = "auto";
          if (button.value == "ON")
          {
            disabled = "none";            
          }
          document.getElementById("Play").style.pointerEvents = disabled;
        }        
      }
           
      window.onload = initRobotArmInputWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });      
    </script>
    <h2 style="color: teal;text-align:center;">Project by: Md. Shafiqul Islam</h2>
  </body>    
</html>
)HTMLHOMEPAGE";

// Function prototypes
void handleRoot(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);
void onRobotArmInputWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void sendCurrentRobotArmState();
void writeServoValues(int servoIndex, int value);
void playRecordedRobotArmSteps();
void setUpPinModes();

void setup(void) {
  setUpPinModes();
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  wsRobotArmInput.onEvent(onRobotArmInputWebSocketEvent);
  server.addHandler(&wsRobotArmInput);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  wsRobotArmInput.cleanupClients();
  if (playRecordedSteps) {
    playRecordedRobotArmSteps();
  }
}

void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "File Not Found");
}

void onRobotArmInputWebSocketEvent(AsyncWebSocket *server,
                                   AsyncWebSocketClient *client,
                                   AwsEventType type,
                                   void *arg,
                                   uint8_t *data,
                                   size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            sendCurrentRobotArmState();
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo *)arg;

            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                std::string dataStr(reinterpret_cast<char *>(data), len);
                std::istringstream ss(dataStr);
                std::string key, value;
                std::getline(ss, key, ',');
                std::getline(ss, value, ',');

                Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str());
                int valueInt = atoi(value.c_str());

                handleWebSocketData(key, valueInt);
            }
            break;
        }
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        default:
            break;
    }
}

void handleWebSocketData(const std::string &key, int valueInt) {
    if (key == "Record") {
        handleRecordCommand(valueInt);
    } else if (key == "Play") {
        handlePlayCommand(valueInt);
    } else {
        handleJointCommand(key, valueInt);
    }
}

void handleRecordCommand(int valueInt) {
    recordSteps = valueInt;
    if (recordSteps) {
        recordedSteps.clear();
        previousTimeInMilli = millis();
    }
}

void handlePlayCommand(int valueInt) {
    playRecordedSteps = valueInt;
}

void handleJointCommand(const std::string &jointName, int valueInt) {
    static const std::unordered_map<std::string, int> jointNameToIndex = {
        {"Base", 0},
        {"Shoulder", 1},
        {"Elbow", 2},
        {"Gripper", 3},
        {"Rotate", 4}
    };

    auto it = jointNameToIndex.find(jointName);
    if (it != jointNameToIndex.end()) {
        writeServoValues(it->second, valueInt);
    }
}


void sendCurrentRobotArmState() {
  for (auto &servoInfo : servoInfos) {
    wsRobotArmInput.textAll(servoInfo.name + "," + servoInfo.servo.read());
  }
  wsRobotArmInput.textAll(String("Record,") + (recordSteps ? "ON" : "OFF"));
  wsRobotArmInput.textAll(String("Play,") + (playRecordedSteps ? "ON" : "OFF"));
}

void writeServoValues(int servoIndex, int value) {
  if (recordSteps) {
    RecordedStep recordedStep;
    if (recordedSteps.empty()) {
      for (auto &servoInfo : servoInfos) {
        recordedStep.servoIndex = &servoInfo - &servoInfos[0];
        recordedStep.value = servoInfo.servo.read();
        recordedStep.delayInStep = 0;
        recordedSteps.push_back(recordedStep);
      }
    }
    unsigned long currentTime = millis();
    recordedStep.servoIndex = servoIndex;
    recordedStep.value = value;
    recordedStep.delayInStep = currentTime - previousTimeInMilli;
    recordedSteps.push_back(recordedStep);
    previousTimeInMilli = currentTime;
  }
  servoInfos[servoIndex].servo.write(value);
}

void playRecordedRobotArmSteps() {
  if (recordedSteps.empty()) {
    return;
  }
  for (int i = 0; i < 4 && playRecordedSteps; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    int currentServoPosition = servoInfos[recordedStep.servoIndex].servo.read();
    while (currentServoPosition != recordedStep.value && playRecordedSteps) {
      currentServoPosition = (currentServoPosition > recordedStep.value ? currentServoPosition - 1 : currentServoPosition + 1);
      servoInfos[recordedStep.servoIndex].servo.write(currentServoPosition);
      wsRobotArmInput.textAll(servoInfos[recordedStep.servoIndex].name + "," + currentServoPosition);
      delay(50);
    }
  }
  delay(2000);

  for (int i = 4; i < recordedSteps.size() && playRecordedSteps; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    delay(recordedStep.delayInStep);
    servoInfos[recordedStep.servoIndex].servo.write(recordedStep.value);
    wsRobotArmInput.textAll(servoInfos[recordedStep.servoIndex].name + "," + recordedStep.value);
  }
}

void setUpPinModes() {
  for (auto &servoInfo : servoInfos) {
    servoInfo.servo.attach(servoInfo.pin);
    servoInfo.servo.write(servoInfo.initialPosition);
  }
}



