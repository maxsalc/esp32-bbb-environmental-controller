import csv
import os
from flask import Flask, render_template

app = Flask(__name__)
CSV_FILE = "env_log.csv"

def get_all_rows():
    if not os.path.exists(CSV_FILE):
        return []

    with open(CSV_FILE, "r", newline="") as f:
        reader = csv.DictReader(f)
        return list(reader)

@app.route("/")
def index():
    rows = get_all_rows()
    latest = rows[-1] if rows else None
    recent_rows = rows[-10:] if rows else []
    recent_rows.reverse()

    return render_template(
        "index.html",
        latest=latest,
        recent_rows=recent_rows
    )
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)