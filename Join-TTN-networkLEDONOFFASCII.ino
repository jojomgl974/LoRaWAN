#include <SoftwareSerial.h>

/*

*/

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;     // whether the string is complete

long old_time=millis();
long new_time;

long uplink_interval=10000;

bool time_to_at_recvb=false;
bool get_LA66_data_status=false;

bool network_joined_status=false;

SoftwareSerial ss(10, 11); // Arduino RX, TX ,

char rxbuff[128];
uint8_t rxbuff_index=0;

void setup() {
  // initialize serial
  Serial.begin(9600);

  ss.begin(9600);
  ss.listen();
  
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);

  ss.println("ATZ");//reset LA66

  pinMode(13, OUTPUT);  // Déclare la LED intégrée comme sortie
  digitalWrite(13, LOW); // Éteint la LED au démarrage
}

// Fonction pour convertir hex -> ASCII
void hexToAscii(const char* hex, char* ascii) {
  int len = strlen(hex);
  int j = 0;

  for (int i = 0; i < len; i += 2) {
    char byteStr[3] = { hex[i], hex[i + 1], '\0' };
    ascii[j++] = (char) strtol(byteStr, NULL, 16);
  }
  ascii[j] = '\0';
}

void loop() {

  new_time = millis();

  if((new_time-old_time>=uplink_interval)&&(network_joined_status==1)){
    old_time = new_time;
    get_LA66_data_status=false;
    ss.println("AT+SENDB=0,2,4,4A4F4A4F");//confirm status,Fport,payload length,payload(HEX)
  }

  if(time_to_at_recvb==true){
    time_to_at_recvb=false;
    get_LA66_data_status=true;
    delay(1000);
    
    ss.println("AT+CFG");
  }

    while ( ss.available()) {
    // get the new byte:
    char inChar = (char) ss.read();
    // add it to the inputString:
    inputString += inChar;

    rxbuff[rxbuff_index++]=inChar;

    if(rxbuff_index>128)
    break;
    
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n' || inChar == '\r') {
      stringComplete = true;
      rxbuff[rxbuff_index]='\0';
      
      if(strncmp(rxbuff,"JOINED",6)==0){
        network_joined_status=1;
      }

      if(strncmp(rxbuff,"Dragino LA66 Device",19)==0){
        network_joined_status=0;
      }

      if(strncmp(rxbuff,"Run AT+RECVB=? to see detail",28)==0){
        time_to_at_recvb=true;
        stringComplete=false;
        inputString = "\0";
      }

      if (strncmp(rxbuff, "AT+RECVB=", 9) == 0) {
        stringComplete = false;
        inputString = "\0";
        
        char* data = &rxbuff[9];   // après AT+RECVB=
        Serial.print("\nFPort & Payload (HEX): ");
        Serial.println(data);
        // Séparer FPort et payload
        char* payloadHex = strchr(data, ':');
        if (payloadHex != NULL) {
          payloadHex++;  // sauter le ':'
          
          char payloadAscii[50];
          hexToAscii(payloadHex, payloadAscii);
          
          Serial.print("Payload HEX   : ");
          Serial.println(payloadHex);
          Serial.print("Payload ASCII : ");
          Serial.println(payloadAscii);
        }
        
        // LIGHTON -> LED ON
        if (strstr(&rxbuff[9], "4c494748544f4e") != NULL) {
          digitalWrite(13, HIGH);
          Serial.println("LED ON");
        }
        
        // LIGHTOFF\n -> LED OFF
        if (strstr(&rxbuff[9], "4c494748544f4646") != NULL) {
          digitalWrite(13, LOW);
          Serial.println("LED OFF");
        }
      }
      
      rxbuff_index=0;

      if(get_LA66_data_status==true){
        stringComplete=false;
        inputString = "\0";
      }
    }
  }
  

   while ( Serial.available()) {
    // get the new byte:
    char inChar = (char) Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n' || inChar == '\r') {
      ss.print(inputString);
      inputString = "\0";
    }
  }
  
  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.print(inputString);
    
    // clear the string:
    inputString = "\0";
    stringComplete = false;
  }
}
