import socket
import threading
import datetime
import time
from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO

app = Flask(__name__)
# Initialize SocketIO
socketio = SocketIO(app, cors_allowed_origins="*")

LAPTOP_TCP_PORT = 5555
NXP_PORT = 8080

# SHARED STATE
LATEST_NXP_STATUS = "Awaiting status push from NXP..."
LATEST_NXP_IP = None  

def parse_compact_status(raw_data):
    html = "<h2>[NXP LittleOS Dashboard]</h2>"
    uptime_str = ""
    sched_str = ""
    esp_str = "<h3>[Wi-Fi Bridge - ESP8266]</h3><ul style='list-style-type: none; padding-left: 0;'>"
    
    threads = []
    
    for line in raw_data.strip().split('\n'):
        try:
            parts = line.split(',')
            if parts[0] == 'U' and len(parts) >= 4:
                uptime_str = f"<p><b>Uptime:</b> {parts[1]}h : {parts[2]}m : {parts[3]}s</p>"
            elif parts[0] == 'S' and len(parts) >= 2:
                sched_str = f"<p><b>Scheduler:</b> {parts[1]}</p>"
            elif parts[0] == 'T' and len(parts) >= 10:
                threads.append(parts)
            elif parts[0] == 'E':
                reachable = "[ONLINE]" if len(parts) > 1 and parts[1] == "1" else "[OFFLINE]"
                status_color = "#00ff00" if len(parts) > 1 and parts[1] == "1" else "red"
                op_mode = parts[2] if len(parts) > 2 else "N/A"
                ssid = parts[3] if len(parts) > 3 else "N/A"
                router_mac = parts[4] if len(parts) > 4 else "N/A"
                ip_addr = parts[5] if len(parts) > 5 else "N/A"
                esp_mac = parts[6] if len(parts) > 6 else "N/A"

                esp_str += f"<li style='margin-bottom: 5px;'><b>Status:</b> <span style='color: {status_color}; font-weight: bold;'>{reachable}</span></li>"
                esp_str += f"<li style='margin-bottom: 5px;'><b>Op Mode:</b> {op_mode}</li>"
                esp_str += f"<li style='margin-bottom: 5px;'><b>SSID:</b> {ssid}</li>"
                esp_str += f"<li style='margin-bottom: 5px;'><b>Router MAC:</b> {router_mac}</li>"
                esp_str += f"<li style='margin-bottom: 5px;'><b>IP Address:</b> {ip_addr}</li>"
                esp_str += f"<li style='margin-bottom: 5px;'><b>ESP MAC:</b> {esp_mac}</li>"
                
        except Exception:
            pass 
            
    esp_str += "</ul>"
    
    html += uptime_str + sched_str
    
    if threads:
        html += "<h3>[Active Threads]</h3>"
        html += "<table border='1' cellpadding='8' style='border-collapse: collapse; width: 100%; text-align: center; font-family: monospace; background-color: #111; color: #00ff00; border-color: #00ff00;'>"
        html += "<tr style='background-color: #222;'><th>ID</th><th>Name</th><th>State</th><th>Pri</th><th>Dead</th><th>Per(ms)</th><th>Targ</th><th>Done</th><th>TAT(ms)</th></tr>"
        for t in threads:
            state_color = "#00ff00" if t[3] == "RUN" else "orange" if t[3] == "READY" else "red" if t[3] == "TERM" else "#0088ff"
            html += f"<tr><td>{t[1]}</td><td>{t[2]}</td><td style='color: {state_color}; font-weight: bold;'>{t[3]}</td><td>{t[4]}</td><td>{t[5]}</td><td>{t[6]}</td><td>{t[7]}</td><td>{t[8]}</td><td>{t[9]}</td></tr>"
        html += "</table><br>"
        
    html += esp_str
    return html

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/command', methods=['POST'])
def send_command():
    global LATEST_NXP_IP 

    cmd = request.json.get('command', '')
    if not cmd:
        return jsonify({"error": "Empty command"}), 400
    if not LATEST_NXP_IP:
        return jsonify({"error": "No NXP board has connected to the Hub yet!"}), 400

    full_command = f"exec {cmd}\n"
    
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(3)
        s.connect((LATEST_NXP_IP, NXP_PORT)) 
        s.send(full_command.encode('utf-8'))
        s.close()
        return jsonify({"success": True, "sent": full_command})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

def raw_tcp_server_thread():
    global LATEST_NXP_STATUS, LATEST_NXP_IP
    
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("0.0.0.0", LAPTOP_TCP_PORT))
    server.listen(5)
    print(f"[*] IoT Hub: Raw TCP Server listening on port {LAPTOP_TCP_PORT}...")

    while True:
        try:
            client, addr = server.accept()
            LATEST_NXP_IP = addr[0] 

            client.settimeout(5.0) 
            data = ""
            try:
                while True:
                    chunk = client.recv(4096).decode('utf-8', errors='ignore')
                    if not chunk: 
                        break 
                    data += chunk
            except socket.timeout:
                pass 

            if "GET_TIME" in data:
                print(f"[*] Time sync requested by NXP ({LATEST_NXP_IP})")
                now = datetime.datetime.now().strftime("%H:%M:%S %d:%m:%Y")
                cmd = f"exec datetime set {now}\n"
                time.sleep(1) 
                try:
                    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    s.settimeout(3)
                    s.connect((LATEST_NXP_IP, NXP_PORT)) 
                    s.send(cmd.encode('utf-8'))
                    s.close()
                    print(f"[+] Synced time to NXP: {now}")
                except Exception as e:
                    print(f"[-] Failed to send time to NXP: {e}")
                    
            elif "STATUS:\n" in data:
                status_idx = data.find("STATUS:\n")
                LATEST_NXP_STATUS = parse_compact_status(data[status_idx + 8:])
                print(f"[+] Received compact system status update from {LATEST_NXP_IP} ({len(data)} bytes)")
                
                socketio.emit('status_update', {'status': LATEST_NXP_STATUS})
                
        except Exception as global_e:
            print(f"[!] FATAL ERROR IN TCP BACKGROUND THREAD: {global_e}")
        finally:
            try:
                client.close()
            except:
                pass

if __name__ == "__main__":
    threading.Thread(target=raw_tcp_server_thread, daemon=True).start()
    print("[*] IoT Hub: Web Interface starting on port 5000...")
    # Use socketio.run instead of app.run
    socketio.run(app, host="0.0.0.0", port=5000, debug=False)