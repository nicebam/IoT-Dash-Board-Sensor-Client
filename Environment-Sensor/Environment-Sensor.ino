#include <SPI.h>
#include <WiFi101.h>
#include "DHT.h"
//
// for temperatura
//
#define DHTPIN 12
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);
unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 5L * 1000L; // delay between updates, in milliseconds

//
// for dust
//
unsigned char Send_data[4] = {0x11,0x01,0x01,0xED};       // 읽는명령
unsigned char Receive_Buff[16];                           // data buffer
unsigned long PCS;                                        // 수량 저장 변수 
float ug;                                                 // 농도 저장 변수 
unsigned char recv_cnt = 0;

//
// for network
//
char ssid[] = "your_ssid";      //  your network SSID (name)
char pass[] = "your_password";   // your network password
int status = WL_IDLE_STATUS;
// Initialize the Wifi client library
WiFiClient client;
// server address:
char server[] = "192.168.1.115";
//IPAddress server(192.168.1.115);



void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  
  Serial1.begin(9600);
  /*while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }*/

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  // you're connected now, so print out the status:
  printWifiStatus();
  dht.begin();
}

void loop() {
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  
  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {
    
    sendData();
  }

  delay(1000);
}


// this method makes a HTTP connection to the server:
void sendData() {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(server, 3000)) {
    Serial.println("connecting...");
    float humidity = 0.f;
    float temperature = 0.f;
    float heatidx = 0.f;
    getTemperature(&humidity, &temperature, &heatidx);
    
    String data = "temperature=" + String(temperature) + "&humidity=" + String(humidity);   
  
    // Send request
    client.println("POST /" + String("api/Temperature") + " HTTP/1.1");
    client.println("Host: " + String(server) + ":" + String(3000));
    client.println(F("Content-Type: application/x-www-form-urlencoded"));
    client.print(F("Content-Length: "));
    client.println(data.length());
    client.println();
    client.print(data);

    
    float ug = 0.f;
    getDust(&ug);
    data = "dust=" + String(ug);
    
    client.println("POST /" + String("api/Dust") + " HTTP/1.1");
    client.println("Host: " + String(server) + ":" + String(3000));
    client.println(F("Content-Type: application/x-www-form-urlencoded"));
    client.print(F("Content-Length: "));
    client.println(data.length());
    client.println();
    client.print(data);
  
    // note the time that the connection was made:
    lastConnectionTime = millis();

    
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


//
// for temperatura
//
void getTemperature(float* h, float* t, float* hic) {
   *h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  *t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(*h) || isnan(*t) ) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  // Compute heat index in Celsius (isFahreheit = false)
  *hic = dht.computeHeatIndex(*t, *h, false);
}


//
// for dust
//
void Send_CMD(void)                                        // COMMAND
{
  unsigned char i;
  for(i=0; i<4; i++)
  {
    Serial1.write(Send_data[i]);
    delay(1);      // Don't delete this line !!
  }
}
unsigned char Checksum_cal(void)                          // CHECKSUM 
{
  unsigned char count, SUM=0;
  for(count=0; count<15; count++)
  {
     SUM += Receive_Buff[count];
  }
  return 256-SUM;
}
bool getDust(float* ug){
  Send_CMD();
  while(1)
  {
    { 
       Receive_Buff[recv_cnt++] = Serial1.read();
      if(recv_cnt ==16){recv_cnt = 0; break;}
    }
  } 
  if(Checksum_cal() == Receive_Buff[15])  // CS 확인을 통해 통신 에러 없으면
  {
        PCS = (unsigned long)Receive_Buff[3]<<24 | (unsigned long)Receive_Buff[4]<<16 | (unsigned long)Receive_Buff[5]<<8| (unsigned long)Receive_Buff[6];  // 수량 
        *ug = (float)PCS*3528/100000; // 농도 변환(이 식은 PM1001 모델만 적용됩니다.)
        Serial.write("PCS : ");
        Serial.print(PCS);
        
        Serial.write(",  ug : ");
        Serial.println(*ug);
        return true;
        
   }
   else
   {
     Serial.write("CHECKSUM Error");
     Serial.println();
     return false;
   }
}
