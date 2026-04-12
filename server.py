import asyncio
import websockets
import sys
import tty
import termios

HOST = "0.0.0.0"
PORT = 8765

esp_websocket = None

async def handler(websocket):
    global esp_websocket
    esp_websocket = websocket
    print("[SERVER] ESP32 connected!", flush=True)
    try:
        async for message in websocket:
            print(f"[ESP] {message}", flush=True)
    except websockets.exceptions.ConnectionClosed:
        print("[SERVER] ESP32 disconnected", flush=True)
        esp_websocket = None

async def keyboard_input():
    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    tty.setraw(fd)

    special = {
        "\r":     "ENTER",
        "\t":     "TAB",
        "\x7f":   "BACKSPACE",
        "\x1b[A": "UP",
        "\x1b[B": "DOWN",
        "\x1b[C": "RIGHT",
        "\x1b[D": "LEFT",
        "\x1b[H": "HOME",
        "\x1b[F": "END",
        "\x1b[5~": "PAGEUP",
        "\x1b[6~": "PAGEDOWN",
        "\x1b[2~": "INSERT",
        "\x1b[3~": "DELETE",
        "\x1b[1;5C": "CTRL+RIGHT",
        "\x1b[1;5D": "CTRL+LEFT",
        "\x1b[1;2C": "SHIFT+RIGHT",
        "\x1b[1;2D": "SHIFT+LEFT",
        "\x1b[1;2A": "SHIFT+UP",
        "\x1b[1;2B": "SHIFT+DOWN",
        "\x1b\t":   "SHIFT+TAB",
        "\x03":   "CTRL+C",  # cuidado — também é o sinal de saída
        "\x16":   "CTRL+V",
        "\x18":   "CTRL+X",
        "\x1a":   "CTRL+Z",
        "\x01":   "CTRL+A",
        "\x13":   "CTRL+S",
        "\x06":   "CTRL+F",
        "\x17":   "CTRL+W",
        "\x14":   "CTRL+T",
        "\x1bOP": "F1",
        "\x1bOQ": "F2",
        "\x1bOR": "F3",
        "\x1bOS": "F4",
        "\x1b[15~": "F5",
        "\x1b[17~": "F6",
        "\x1b[18~": "F7",
        "\x1b[19~": "F8",
        "\x1b[20~": "F9",
        "\x1b[21~": "F10",
        "\x1b[23~": "F11",
        "\x1b[24~": "F12",
    }

    try:
        loop = asyncio.get_event_loop()
        while True:
            char = await loop.run_in_executor(None, sys.stdin.read, 1)

            # lê sequência de escape completa
            if char == "\x1b":
                rest = await loop.run_in_executor(None, sys.stdin.read, 5)
                char = char + rest.rstrip("\x00")

            key = special.get(char, char if len(char) == 1 else None)

            if key is None:
                continue

            # CTRL+Q para sair (evita conflito com CTRL+C sendo enviado)
            if char == "\x11":
                break

            print(key, end=" ", flush=True)

            if esp_websocket:
                await esp_websocket.send(key)
            else:
                print("\n[WARN] ESP not connected", flush=True)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old)

async def main():
    async with websockets.serve(handler, HOST, PORT):
        print(f"[SERVER] Listening on ws://{HOST}:{PORT}", flush=True)
        await keyboard_input()

asyncio.run(main())
