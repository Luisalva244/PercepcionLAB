#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

#define S3 15            /* Define S3 Pin Number of ESP32 */
#define sensorOut 4      /* Define Sensor Output Pin Number of ESP32 */
#define maxAttempts 20 
#define INFRA_ROJO 34
#define INFRA_LED_ROJO 35
#define INFRA_LED_GREEN 39
#define INFRA_LED_BLUE 25
#define ledRojo 13
#define ledGreen 16
#define ledBlue 17
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 33
#define OLED_SCL 32
#define echoPin 12       /* Echo pin for ultrasonic sensor */
#define trigPin 14       /* Trigger pin for ultrasonic sensor */
#define servoPin 27      

const char* ssid = "INFINITUM84AF";
const char* password = "4tPVYEG7FE";

WiFiServer server(80);
MFRC522 rfid(SS, 22); 
MFRC522::MIFARE_Key key;
String header;
String outState = "apagado";

int getRed();
int getGreen();
int getBlue();
//Color getAverage(float &value); es una funcion pero no deja ponerla porque apunta a un enum y alch no se como se pone este pedo xd, una disculpa cesar 
bool checkRFID();
void readSensor(int &Distancia);

String DatoHex;
bool statusRfid = false;
const String UserReg_1 = "4BE6BD11";
const String UserReg_2 = "B33786A3";
const String UserReg_3 = "7762C83B";

long duracion;
int distancia = 0;

bool carro1 = false;
bool rfidDetected = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Servo myServo;

class DisplayManager {
public:
    DisplayManager() {}

    // Método para inicializar el OLED
    void begin() {
        Wire.begin(OLED_SDA, OLED_SCL);
        if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            Serial.println(F("No se pudo encontrar la pantalla OLED."));
            while (true);
        }
        Serial.println(F("Pantalla OLED inicializada correctamente."));
        display.clearDisplay();
    }

    // Método para mostrar dos cadenas en diferentes líneas
    void showText(String line1, String line2) {
        display.clearDisplay();
        display.setTextSize(2);  // Tamaño del texto
        display.setTextColor(SSD1306_WHITE);  // Color del texto

        // Mostrar texto en la primera fila
        display.setCursor(0, 0);
        display.println(line1);

        // Mostrar texto en la segunda fila
        display.setCursor(0, 20);
        display.println(line2);

        display.display();  // Actualizar la pantalla
        delay(20);        // Esperar 2 segundos
    }

    // Método para mostrar una lista de textos
    void showMultipleTexts(String line1, String line2, String line3) {
        display.clearDisplay();
        display.setTextSize(2);  // Tamaño del texto
        display.setTextColor(SSD1306_WHITE);  // Color del texto

        // Mostrar texto en la primera fila
        display.setCursor(0, 0);
        display.println(line1);

        // Mostrar texto en la segunda fila
        display.setCursor(0, 20);
        display.println(line2);

        display.setCursor(0, 40);
        display.println(line3);
        
        display.display();  // Actualizar la pantalla
        delay(20);       
    }
};
DisplayManager displayManager;
/* Define int variables for color detection */
float average = 0;
int Red = 0;
int Green = 0;
int Blue = 0;
int Frequency = 0;
int redCount = 0;
int blueCount = 0;
int greenCount = 0;
int attempt = 0;
int sensor1 = HIGH;
int sensor_led_rojo = HIGH;
int sensor_led_green = HIGH;
int sensor_led_blue = HIGH;


bool rfidPass = false;

enum Color {
    RED,
    BLUE,
    GREEN,
    NONE
} RBG;

void setup() {
    Serial.begin(115200); /* Set the baud rate to 115200 */
    displayManager.begin();
    // Setup ultrasonic sensor pins
    pinMode(ledRojo, OUTPUT);
    pinMode(ledGreen, OUTPUT);
    pinMode(ledBlue, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);


    Serial.print("Conectando a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
  
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    displayManager.showText("Conectando a","Internet");
    }
  server.begin(); 

  Serial.println("");
  Serial.println("Dispositivo conectado.");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
    

    // Setup servo
    myServo.attach(servoPin);  
    myServo.write(160);  

    // Setup RFID
    Serial.println("Iniciando SPI...");
    SPI.begin(SCK, MISO, MOSI, SS);
    Serial.println("SPI Inicializado");

    Serial.println("Iniciando RFID...");
    rfid.PCD_Init();
    delay(50);

    rfid.PCD_DumpVersionToSerial(statusRfid);
    if (statusRfid) {
        Serial.println("RFID ready.");
    } else {
        Serial.println("RFID initialization failed.");
    }

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    // Setup color detection pins
    pinMode(S2, OUTPUT); 
    pinMode(S3, OUTPUT); 
    pinMode(sensorOut, INPUT); 
    pinMode(INFRA_ROJO, INPUT); 
    pinMode(INFRA_LED_ROJO, INPUT); 
   // pinMode(INFRA_LED_GREEN, INPUT); 
    pinMode(INFRA_LED_BLUE, INPUT); 
    Serial.println("TCS3200 Calibration and Color Detection Initialized");
  

}


void loop() 
{
    // Leer distancia con el sensor ultrasónico
    readSensor(distancia);
    htmlPage();
    // Verificar si hay un objeto presente para hacer la lectura de RFID
    if (carro1) 
    {
      Serial.println("Buscando tarjeta RFID...");
      rfidDetected = checkRFID();
      htmlPage();

      //solo es para pruebas "rfidDetected = true;" despues comentar con //
     // rfidDetected = true;
      if (rfidDetected)
      {
        displayManager.showText("Tarjeta","Aceptada");
        Serial.println("Tarjeta RFID detectada, activando servo.");
        myServo.write(0);  
        delay(3000);        
        htmlPage();
        rfidPass = true;
        carro1 = false;
        rfidDetected = false;
      }
    }

    if (rfidPass == true)
    {
       bool redDetected = false;
       bool greenDetected = false;
       bool blueDetected = false;
       bool colorDetected = false;

       bool sensor1Detect = false;
        redCount = 0;
        blueCount = 0;
        greenCount = 0;
        htmlPage();      
        sensor1 = digitalRead(INFRA_ROJO);
        Serial.println(sensor1);
        delay(2000);
        //Solo para pruebas despues borrar o comentar sensor1 = LOW;  
        //sensor1 = LOW;
        if (sensor1 == 1)
        {
          sensor1Detect = true;
           myServo.write(150); 
           Serial.println("Objeto detectado 1");
           htmlPage();
          displayManager.showMultipleTexts("Iniciando","Lectura de","Color");
        } 
        else if (sensor1 == 0)
         {
           
           do
           {
            sensor1 = digitalRead(INFRA_ROJO);
            Serial.println("Esperando objeto");
            displayManager.showText("Esperando","Carro");
            htmlPage();
           } while (sensor1 == 0);
            Serial.println("Objeto detectado 2");
            sensor1Detect = true;
             myServo.write(150); 
            displayManager.showMultipleTexts("Iniciando","Lectura de","Color");
         }
         
        delay(1000);

        if (sensor1Detect) 
        {
            Serial.println("Objeto detectado, iniciando detección de color...");

            // Variable para asegurar que se detecte un color antes de salir
            bool colorDetected = false;
            htmlPage();
            
            //Test case borrar despues de pruebas o comentar
            //colorDetected = true;

            while (!colorDetected)  // Repetir hasta que se detecte un color
            {   
              displayManager.showText("No detecta","Color");
                redCount = 0;
                blueCount = 0;
                greenCount = 0;
                htmlPage();

                for (attempt = 0; attempt < maxAttempts; attempt++) 
                {
                    Red = getRed();
                    delay(20); 
                    Green = getGreen();
                    delay(20); 
                    Blue = getBlue();
                    delay(20); 

                    Color detectedColor = getAverage(average);

                    // Incrementar los contadores de color detectado
                    if (detectedColor == RED) {
                        redCount++;
                    } else if (detectedColor == BLUE) {
                        blueCount++;
                    } else if (detectedColor == GREEN) {
                        greenCount++;
                    }

                    // Imprimir el conteo después de cada intento
                    Serial.printf("Intento %d - ROJO: %d, AZUL: %d, VERDE: %d\n", attempt + 1, redCount, blueCount, greenCount);
                }
                
                // Determinar el color más detectado
                if (redCount > blueCount && redCount > greenCount) 
                {
                  Serial.println("Decisión final: Coche ROJO detectado");
                  redDetected = true;
                  colorDetected = true;
                } 
                else if (blueCount > redCount && blueCount > greenCount)
                {
                  Serial.println("Decisión final: Coche AZUL detectado");
                  blueDetected = true;
                  colorDetected = true;
                } 
                else if (greenCount > redCount && greenCount > blueCount) 
                {
                  Serial.println("Decisión final: Coche VERDE detectado");
                  greenDetected = true;
                  colorDetected = true;
                } 
                else 
                {
                  Serial.println("No se detectó un color claro, repitiendo la detección...");
                  delay(1000);  // Esperar antes de volver a intentar la detección
                }
            }

            if (colorDetected)
            {
              displayManager.showText("Color","Detectado");
              delay(1000);
             // redDetected = true;
              if(redDetected)
              {
                digitalWrite(ledRojo,HIGH); 
                displayManager.showMultipleTexts("Color","Rojo","Detectado");
                Serial.println("Hola color rojo");
                sensor_led_rojo = digitalRead(INFRA_LED_ROJO);
                Serial.println(sensor_led_rojo);
                delay(1000);

                do
                {
                 sensor_led_rojo = digitalRead(INFRA_LED_ROJO);
                 Serial.println("Waiting for car red");
                 /*int i = 0;
                 i++;

                 if(i> 20)
                 {
                  sensor_led_rojo = LOW;
                 }*/
                 } while(sensor_led_rojo != LOW);
                
                if(sensor_led_rojo == LOW)
                {
                  displayManager.showMultipleTexts("Carro","Rojo","LLego");
                  digitalWrite(ledRojo,LOW);
                  Serial.println("Car arrived");
                  delay(1000);
                }
              }

              if(greenDetected)
              {
                
                displayManager.showMultipleTexts("Color","Verde","Detectado");
                digitalWrite(ledGreen,HIGH); 
                Serial.println("Hola color verde");
                sensor_led_green = digitalRead(INFRA_LED_GREEN);
                Serial.println( sensor_led_green);
                delay(2000);
                do
                {
                 sensor_led_green = digitalRead(INFRA_LED_GREEN);
                 Serial.println("Waiting for car green");
                } while(sensor_led_green != LOW);
                
                if(sensor_led_green == LOW)
                {
                  displayManager.showMultipleTexts("Carro","Verde","LLego");
                  digitalWrite(ledGreen,LOW);
                  Serial.println("Car arrived");
                  delay(2000);
                }
              }

              if(blueDetected)
              {
                displayManager.showMultipleTexts("Color","Azul","Detectado");
                digitalWrite(ledBlue,HIGH); 
                Serial.println("Hola color azul");
                sensor_led_blue = digitalRead(INFRA_LED_BLUE);
                Serial.println(sensor_led_blue);
                delay(1000);
                do
                {
                 sensor_led_blue = digitalRead(INFRA_LED_BLUE);
                 Serial.println("Waiting for car blue");
                } while(sensor_led_blue != LOW);
                
                if(sensor_led_blue == LOW)
                {
                  displayManager.showMultipleTexts("Carro","Azul","LLego");
                  digitalWrite(ledBlue,LOW);
                  Serial.println("Car arrived");
                  delay(1000);
                }
              }
          } 
        delay(1000); // Esperar antes de iniciar el siguiente ciclo de detección
      } 
      else 
      {
       Serial.println("No se detectó un objeto para la detección de color.");
       delay(1000);
      }

      Serial.println("rfidPass en false");
      rfidPass = false;
    }
}


// Function to get the frequency for the red component
int getRed() {
    digitalWrite(S2, LOW);
    digitalWrite(S3, LOW);
    Frequency = pulseIn(sensorOut, LOW);
    return Frequency;
}

// Function to get the frequency for the green component
int getGreen() {
    digitalWrite(S2, HIGH);
    digitalWrite(S3, HIGH);
    Frequency = pulseIn(sensorOut, LOW);
    return Frequency;
}

// Function to get the frequency for the blue component
int getBlue() {
    digitalWrite(S2, LOW);
    digitalWrite(S3, HIGH);
    Frequency = pulseIn(sensorOut, LOW);
    return Frequency;
}

// Function to calculate the average and identify the color
Color getAverage(float &value) {
    Serial.println(Red);
    Serial.println(Blue);
    Serial.println(Green);
    value = ((float)Red + (float)Blue + (float)Green) / 3;
    
     
    /*
    if ((Red >= 48 && Red <= 50) && (Blue >= 46 && Blue <= 48) && (Green >= 44 && value <= 46)) 
    {
        RBG = GREEN;
    } else if ((Red >= 40 && Red <= 43) && (Blue >= 45 && Blue <= 48) && (Green >= 60 && value <= 62))
    {
        RBG = RED;
    } 
    else if ((Red >= 55 && Red <= 58) && (Blue >= 34 && Blue <= 37) && (Green >= 44 && value <= 46))
    {
        RBG = BLUE;
    } */
    if ((Red >= 48 && Red <= 50) && (Blue >= 46 && Blue <= 48) && (Green >= 44 && Green <= 50)) 
    {
        RBG = GREEN;
    } else if ((Red >= 40 && Red <= 43) && (Blue >= 45 && Blue <= 48) && (Green >= 60 && Green <= 62))
    {
        RBG = RED;
    } 
    else if ((Red >= 38 && Red <= 58) && (Blue >= 24 && Blue <= 35) && (Green >= 35 && Green <= 45))
    {
        RBG = BLUE;
    } else
    {
        RBG = NONE;  // No clear color detected
    }

    return RBG;  // Return identified color
}

// Function to read distance from the ultrasonic sensor
void readSensor(int &Distancia) {
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duracion = pulseIn(echoPin, HIGH);
    Distancia = duracion * 0.034 / 2;

   /*
   for (int i=0;i<10;i++)
    { 
    Distancia = i;
    String distanciaString = String(Distancia) + " m";
    displayManager.showText("Distancia", distanciaString);
    delay(500);
    }
    */
    String distanciaString = String(Distancia) + " m";
    displayManager.showText("Distancia", distanciaString);
    Serial.println(Distancia);
    //displayManager.showText("Distancia", distanciaString);

    if ((Distancia < 9 && Distancia > 1) && !rfidDetected) {
      carro1 = true;
      Serial.println("Object detected by ultrasonic sensor.");
      displayManager.showText("Distancia", "Optima");
    }

    if (Distancia > 30 && carro1) {
        carro1 = false;
        Serial.println("Object out of range, resetting.");
    }

    delay(100);
}

// Function to check RFID status and read cards
bool checkRFID() {
    if (!statusRfid) {
        rfid.PCD_DumpVersionToSerial(statusRfid);
        if (!statusRfid) {
            Serial.println("Retrying RFID initialization...");
            delay(100);
            return false;  // Return false if RFID initialization fails
        } else {
            Serial.println("RFID ready after retry.");
        }
    }

    byte firmwareVersion = rfid.PCD_ReadRegister(rfid.VersionReg);
    if (firmwareVersion == 0x00 || firmwareVersion == 0xFF) {
        Serial.println("RFID communication failure detected, retrying...");
        statusRfid = false;
        return false;  // Return false if communication fails
    }

    if (statusRfid) {
        if (!rfid.PICC_IsNewCardPresent()) {
            Serial.println("No new card detected.");
            return false;  // Return false if no new card is detected
        }
        Serial.println("New card detected!");

        if (rfid.PICC_ReadCardSerial()) {
            Serial.println("Card detected!");
            DatoHex = "";
            for (byte i = 0; i < rfid.uid.size; i++) {
                DatoHex += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i], HEX);
            }
            DatoHex.toUpperCase();

            Serial.print("Card UID: ");
            Serial.println(DatoHex);

            if (UserReg_1 == DatoHex || UserReg_2 == DatoHex || UserReg_3 == DatoHex) {
                Serial.println("Authorized user detected - Access granted.");
                rfid.PICC_HaltA();  // Halt card communication
                return true;  // Return true if a valid user is detected
            } else {
                Serial.println("Unauthorized user - Access denied.");
            }

            rfid.PICC_HaltA();  // Halt card communication
        } else {
            Serial.println("Failed to read card.");
            statusRfid = false;  // Reset status if reading card fails
        }
    }

    return false;  // Return false if no valid card is detected
}

void htmlPage()
{
   WiFiClient client = server.available();   //El ESP32 siempre está escuchando a los clientes entrantes

  if (client) 
  {                               
    Serial.println("Nuevo cliente.");      
    String currentLine = ""; // Cadena para contener los datos entrantes del cliente          
    while (client.connected()) 
    {
      if (client.available()) 
      {     
        char c = client.read();            
        Serial.write(c);                  
        header += c;

        if (c == '\n') 
        {           
          if (currentLine.length() == 0)
           {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: keep-alive");
            client.println();
            
            if (header.indexOf("GET /LED_BUILTIN/on") >= 0) 
            {
            Serial.println("GPIO LED_BUILTIN encendido");
            outState = "encendido";
            digitalWrite(LED_BUILTIN, HIGH);  // Turn LED on
            myServo.write(0);  
            }

            else if (header.indexOf("GET /LED_BUILTIN/off") >= 0) 
            {
            Serial.println("GPIO LED_BUILTIN apagado");
            outState = "apagado";
            digitalWrite(LED_BUILTIN, LOW);  // Turn LED off
            myServo.write(140);  
            }
            

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta http-equiv=\"refresh\" content=\"1\">"); // Page will refresh every 10 seconds
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}</style></head>");
            client.println("<body><h1>Estacionamiento inteligente</h1>"); // Title
            client.println("<p>Lectura ultrasonico: " + String(distancia) + "</p>"); 
            if (rfidPass == true)
            {
               client.println("<p>Tarjeta de accesso aceptada </p>"); 
            }  // Potentiometer average
            // LED state
            client.println("<p>Dispositivo: " + outState + "</p>"); // Estado del dispositivo
                  
           if (outState == "apagado") {
           client.println("<p><a href=\"/LED_BUILTIN/on\"><button class=\"button\">Encender</button></a></p>");
           } else {
            client.println("<p><a href=\"/LED_BUILTIN/off\"><button class=\"button buttonOff\">Apagar</button></a></p>");
           } 
            client.println("</body></html>");
            client.println();

            break;
          } 
          else
          { 
            currentLine = "";
          }
        } 
        else if (c != '\r')
         {// si tiene algo diferente a un carácter de retorno de carro 
          currentLine += c;      
        }
      }
    }
    
    header = "";  
    client.stop();
    Serial.println("Cliente desconectado.");
    Serial.println("");
  }
}
