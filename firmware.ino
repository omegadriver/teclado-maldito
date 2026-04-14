#include <WiFi.h>
#include <WebSocketsClient.h>
#include <USB.h>
#include <USBHIDKeyboard.h>


// ── configuração ─────────────────────────────────────────────────────

const char* WIFI_SSID     = "SSID";
const char* WIFI_PASSWORD = "PASSWORD";

const int BAUND_RATE      = 115200;

const char* WS_HOST       = "HOST";
const int   WS_PORT       = 8765;


WebSocketsClient wsClient;
USBHIDKeyboard Keyboard;


void typeChar(String msg) {

    // CTRL+ALT+<tecla>
    if (msg.startsWith("CTRL+ALT+")) {
        String key = msg.substring(9); // pega o que vem depois de "CTRL+ALT+"
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press(KEY_LEFT_ALT);
        Keyboard.releaseAll();
    }

    // CTRL+SHIFT+<tecla>
    else if (msg.startsWith("CTRL+SHIFT+")) {
        String key = msg.substring(11);
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.releaseAll();
    }

    // WIN+<tecla>
    else if (msg.startsWith("WIN+")) {
        String key = msg.substring(4);
        Keyboard.press(KEY_LEFT_GUI);
        Keyboard.releaseAll();
    }

    // CTRL+<tecla>
    else if (msg.startsWith("CTRL+")) {
        String key = msg.substring(5);
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.releaseAll();
    }

    else if (msg == " ")          { Keyboard.press(' ');                Keyboard.releaseAll(); }
    else if (msg == "ENTER")      { Keyboard.press(KEY_RETURN);         Keyboard.releaseAll(); }
    else if (msg == "TAB")        { Keyboard.press(KEY_TAB);            Keyboard.releaseAll(); }
    else if (msg == "BACKSPACE")  { Keyboard.press(KEY_BACKSPACE);      Keyboard.releaseAll(); }
    else if (msg == "DELETE")     { Keyboard.press(KEY_DELETE);         Keyboard.releaseAll(); }
    else if (msg == "ESC")        { Keyboard.press(KEY_ESC);            Keyboard.releaseAll(); }
    else if (msg == "CAPSLOCK")   { Keyboard.press(KEY_CAPS_LOCK);      Keyboard.releaseAll(); }

    // Arrows
    else if (msg == "UP")         { Keyboard.press(KEY_UP_ARROW);       Keyboard.releaseAll(); }
    else if (msg == "DOWN")       { Keyboard.press(KEY_DOWN_ARROW);     Keyboard.releaseAll(); }
    else if (msg == "LEFT")       { Keyboard.press(KEY_LEFT_ARROW);     Keyboard.releaseAll(); }
    else if (msg == "RIGHT")      { Keyboard.press(KEY_RIGHT_ARROW);    Keyboard.releaseAll(); }

    // F1-F12
    else if (msg == "F1")         { Keyboard.press(KEY_F1);             Keyboard.releaseAll(); }
    else if (msg == "F2")         { Keyboard.press(KEY_F2);             Keyboard.releaseAll(); }
    else if (msg == "F3")         { Keyboard.press(KEY_F3);             Keyboard.releaseAll(); }
    else if (msg == "F4")         { Keyboard.press(KEY_F4);             Keyboard.releaseAll(); }
    else if (msg == "F5")         { Keyboard.press(KEY_F5);             Keyboard.releaseAll(); }
    else if (msg == "F6")         { Keyboard.press(KEY_F6);             Keyboard.releaseAll(); }
    else if (msg == "F7")         { Keyboard.press(KEY_F7);             Keyboard.releaseAll(); }
    else if (msg == "F8")         { Keyboard.press(KEY_F8);             Keyboard.releaseAll(); }
    else if (msg == "F9")         { Keyboard.press(KEY_F9);             Keyboard.releaseAll(); }
    else if (msg == "F10")        { Keyboard.press(KEY_F10);            Keyboard.releaseAll(); }
    else if (msg == "F11")        { Keyboard.press(KEY_F11);            Keyboard.releaseAll(); }
    else if (msg == "F12")        { Keyboard.press(KEY_F12);            Keyboard.releaseAll(); }

    // Modificadores sozinhos — ignorar
    else if (msg == "SHIFT")      { }
    else if (msg == "CTRL")       { }
    else if (msg == "ALT")        { }

    // Caractere normal
    else if (msg.length() == 1)   { Keyboard.press((char)msg[0]); delay(50); Keyboard.releaseAll(); }
}

void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial0.println("[WS] Connected!");
            break;
        case WStype_DISCONNECTED:
            Serial0.println("[WS] Disconnected — retrying...");
            break;
        case WStype_TEXT:
            String msg = String((char*)payload);
            Serial0.printf("[WS] Received: '%s'\n", msg.c_str());
            typeChar(msg);
            break;
    }
}

// ── configurar teclado ────────────────────────────────────────────────

void keyboardSetup() {
    Serial0.println("[HID] Keyboard initializing...");

    USB.VID(0x25A7);
    USB.PID(0xFA23);
    USB.productName("teclado-maldito");
    USB.manufacturerName("Omega Driver");

    Keyboard.begin();
    USB.begin();

    Serial0.println("[HID] Keyboard initialized!");
}

// ── conectar com WiFi ─────────────────────────────────────────────────

void wifiConnect() {
    Serial0.println("[WIFI] Connecting to wifi...");

    WiFi.disconnect(true);  // força parada da tentativa anterior
    delay(100);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial0.print(".");
    }
    Serial0.println();
    Serial0.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
}

// ── conectar com WebSocket ────────────────────────────────────────────

void websocketSetup() {
    Serial0.println("[WS] Setting up WebSocket client...");

    wsClient.begin(WS_HOST, WS_PORT, "/");
    wsClient.onEvent(onWebSocketEvent);
    wsClient.setReconnectInterval(3000);

    Serial0.println("[WS] Setup complete!");
}

// ── setup + loop ──────────────────────────────────────────────────────

void setup() {
    Serial0.begin(BAUND_RATE); delay(1000);
    
    keyboardSetup(); delay(1000); 

    wifiConnect(); delay(1000);

    websocketSetup(); delay(1000);

    Serial0.println("[WS] Connecting to WebSocket server...");
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial0.println("[WIFI] Connection lost — retrying...");
        wsClient.disconnect(); 

        wifiConnect();

        websocketSetup();      
    }
    wsClient.loop();
}
