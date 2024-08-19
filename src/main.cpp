#pragma GCC optimize("O3") // code optimisation controls - "O2" & "O3" code performance, "Os" code size

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <FastLED.h>
#include <WiFi.h>
#include <Arduino.h>
#include <FirebaseESP32.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

//#define WIFI_SSID "NETLIFE-AVILAMARIA"
//#define WIFI_PASSWORD "Jeffry4512"
//#define WIFI_SSID "Wifi22"
//#define WIFI_PASSWORD "Jeffry4512"
#define WIFI_SSID "Microcontroladores"
#define WIFI_PASSWORD "raspy123"
/* 2. Define the API Key */
#define API_KEY "AIzaSyASDX2eUq3Nqk4UgZ3ue4t4QUeNlIAzQjw"

/* 3. Define the RTDB URL */
#define DATABASE_URL "fir-pst-2024-default-rtdb.firebaseio.com" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "jeffaprende@gmail.com"
#define USER_PASSWORD "jeffry4512"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

#define COLUMS 20             // LCD columns
#define ROWS 4                // LCD rows
#define LCD_SPACE_SYMBOL 0x20 // space symbol from LCD ROM, see p.9 of GDM2004D datasheet

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A00, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE); // Cambiado a PCF8574_ADDR_A21_A11_A00

// fastled
#define LED_PIN 5
#define NUM_LEDS 16
// #define BRIGHTNESS 64
#define BRIGHTNESS 16
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// Pin del pulsador
// #define BUTTON_PIN 32
#define BUTTON_PIN 27
// Pines del joystick
#define JOYSTICK_X_PIN 34
#define JOYSTICK_Y_PIN 35
#define JOYSTICK_BUTTON_PIN 26

// OJO 2048 es el punto medio de la entrada analogica PERO esto CAMBIA EN CADA JOYSTICK
#define THRESHOLD_LOW 100   // Limite Valor cercano a 0.
#define THRESHOLD_HIGH 4000 // Limite Valor cercano a 4095.
#define NUM_READINGS 10     // Número de lecturas para promediar

// Variables para controlar el menú
int currentSelection = 0;
const int maxSelection = 2; // Hay tres opciones: 0, 1 y 2
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 200; // Tiempo de debounce

// Variables para almacenar los jugadores y puntajes
char *players[100];
int listPuntajes[100];
int numPlayers = 0;

CRGB ledColors[NUM_LEDS]; // Almacena los colores iniciales de los LEDs

void listPlayers()
{
  if (Firebase.ready())
  {
    Serial.println("Recuperando lista de jugadores...");
    if (Firebase.getJSON(fbdo, F("/jugadores")))
    {
      FirebaseJson &json = fbdo.jsonObject();
      size_t len = json.iteratorBegin();
      String key, value;
      int type;

      // Limpiar las listas
      for (int i = 0; i < numPlayers; i++)
      {
        delete[] players[i];
      }
      numPlayers = 0;

      Serial.println("Jugadores y Puntajes:");
      for (size_t i = 0; i < len && numPlayers < 100; i++)
      {
        json.iteratorGet(i, type, key, value);
        if (type == FirebaseJson::JSON_OBJECT)
        {
          FirebaseJson jsonObj;
          jsonObj.setJsonData(value);
          FirebaseJsonData jsonData;

          if (jsonObj.get(jsonData, "nombre"))
          {
            String nombre = jsonData.stringValue;
            if (jsonObj.get(jsonData, "puntaje"))
            {
              int puntaje = jsonData.intValue;

              // Guardar el nombre y puntaje en las listas
              players[numPlayers] = new char[nombre.length() + 1];
              strcpy(players[numPlayers], nombre.c_str());
              listPuntajes[numPlayers] = puntaje;
              numPlayers++;

              // Imprimir el nombre y puntaje en el monitor serial
              Serial.print("Jugador: ");
              Serial.print(nombre);
              Serial.print(", Puntaje: ");
              Serial.println(puntaje);
            }
          }
        }
      }
      json.iteratorEnd();
    }
    else
    {
      Serial.println("No se pudo recuperar la lista de jugadores.");
      Serial.println(fbdo.errorReason());
    }
  }
}

void setup()
{
  Serial.begin(115200);
  
  /*while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }*/
  

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  IPAddress wfi= WiFi.localIP();
  Serial.println(wfi);
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  Serial.println("Connected to WiFi");

  // Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer needs to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // Or use legacy authenticate method
  // config.database_url = DATABASE_URL;
  // config.signer.tokens.legacy_token = "<database secret>";

  // To connect without auth in Test Mode, see Authentications/TestMode/TestMode.ino

  //////////////////////////////////////////////////////////////////////////////////////////////
  // Please make sure the device free Heap is not lower than 80 k for ESP32 and 10 k for ESP8266,
  // otherwise the SSL connection will fail.
  //////////////////////////////////////////////////////////////////////////////////////////////

  Firebase.begin(&config, &auth);

  Firebase.setDoubleDigits(5);

  // You can use TCP KeepAlive in FirebaseData object and tracking the server connection status, please read this for detail.
  // https://github.com/mobizt/Firebase-ESP32#about-firebasedata-object
  // fbdo.keepAlive(5, 5, 1);

  while (lcd.begin(COLUMS, ROWS, LCD_5x8DOTS) != 1) // colums, rows, characters size
  {
    Serial.println(F("PCF8574 is not connected or lcd pins declaration is wrong. Only pins numbers: 4,5,6,16,11,12,13,14 are legal."));
    delay(5000);
  }

  lcd.print(F("PCF8574 is OK...")); //(F()) saves string to flash & keeps dynamic memory free
  delay(1000);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  lcd.clear();
  // Mostrar mensajes iniciales
  lcd.setCursor(0, 0);
  lcd.print("Bienvenido Jugador!!!");
  lcd.setCursor(0, 1);
  lcd.print("Pulsa Start para ");
  lcd.setCursor(0, 2);
  lcd.print("entrar al Menu :D");

  // Configurar el pin del pulsador como entrada con pull-down
  // pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Configurar los pines del joystick
  pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP); // Botón con pull-up

  // OJO
  listPlayers();
}

void displayMenu(int selection)
{
  // Limpiar la pantalla del LCD
  lcd.clear();

  // Mostrar el menú con la selección actual
  lcd.setCursor(0, 0);
  lcd.print(selection == 0 ? "> 1. Jugar!!!" : "1. Jugar!!!");

  lcd.setCursor(0, 1);
  lcd.print(selection == 1 ? "> 2. Ver puntaje O_o" : "2. Ver puntaje O_o");

  lcd.setCursor(0, 2);
  lcd.print(selection == 2 ? "> 3. Ingresar Jugador" : "3. Ingresar Jugador");

  lcd.setCursor(0, 3);
  lcd.print("   Nuevo");
}

void displayPlayerSelection(int start, int playerSelection)
{

  // Limpiar la pantalla del LCD
  lcd.clear();

  // Mostrar el menú de selección de jugador
  lcd.setCursor(0, 0);
  lcd.print("Use el Joystick para");
  lcd.setCursor(0, 1);
  lcd.print("seleccionar jugador");

  lcd.setCursor(0, 2);
  lcd.print(playerSelection == start ? "> " : "  ");
  lcd.print(players[start]);

  // Si hay otro jugador después del actual (start + 1), muestra el nombre de ese jugador en la cuarta línea del LCD
  if (start + 1 < numPlayers)
  {
    lcd.setCursor(0, 3);
    lcd.print(playerSelection == start + 1 ? "> " : "  ");
    lcd.print(players[start + 1]);
  }
}

int readJoystick(int pin)
{
  long total = 0;
  for (int i = 0; i < NUM_READINGS; i++)
  {
    total += analogRead(pin);
    delay(5); // Pequeño retraso entre lecturas
  }
  return total / NUM_READINGS;
}

void cambiarPuntajePlayer(String nombrePlayer, int nuevoPuntaje)
{
  if (Firebase.ready())
  {
    String path = "/jugadores/" + nombrePlayer + "/puntaje";
    Serial.println(Firebase.setInt(fbdo, F(path.c_str()), nuevoPuntaje) ? "ok" : fbdo.errorReason().c_str());
    Serial.println(path);
  }
  else
  {
    Serial.println("Firebase no está listo.");
  }
}

void matrizLedsMemoriaGame(String nombrePlayer, int Puntaje)
{

  String playerAtual = nombrePlayer;
  int playerPuntajeAtual = Puntaje;
  // Definir matriz de posiciones
  int MatrizPosiciones[16] = {
      0, 1, 2, 3,
      7, 6, 5, 4,
      8, 9, 10, 11,
      15, 14, 13, 12};

  // Definir colores
  CRGB colors[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Purple, CRGB::Orange, CRGB::Brown, CRGB::Orchid};
  int numPairs = NUM_LEDS / 2;
  int positions[NUM_LEDS];

  // Inicializar posiciones
  for (int i = 0; i < NUM_LEDS; i++)
  {
    positions[i] = MatrizPosiciones[i];
  }

  // Mezclar posiciones
  for (int i = 0; i < NUM_LEDS; i++)
  {
    int r = random(NUM_LEDS);
    int temp = positions[i];
    positions[i] = positions[r];
    positions[r] = temp;
  }

  // Asignar colores a posiciones
  for (int i = 0; i < numPairs; i++)
  {
    ledColors[positions[i * 2]] = colors[i];
    ledColors[positions[i * 2 + 1]] = colors[i];
  }

  // Encender LEDs por 10 segundos
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = ledColors[i];
  }
  FastLED.show();
  delay(10000);

  // Apagar LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Encender un LED blanco en la posición 0 del row 1
  int currentPos = 0;
  leds[MatrizPosiciones[currentPos]] = CRGB::White;
  FastLED.show();

  bool revealedPositions[NUM_LEDS] = {false}; // Array para rastrear posiciones reveladas
  int revealedCount = 0;                      // Contador de posiciones reveladas
  int firstRevealedPos = -1;                  // Almacenar la primera posición revelada
  int secondRevealedPos = -1;                 // Almacenar la segunda posición revelada
  int totalRevealed = 0;
  int puntos = 0;

  int banderaMantenerColor = 1;

  // Movimiento del LED blanco con joystick
  while (true)
  {
    int xValue = readJoystick(JOYSTICK_X_PIN);
    int yValue = readJoystick(JOYSTICK_Y_PIN);
    int newPos = currentPos;

    // Mover a la derecha
    if (xValue < THRESHOLD_LOW && (currentPos % 4 != 3))
    {
      newPos = currentPos + 1;
    }
    // Mover a la izquierda
    else if (xValue > THRESHOLD_HIGH && (currentPos % 4 != 0))
    {
      newPos = currentPos - 1;
    }
    // Mover hacia abajo
    else if (yValue > 3500 && (currentPos < 12))
    {
      newPos = currentPos + 4;
    }
    // Mover hacia arriba
    else if (yValue < THRESHOLD_LOW && (currentPos > 3))
    {
      newPos = currentPos - 4;
    }

    if (revealedPositions[newPos])
    {
      banderaMantenerColor = 0;
      // La nueva posición es igual a una posición previamente revelada
      // Aquí se puede agregar la lógica deseada, por ejemplo, mostrar un mensaje o tomar una acción específica
      // Serial.println("La nueva posición es igual a una posición previamente revelada.");
    }

    // Actualizar posición si cambió
    if (newPos != currentPos)
    {
      if (!revealedPositions[currentPos])
      {
        leds[MatrizPosiciones[currentPos]] = CRGB::Black; // Apagar el LED anterior si no está revelado
      }
      else if (banderaMantenerColor == 0 && ledColors[MatrizPosiciones[newPos]] != leds[MatrizPosiciones[currentPos]])
      {
        leds[MatrizPosiciones[currentPos]] = ledColors[MatrizPosiciones[currentPos]];
        delay(10);
        banderaMantenerColor = 1;
      }

      currentPos = newPos;
      delay(10);
      leds[MatrizPosiciones[currentPos]] = CRGB::White; // Encender el nuevo LED blanco
      FastLED.show();
      delay(50); // Pequeño retraso para evitar lecturas rápidas

      /*
      leds[MatrizPosiciones[currentPos]] = ledColors[MatrizPosiciones[currentPos]];
        FastLED.show();
        */
    }

    // Verificar si se presionó el botón
    // AQUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUIIIIIIIII
    if (digitalRead(JOYSTICK_BUTTON_PIN) == LOW && !revealedPositions[newPos])
    {
      // Encender el LED con el color previamente mostrado si hay uno
      if (ledColors[MatrizPosiciones[currentPos]] != CRGB::Black)
      {
        leds[MatrizPosiciones[currentPos]] = ledColors[MatrizPosiciones[currentPos]];
        revealedPositions[currentPos] = true; // Marcar posición como revelada
        FastLED.show();                       // Mantener este color encendido

        // Almacenar la posición revelada
        if (revealedCount == 0)
        {
          firstRevealedPos = currentPos;
          revealedCount++;
        }
        else if (revealedCount == 1)
        {
          secondRevealedPos = currentPos;
          revealedCount++;

          // Verificar si los colores son iguales
          if (ledColors[MatrizPosiciones[firstRevealedPos]] == ledColors[MatrizPosiciones[secondRevealedPos]])
          {
            // Colores iguales, mantenerlos encendidos
            totalRevealed += 2;
            puntos += 1;
            revealedCount = 0; // Reiniciar contador para la siguiente comparación
          }
          else
          {
            // Colores diferentes, apagarlos después de un breve retraso
            delay(1000);
            leds[MatrizPosiciones[firstRevealedPos]] = CRGB::Black;
            leds[MatrizPosiciones[secondRevealedPos]] = CRGB::Black;
            revealedPositions[firstRevealedPos] = false;
            revealedPositions[secondRevealedPos] = false;
            revealedCount = 0; // Reiniciar contador para la siguiente comparación
            firstRevealedPos = -1;
            secondRevealedPos = -1;
            puntos -= 1;
            FastLED.show();
          }
        }
      }
      // Esperar a que se suelte el botón para evitar múltiples lecturas
      while (digitalRead(JOYSTICK_BUTTON_PIN) == LOW)
      {
        delay(10);
      }
    }

    // Verificar si todas las posiciones han sido reveladas
    if (totalRevealed == NUM_LEDS)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Bien Acabaste");

      if (puntos <= 0)
      {
        lcd.setCursor(0, 1);
        lcd.print("Equivodas>Aciertos");
        lcd.setCursor(0, 2);
        lcd.print("Te ira mejor la sgt");
        lcd.setCursor(0, 3);
        lcd.print("Sumaste 1 pts");
        puntos = 1;
      }
      else
      {
        lcd.setCursor(0, 1);
        lcd.print("Eres Genial ");
        lcd.setCursor(0, 2);
        lcd.print("Sumaste estos pts:");
        lcd.setCursor(0, 3);
        lcd.print(puntos);
        lcd.setCursor(3, 3);
        lcd.print(",Guardando pts");
      }
      playerPuntajeAtual = playerPuntajeAtual + puntos;

      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      String path = "/jugadores/" + playerAtual + "/puntaje";
      Serial.println(path);
      Serial.println(Firebase.setInt(fbdo, F(path.c_str()), playerPuntajeAtual) ? "ok" : fbdo.errorReason().c_str());
      cambiarPuntajePlayer(playerAtual, playerPuntajeAtual);
      break;
      delay(5000);
    }
    delay(50); // Pequeño retraso para evitar lecturas rápidas
  }
}

void jugar()
{
  int playerSelection = 0;
  int start = 0;

  displayPlayerSelection(start, playerSelection);

  while (true)
  {
    int yValue = analogRead(JOYSTICK_Y_PIN);
    int joystickButtonState = digitalRead(JOYSTICK_BUTTON_PIN);

    // Movimiento hacia arriba
    if (yValue < 1000 && (millis() - lastDebounceTime) > debounceDelay)
    {
      lastDebounceTime = millis();
      // Si el jugador seleccionado no es el primero en la lista
      if (playerSelection > 0)
      {
        playerSelection--; // Decrementa el índice del jugador seleccionado
        // Si el jugador seleccionado es menor que el inicio de la lista mostrada
        if (playerSelection < start)
        {
          start--; // Decrementa el índice de inicio para desplazar la lista hacia arriba.
        }
        displayPlayerSelection(start, playerSelection);
      }
    }

    // Movimiento hacia abajo
    if (yValue > 3000 && (millis() - lastDebounceTime) > debounceDelay)
    {
      lastDebounceTime = millis();
      // Incrementa el índice del jugador seleccionado.
      if (playerSelection < numPlayers - 1)
      {
        playerSelection++; // Incrementa el índice del jugador seleccionado
        // Si el jugador seleccionado es mayor que el último índice visible en la lista mostrada
        if (playerSelection > start + 1)
        {
          start++; // Incrementa el índice de inicio para desplazar la lista hacia abajo.
        }
        displayPlayerSelection(start, playerSelection);
      }
    }

    // Selección del jugador
    if (joystickButtonState == LOW)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(players[playerSelection]);
      lcd.setCursor(0, 1);
      lcd.print("Hola! tu puntaje es:");
      lcd.setCursor(0, 2);
      lcd.print(listPuntajes[playerSelection]);
      lcd.setCursor(0, 3);
      lcd.print("Vamos a empezar Wuuu");
      delay(5000);
      // Aquí puedes agregar la lógica para iniciar el juego con el jugador seleccionado
      String playerAtual = players[playerSelection];
      int playerPuntajeAtual = listPuntajes[playerSelection];
      matrizLedsMemoriaGame(playerAtual, playerPuntajeAtual);

      break;
    }

    delay(100); // Pequeño retraso para evitar lecturas rápidas
  }
}

void displayPlayerPuntajeSelection(int start, int playerSelection)
{

  // Limpiar la pantalla del LCD
  lcd.clear();

  // Mostrar el menú de selección de jugador
  lcd.setCursor(0, 0);
  lcd.print("Joystick para mover");
  lcd.setCursor(0, 1);
  lcd.print("y ver puntajes");

  // ojito intentar poner puntaje con (-1,2)
  lcd.setCursor(0, 2);
  lcd.print(playerSelection == start ? "> " : "  ");
  lcd.print(players[start]);
  lcd.setCursor(15, 2);
  lcd.print(String(listPuntajes[start]));

  // Si hay otro jugador después del actual (start + 1), muestra el nombre de ese jugador en la cuarta línea del LCD
  if (start + 1 < numPlayers)
  {
    lcd.setCursor(0, 3);
    lcd.print(playerSelection == start + 1 ? "> " : "  ");
    lcd.print(players[start + 1]);
    lcd.setCursor(15, 3);
    lcd.print(String(listPuntajes[start + 1]));
  }
}

void jugarPUNTAJE()
{
  // Limpiar la pantalla del LCD
  lcd.clear();

  // Mostrar el menú de selección de jugador
  lcd.setCursor(0, 0);
  lcd.print("Si despues de ver");
  lcd.setCursor(0, 1);
  lcd.print("los puntajes quieres");
  lcd.setCursor(0, 2);
  lcd.print("volver al menu");
  lcd.setCursor(0, 3);
  lcd.print("Pulsa el JoyStick");
  delay(3000);

  int playerSelection = 0;
  int start = 0;

  displayPlayerPuntajeSelection(start, playerSelection);

  while (true)
  {
    int yValue = analogRead(JOYSTICK_Y_PIN);
    int joystickButtonState = digitalRead(JOYSTICK_BUTTON_PIN);

    // Movimiento hacia arriba
    if (yValue < 1000 && (millis() - lastDebounceTime) > debounceDelay)
    {
      lastDebounceTime = millis();
      // Si el jugador seleccionado no es el primero en la lista
      if (playerSelection > 0)
      {
        playerSelection--; // Decrementa el índice del jugador seleccionado
        // Si el jugador seleccionado es menor que el inicio de la lista mostrada
        if (playerSelection < start)
        {
          start--; // Decrementa el índice de inicio para desplazar la lista hacia arriba.
        }
        displayPlayerPuntajeSelection(start, playerSelection);
      }
    }

    // Movimiento hacia abajo
    if (yValue > 3000 && (millis() - lastDebounceTime) > debounceDelay)
    {
      lastDebounceTime = millis();
      // Incrementa el índice del jugador seleccionado.
      if (playerSelection < numPlayers - 1)
      {
        playerSelection++; // Incrementa el índice del jugador seleccionado
        // Si el jugador seleccionado es mayor que el último índice visible en la lista mostrada
        if (playerSelection > start + 1)
        {
          start++; // Incrementa el índice de inicio para desplazar la lista hacia abajo.
        }
        displayPlayerPuntajeSelection(start, playerSelection);
      }
    }

    // Selección del jugador
    if (joystickButtonState == LOW)
    {
      break;
    }

    delay(100); // Pequeño retraso para evitar lecturas rápidas
  }
}

void ingresarJugador()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Registra a tu :D");
  lcd.setCursor(0, 1);
  lcd.print("Jugador por la APP ");
  delay(2000);
  // Aquí puedes agregar la lógica para ingresar un nuevo jugador
}

void loop()
{

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  if (Firebase.ready())
  {
    // Leer el estado del pulsador
    int buttonState = digitalRead(BUTTON_PIN);

    // Si el pulsador está presionado, mostrar el menú
    // if (buttonState == HIGH)
    if (buttonState == LOW)
    {
      displayMenu(currentSelection);

      // Esperar a que se suelte el botón para evitar múltiples lecturas
      // while (digitalRead(BUTTON_PIN) == HIGH)
      while (digitalRead(BUTTON_PIN) == LOW)
      {
        delay(10); // Pequeño retraso para evitar rebotes
      }

      // Loop principal del menú
      while (true)
      {
        int xValue = analogRead(JOYSTICK_X_PIN);
        int yValue = analogRead(JOYSTICK_Y_PIN);
        int joystickButtonState = digitalRead(JOYSTICK_BUTTON_PIN);

        // Movimiento hacia arriba
        if (yValue < 1000 && (millis() - lastDebounceTime) > debounceDelay)
        {
          lastDebounceTime = millis();
          if (currentSelection > 0)
          {
            currentSelection--;
            displayMenu(currentSelection);
          }
        }

        // Movimiento hacia abajo
        if (yValue > 3000 && (millis() - lastDebounceTime) > debounceDelay)
        {
          lastDebounceTime = millis();
          if (currentSelection < maxSelection)
          {
            currentSelection++;
            displayMenu(currentSelection);
          }
        }

        // Selección del menú
        if (joystickButtonState == LOW)
        {
          // Esperar a que se suelte el botón para evitar múltiples lecturas
          while (digitalRead(JOYSTICK_BUTTON_PIN) == LOW)
          {
            delay(10); // Pequeño retraso para evitar rebotes
          }

          // Ejecutar la lógica para cada selección
          switch (currentSelection)
          {
          case 0:
            listPlayers();
            jugar();
            break;
          case 1:
            listPlayers();
            jugarPUNTAJE();
            break;
          case 2:
            ingresarJugador();
            break;
          }
          // Salir del bucle del menú
          break;
        }

        delay(100); // Pequeño retraso para evitar lecturas rápidas
      }

      // Volver a mostrar el mensaje de bienvenida después de salir del menú
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Bienvenido Jugador!!!");
      lcd.setCursor(0, 1);
      lcd.print("Pulsa Start para ");
      lcd.setCursor(0, 2);
      lcd.print("entrar al Menu :D");
    }
  }
}
