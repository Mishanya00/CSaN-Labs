import socket

HOST = '127.0.0.1'  # Localhost
PORT = 12345        # Port to listen on

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    print(f"Listening on {HOST}:{PORT}...")

    while True:  # Keep the server running indefinitely
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            data = conn.recv(1024)
            if data:
                print(f"Received: {data.decode('utf-8')}")
                conn.sendall(data)  # Echo back the received data
            print(f"Connection with {addr} closed.")
