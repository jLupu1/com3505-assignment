from flask import Flask, send_file

app = Flask(__name__)


@app.route("/")
def index():
    # TODO render_template?
    return send_file("index.html")


if __name__ == "__main__":
    app.run()
