import csv
from flask import Flask, request
from datetime import datetime
from pathlib import Path

VAULT = Path(r"C:\Users\LENOVO\Clement\Apps\Obsidian\SecondBrain")
LOG_FOLDER = VAULT / "Maru"
DATA_FOLDER = Path(__file__).parent / "data"  # raw data stays in the project folder

FIELDS = ["time", "temp", "hum", "light", "motion", "sound", "studying"]

app = Flask(__name__)
last_studying = None
session_start = None


def save_raw(now, data):
    """Every reading: one row in today's CSV file."""
    DATA_FOLDER.mkdir(exist_ok=True)
    file = DATA_FOLDER / f"{now:%Y-%m-%d}.csv"
    is_new = not file.exists()
    with file.open("a", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        if is_new:
            writer.writerow(FIELDS)
        writer.writerow([f"{now:%H:%M:%S}"] + [data.get(k) for k in FIELDS[1:]])


def log_event(now, text):
    """Only important events: one line in today's Obsidian note."""
    LOG_FOLDER.mkdir(parents=True, exist_ok=True)
    note = LOG_FOLDER / f"{now:%Y-%m-%d}.md"
    if not note.exists():
        note.write_text(f"# Maru log {now:%Y-%m-%d}\n\n", encoding="utf-8")
    with note.open("a", encoding="utf-8") as f:
        f.write(f"- {now:%H:%M} {text}\n")


@app.route("/data", methods=["POST"])
def receive():
    global last_studying, session_start
    data = request.get_json()
    now = datetime.now()

    save_raw(now, data)

    # Did the study state change since the last reading?
    studying = data.get("studying")
    if last_studying is not None and studying != last_studying:
        if studying == 1:
            session_start = now
            log_event(now, f"📚 Study session started ({data['temp']} °C, {data['hum']} % humidity)")
        elif session_start is not None:
            minutes = round((now - session_start).total_seconds() / 60)
            log_event(now, f"✅ Study session ended after {minutes} min")
    last_studying = studying

    print("Received:", data)
    return "ok"


app.run(host="0.0.0.0", port=5000)