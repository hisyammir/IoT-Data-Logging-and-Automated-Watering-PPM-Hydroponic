 #include <ESP8266WiFi.h>
 #include <ESP8266HTTPClient.h>
 //tds meter output,connect to Arduino controller ADC pin
#define VREF          3.3// analog reference voltage(Volt) of the ADC
#define SCOUNT        30  // sum of sample point
#define relay D0

#include "DHT.h"
#define DHTPIN D1 // what digital pin we're connected to
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11 // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321
WiFiServer server(80);
WiFiClient client;
HTTPClient http;

//sesuikan posisi pin select
int s0 = D4;
int s1 = D3;
int s2 = D2;

//gunakan A0 sebagai input
int analogPin = A0;

float minppm;
int solenoidNyala = LOW;
int solenoidMati = HIGH;

 int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0, temperature = 27;
unsigned long int analogSampleTimepoint,printTimepoint;

 int analogBuffer2[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp2[SCOUNT];
int analogBufferIndex2 = 0,copyIndex2 = 0;
float averageVoltage2 = 0,tdsValue2 = 0;
unsigned long int analogSampleTimepoint2,printTimepoint2, sendTimepoint;

//gunakan A0 sebagai input
//int analogPin = A0;
  
//variabel untuk menyimpan nilai input
int nilaiInput = 0;

const char* ssid     = "uhuy"; //nama wifi
const char* password = "wifikere"; //password
const char* host = "192.168.43.35"; //IP PC
String get_host = "192.168.43.35";

String rcv=""; //simpan string minppm

DHT dht(DHTPIN, DHTTYPE);


void setup() {
  //jadikan pin select sebagai output
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  
  //aktifkan komunikasi serial
  Serial.begin(9600);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, solenoidMati);

  Serial.println("DHTxx test!");
  dht.begin();

  delay(10);
  Serial.println('\n');
  
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
  //starting the server
    server.begin();
    
     if (!client.connect(host, 80)) {
          Serial.println("connection failed");
          return;
        }
            client.print(String("GET ") + "/script-to-database-pgsql/get_pengaliran.php" + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");
        String minppmstring;
         while (client.connected()) {
          minppmstring = client.readStringUntil('\r');
//          Serial.print(line);
          if (minppmstring == "\r") {
//              Serial.println("headers received");
              break;
            }
        }

          float minppm = minppmstring.toFloat();
          Serial.println("Min PPM : ");    
           Serial.println(minppm);     
}
  
void loop() {
  //Pembacaan TDS Sensor di Bak Penampungan Nutrisi
  //memilih y0 sebagai input
  digitalWrite(s0,LOW);
  digitalWrite(s1,LOW);
  digitalWrite(s2,LOW);

  if(millis()-analogSampleTimepoint>40)     //read the analog value from the ADC
 {
   analogSampleTimepoint=millis();
   analogBuffer[analogBufferIndex]=analogRead(analogPin);    //read the analog value and store into the buffer
   analogBufferIndex++;
   if(analogBufferIndex==SCOUNT) analogBufferIndex=0;
 }   

 if(millis()-printTimepoint>1000)
 {  
    printTimepoint=millis();
//    temperature = dht.readTemperature();
    for(copyIndex=0;copyIndex<SCOUNT;copyIndex++) analogBufferTemp[copyIndex]=analogBuffer[copyIndex];
    averageVoltage=getMedianNum(analogBufferTemp,SCOUNT)*(float)VREF/1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
    tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge-255.86*compensationVolatge*compensationVolatge+857.39*compensationVolatge)*0.5; //convert voltage value to tds value
    
    Serial.print("TDS Value1:");
    Serial.print(tdsValue,0);
    Serial.println("ppm");
    //Control Solenoid
      if (tdsValue > minppm){
    digitalWrite(relay, solenoidNyala);
    Serial.print("Buka – dibawah min ppm \n");
    }else {
    digitalWrite(relay, solenoidMati);
    Serial.print("tutup – diatas min ppm \n");
      };
    //END Control Solenoid  
  } 
//     Serial.print(" ");

  //Pembacaan TDS Sensor di Pipa Akhir Pengaliran Nutrisi
  //  memilih y1 sebagai input
  digitalWrite(s0,HIGH);
  digitalWrite(s1,LOW);
  digitalWrite(s2,LOW);
  if(millis()-analogSampleTimepoint2>40)     //read the analog value from the ADC
 {
   analogSampleTimepoint2=millis();
   analogBuffer2[analogBufferIndex2]=analogRead(analogPin);    //read the analog value and store into the buffer
   analogBufferIndex2++;
   if(analogBufferIndex2==SCOUNT) analogBufferIndex2=0;
 }   

 if(millis()-printTimepoint2>1000)
 {
    printTimepoint2=millis();
    for(copyIndex2=0;copyIndex2<SCOUNT;copyIndex2++) analogBufferTemp2[copyIndex2]=analogBuffer2[copyIndex2];
    averageVoltage2=getMedianNum(analogBufferTemp2,SCOUNT)*(float)VREF/1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient2=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge2=averageVoltage2/compensationCoefficient2;  //temperature compensation
    tdsValue2=(133.42*compensationVolatge2*compensationVolatge2*compensationVolatge2-255.86*compensationVolatge2*compensationVolatge2+857.39*compensationVolatge2)*0.5; //convert voltage value to tds value
    
    Serial.print("TDS Value2:");
    Serial.print(tdsValue2,0);
    Serial.println("ppm");
  
    if(millis()-sendTimepoint>360000)//POST Data PPM ke server
    {
        sendTimepoint=millis();    
        Serial.print("connecting to ");
        Serial.println(host);

        const int httpPort = 80;
        if (!client.connect(host, httpPort)) {
          Serial.println("connection failed");
          return;
        }
       // We now create a URI for the request
      String url = "/script-to-database-pgsql/add.php?";

      url += "ppm=";
      url += tdsValue;
      url += "&ppm2=";
      url += tdsValue2;
  
      Serial.print("Requesting URL: ");
      Serial.println(url);
  
    //  // This will send the request to the server
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");
  
      unsigned long timeout = millis();
      while (client.available() == 0) {
        if (millis() - timeout > 420000) {
          Serial.println(">>> Client Timeout !");
          client.stop();
          return;
        }
     }
    } //END of POST DATA PPM KE SERVER
 }
}

int getMedianNum(int bArray[], int iFilterLen) 
{
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) 
  {
    for (i = 0; i < iFilterLen - j - 1; i++) 
    {
      if (bTab[i] > bTab[i + 1]) 
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if((iFilterLen&1)>0) bTemp = bTab[(iFilterLen - 1) / 2];
  else bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
