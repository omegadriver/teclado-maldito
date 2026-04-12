#include <WiFi.h>
#include <WebSocketsClient.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

const char* ws_host  = "";
const int   ws_port  = 8765;
const char* ssid     = "";
const char* password = "";

WebSocketsClient wsClient;
USBHIDKeyboard Keyboard;

void typeChar(String msg) {
    Serial0.printf("[HID] Typing: %s\n", msg.c_str());

    // Teclas especiais
    if      (msg == "ENTER")     { Keyboard.write(KEY_RETURN);        }
    else if (msg == "TAB")       { Keyboard.write(KEY_TAB);           }
    else if (msg == "BACKSPACE") { Keyboard.write(KEY_BACKSPACE);     }
    else if (msg == "DELETE")    { Keyboard.write(KEY_DELETE);        }
    else if (msg == "ESC")       { Keyboard.write(KEY_ESC);           }
    else if (msg == "UP")        { Keyboard.write(KEY_UP_ARROW);      }
    else if (msg == "DOWN")      { Keyboard.write(KEY_DOWN_ARROW);    }
    else if (msg == "LEFT")      { Keyboard.write(KEY_LEFT_ARROW);    }
    else if (msg == "RIGHT")     { Keyboard.write(KEY_RIGHT_ARROW);   }
    else if (msg == "HOME")      { Keyboard.write(KEY_HOME);          }
    else if (msg == "END")       { Keyboard.write(KEY_END);           }
    else if (msg == "PAGEUP")    { Keyboard.write(KEY_PAGE_UP);       }
    else if (msg == "PAGEDOWN")  { Keyboard.write(KEY_PAGE_DOWN);     }
    else if (msg == "INSERT")    { Keyboard.write(KEY_INSERT);        }
    else if (msg == "F1")        { Keyboard.write(KEY_F1);            }
    else if (msg == "F2")        { Keyboard.write(KEY_F2);            }
    else if (msg == "F3")        { Keyboard.write(KEY_F3);            }
    else if (msg == "F4")        { Keyboard.write(KEY_F4);            }
    else if (msg == "F5")        { Keyboard.write(KEY_F5);            }
    else if (msg == "F6")        { Keyboard.write(KEY_F6);            }
    else if (msg == "F7")        { Keyboard.write(KEY_F7);            }
    else if (msg == "F8")        { Keyboard.write(KEY_F8);            }
    else if (msg == "F9")        { Keyboard.write(KEY_F9);            }
    else if (msg == "F10")       { Keyboard.write(KEY_F10);           }
    else if (msg == "F11")       { Keyboard.write(KEY_F11);           }
    else if (msg == "F12")       { Keyboard.write(KEY_F12);           }

    // Combinações com CTRL
    else if (msg == "CTRL+C")    { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('c'); delay(50); Keyboard.releaseAll(); }
    else if (msg == "CTRL+V")    { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('v'); delay(50); Keyboard.releaseAll(); }
    else if (msg == "CTRL+X")    { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('x'); delay(50); Keyboard.releaseAll(); }
    else if (msg == "CTRL+Z")    { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('z'); delay(50); Keyboard.releaseAll(); }
    else if (msg == "CTRL+A")    { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('a'); delay(50); Keyboard.releaseAll(); }
    else if (msg == "CTRL+S")    { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('s'); delay(50); Keyboard.releaseAll(); }
    else if (msg == "CTRL+F")    { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('f'); delay(50); Keyboard.releaseAll(); }
    else if (msg == "CTRL+W")    { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('w'); delay(50); Keyboard.releaseAll(); }
    else if (msg == "CTRL+T")    { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('t'); delay(50); Keyboard.releaseAll(); }
    else if (msg == "CTRL+LEFT") { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press(KEY_LEFT_ARROW); delay(50); Keyboard.releaseAll(); }
    else if (msg == "CTRL+RIGHT"){ Keyboard.press(KEY_LEFT_CTRL); Keyboard.press(KEY_RIGHT_ARROW); delay(50); Keyboard.releaseAll(); }

    // Combinações com SHIFT
    else if (msg == "SHIFT+TAB") { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.press(KEY_TAB); delay(50); Keyboard.releaseAll(); }
    else if (msg == "SHIFT+LEFT") { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.press(KEY_LEFT_ARROW); delay(50); Keyboard.releaseAll(); }
    else if (msg == "SHIFT+RIGHT"){ Keyboard.press(KEY_LEFT_SHIFT); Keyboard.press(KEY_RIGHT_ARROW); delay(50); Keyboard.releaseAll(); }
    else if (msg == "SHIFT+UP")  { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.press(KEY_UP_ARROW); delay(50); Keyboard.releaseAll(); }
    else if (msg == "SHIFT+DOWN"){ Keyboard.press(KEY_LEFT_SHIFT); Keyboard.press(KEY_DOWN_ARROW); delay(50); Keyboard.releaseAll(); }

    // Caractere normal
    else if (msg.length() == 1) {
        Keyboard.press((char)msg[0]);
        delay(50);
        Keyboard.releaseAll();
    }

    delay(20);
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

void setup() {
    Serial0.begin(115200);
    delay(1000);
    Serial0.println("[SYSTEM] Initializing...");


    USB.VID(0x25A7);
    USB.PID(0xFA23);
    USB.productName("teclado-maldito");
    USB.manufacturerName("Omega Driver");
    // HID PRIMEIRO — PC precisa reconhecer antes do WiFi
    Keyboard.begin();
    USB.begin();
    Serial0.println("[HID] Keyboard initialized");
    delay(1000); // aguarda PC reconhecer o dispositivo HID

    Serial0.println("[WIFI] Connecting...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial0.print(".");
    }
    Serial0.println();
    Serial0.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());

    wsClient.begin(ws_host, ws_port, "/");
    wsClient.onEvent(onWebSocketEvent);
    wsClient.setReconnectInterval(3000);
    Serial0.println("[WS] Connecting to server...");
}

void loop() {
    wsClient.loop();
}
