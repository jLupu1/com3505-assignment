from flask import Flask, request, send_file
from simple_websocket import ConnectionClosed, Server

app = Flask(__name__)

hardware_ws = None
software_ws = None


@app.route("/")
def index():
    return send_file("index.html")


@app.route("/hardware", websocket=True)
def hardware():
    global hardware_ws, software_ws

    ws = Server.accept(request.environ)
    hardware_ws = ws

    try:
        while True:
            message = ws.receive()

            if software_ws:
                software_ws.send(message)
    except ConnectionClosed:
        hardware_ws = None

    return ""

@app.route("/software", websocket=True)
def software():
    global hardware_ws, software_ws

    ws = Server.accept(request.environ)
    software_ws = ws

    try:
        while True:
            message = ws.receive()

            if hardware_ws:
                hardware_ws.send(message)
    except ConnectionClosed:
        software_ws = None

    return ""


if __name__ == "__main__":
    app.run(host='0.0.0.0',port='6767')
