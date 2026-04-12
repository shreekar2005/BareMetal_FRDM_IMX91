import socket
import threading
import datetime
from flask import Flask, render_template, request, jsonify

app = Flask(__name__)

LAPTOP_TCP_PORT = 5555
NXP_IP = "192.168.21.234" # Change this to your ESP's IP
NXP_PORT = 8080

# SHARED STATE
# Both threads can read and write to this variable
LATEST_NXP_STATUS = "Awaiting status push from NXP..."


# FLASK WEB ROUTES (Port 5000)
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/status', methods=['GET'])
def get_status():
    # The web browser asks for this every 2 seconds
    return jsonify({"status": LATEST_NXP_STATUS})

@app.route('/api/command', methods=['POST'])
def send_command():
    # The web browser sends a command here
    cmd = request.json.get('command', '')
    if not cmd:
        return jsonify({"error": "Empty command"}), 400

    # Magically prepend 'exec ' so the user doesn't have to!
    full_command = f"exec {cmd}\n"
    
    try:
        # Connect to the NXP's 8080 server and inject the command
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(3)
        s.connect((NXP_IP, NXP_PORT))
        s.send(full_command.encode('utf-8'))
        s.close()
        return jsonify({"success": True, "sent": full_command})
    except Exception as e:
        return jsonify({"error": str(e)}), 500


# RAW TCP SERVER THREAD (Port 5555)
def raw_tcp_server_thread():
    global LATEST_NXP_STATUS
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind(("0.0.0.0", LAPTOP_TCP_PORT))
    server.listen(5)
    print(f"[*] IoT Hub: Raw TCP Server listening on port {LAPTOP_TCP_PORT}...")

    while True:
        client, addr = server.accept()
        # Read the raw payload from the NXP
        data = client.recv(4096).decode('utf-8', errors='ignore')
        client.close() # Instantly close the socket

        if "GET_TIME" in data:
            print(f"[*] Time sync requested by NXP ({addr[0]})")
            now = datetime.datetime.now().strftime("%H:%M:%S %d:%m:%Y")
            cmd = f"exec datetime --set {now}\n"
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect((NXP_IP, NXP_PORT))
                s.send(cmd.encode('utf-8'))
                s.close()
                print(f"[+] Synced time to NXP: {now}")
            except:
                print("[-] Failed to send time to NXP")
                
        elif data.startswith("STATUS:"):
            # If the NXP sends "STATUS: <huge string of data>"
            # We strip off the "STATUS:" prefix and save it to the global variable
            LATEST_NXP_STATUS = data[7:]
            print(f"[+] Received system status update from NXP ({len(LATEST_NXP_STATUS)} bytes)")

if __name__ == "__main__":
    # start the raw TCP server in a background thread
    threading.Thread(target=raw_tcp_server_thread, daemon=True).start()
    
    # start the Flask web server in the main thread
    print("[*] IoT Hub: Web Interface starting on port 5000...")
    app.run(host="0.0.0.0", port=5000, debug=False)