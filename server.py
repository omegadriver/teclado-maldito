import asyncio
import websockets
from flask import Flask, render_template_string
import threading

app = Flask(__name__)
WS_PORT = 8765
clients = set()

HTML = """
<!DOCTYPE html>
<html>
<head>
    <title>Keyboard</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            background: #1a1a2e;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            font-family: monospace;
            gap: 16px;
        }
        h2 { color: #e0e0e0; }
        #status { font-size: 13px; color: #888; }
        #capture {
            background: #0f0f1a;
            border: 2px solid #0f3460;
            border-radius: 8px;
            color: #00ff88;
            font-size: 16px;
            padding: 14px 20px;
            width: 420px;
            text-align: center;
            outline: none;
            caret-color: transparent;
        }
        #capture:focus { border-color: #00ff88; }
        #display {
            font-size: 32px;
            color: #00ff88;
            background: #0f0f1a;
            border: 1px solid #222;
            border-radius: 8px;
            padding: 18px 50px;
            min-width: 260px;
            text-align: center;
            letter-spacing: 3px;
        }
        #modifiers {
            display: flex;
            gap: 10px;
        }
        .mod-btn {
            padding: 10px 20px;
            font-family: monospace;
            font-size: 13px;
            border-radius: 8px;
            border: 2px solid #0f3460;
            background: #0f0f1a;
            color: #888;
            cursor: pointer;
            transition: all 0.15s;
        }
        .mod-btn.active {
            background: #00ff88;
            color: #111;
            border-color: #00ff88;
        }
        #log {
            width: 420px;
            height: 160px;
            overflow-y: auto;
            background: #0f0f1a;
            border: 1px solid #222;
            border-radius: 8px;
            padding: 10px;
            font-size: 12px;
            color: #555;
        }
        #log div { margin-bottom: 4px; }
        #log .sent { color: #00ff88; }
        #log .info { color: #444; }
    </style>
</head>
<body>
    <h2>ESP32 Keyboard</h2>
    <div id="status">Connecting...</div>
    <input id="capture" placeholder="clique aqui e digite..." autocomplete="off" spellcheck="false" readonly />

    <div id="modifiers">
        <button class="mod-btn" id="btn-win"        onclick="toggleMod('WIN')">⊞ WIN</button>
        <button class="mod-btn" id="btn-ctrl_alt"   onclick="toggleMod('CTRL+ALT')">CTRL+ALT</button>
        <button class="mod-btn" id="btn-ctrl_shift" onclick="toggleMod('CTRL+SHIFT')">CTRL+SHIFT</button>
    </div>

    <div id="display">—</div>
    <div id="log"></div>

    <script>
        const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
        const ws = new WebSocket(proto + '//' + location.host + '/ws');
        const display = document.getElementById('display');
        const status  = document.getElementById('status');
        const capture = document.getElementById('capture');
        const log     = document.getElementById('log');

        let activeMod = null;

        ws.onopen  = () => { status.textContent = 'Connected ✓';    status.style.color = '#00ff88'; addLog('WebSocket connected', 'info'); };
        ws.onclose = () => { status.textContent = 'Disconnected ✗'; status.style.color = '#ff4444'; addLog('WebSocket disconnected', 'info'); };

        function toggleMod(mod) {
            if (activeMod === mod) {
                deactivateMod();
            } else {
                deactivateMod();
                activeMod = mod;
                document.getElementById('btn-' + mod.toLowerCase().replace('+', '_')).classList.add('active');
                addLog(mod + ' ativado', 'info');
            }
            capture.focus();
        }

        function deactivateMod() {
            if (!activeMod) return;
            document.getElementById('btn-' + activeMod.toLowerCase().replace('+', '_')).classList.remove('active');
            addLog(activeMod + ' desativado', 'info');
            activeMod = null;
        }

        function addLog(msg, cls='sent') {
            const d = document.createElement('div');
            d.style.display = 'flex';
            d.style.justifyContent = 'space-between';
            d.style.alignItems = 'center';

            const text = document.createElement('span');
            text.className = cls;
            text.textContent = (cls === 'sent' ? '→ ' : '  ') + msg;

            const time = document.createElement('span');
            const now = new Date();
            time.textContent = now.toLocaleTimeString('pt-BR', {
                hour: '2-digit', minute: '2-digit', second: '2-digit'
            });
            time.style.color = '#2a2a2a';
            time.style.fontSize = '11px';
            time.style.flexShrink = '0';
            time.style.marginLeft = '10px';

            d.appendChild(text);
            d.appendChild(time);
            log.appendChild(d);
            log.scrollTop = log.scrollHeight;
        }

        function send(key) {
            if (ws.readyState !== WebSocket.OPEN) {
                addLog('Not connected!', 'info');
                return;
            }
            ws.send(key);
            display.textContent = key;
            addLog(key);
        }

        const simpleMap = {
            'Enter':     'ENTER',
            'Tab':       'TAB',
            'Backspace': 'BACKSPACE',
            'Delete':    'DELETE',
            'Escape':    'ESC',
            'CapsLock':  'CAPSLOCK',
            'ArrowUp':   'UP',
            'ArrowDown': 'DOWN',
            'ArrowLeft': 'LEFT',
            'ArrowRight':'RIGHT',
            ' ':         'SPACE',
            'F1':'F1','F2':'F2','F3':'F3','F4':'F4',
            'F5':'F5','F6':'F6','F7':'F7','F8':'F8',
            'F9':'F9','F10':'F10','F11':'F11','F12':'F12',
        };

        capture.addEventListener('keydown', function(e) {
            e.preventDefault();

            const ctrl = e.ctrlKey;
            const meta = e.metaKey;
            const k    = e.key;

            if (['Control','Shift','Alt','Meta'].includes(k)) return;

            let mapped = simpleMap[k] || (k.length === 1 ? k : null);
            if (!mapped) return;

            let key = null;

            if (activeMod) {
                key = activeMod + '+' + (simpleMap[k] || k.toUpperCase());
                deactivateMod();
            } else if (meta && k !== 'Meta') {
                key = 'WIN+' + (simpleMap[k] || k.toUpperCase());
            } else if (ctrl && k !== 'Control') {
                key = 'CTRL+' + (simpleMap[k] || k.toUpperCase());
            } else {
                key = mapped;
            }

            if (key) send(key);
        });

        window.addEventListener('load', () => capture.focus());
        capture.addEventListener('blur', () => setTimeout(() => capture.focus(), 100));
    </script>
</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML)

async def ws_handler(websocket):
    clients.add(websocket)
    print(f"[WS] Client connected: {websocket.remote_address}")
    try:
        async for msg in websocket:
            print(f"[WS] → '{msg}'")
            others = [c for c in clients if c != websocket]
            if others:
                await asyncio.gather(*[c.send(msg) for c in others])
    except:
        pass
    finally:
        clients.discard(websocket)
        print(f"[WS] Client disconnected")

async def ws_main():
    async with websockets.serve(ws_handler, "0.0.0.0", WS_PORT):
        print(f"[WS] ws://0.0.0.0:{WS_PORT}")
        await asyncio.Future()

def run_ws():
    asyncio.run(ws_main())

# inicia websocket ao importar — funciona com gunicorn e python direto
threading.Thread(target=run_ws, daemon=True).start()

if __name__ == "__main__":
    print("[HTTP] http://0.0.0.0:5000")
    app.run(host="0.0.0.0", port=5000, debug=False)
