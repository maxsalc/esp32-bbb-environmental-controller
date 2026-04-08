import serial
import csv
import os
from datetime import datetime

PORT = "/dev/ttyS1"
BAUD = 115200
CSV_FILE = "env_log.csv"


def ensure_csv_exists():
    if not os.path.exists(CSV_FILE):
        with open(CSV_FILE, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["timestamp", "temp_f", "humidity", "fan", "state"])


def parse_line(line):
    data = {}

    parts = line.split(",")

    for part in parts:
        if "=" not in part:
            continue

        key, value = part.split("=", 1)
        data[key.strip()] = value.strip()

    return {
        "temp_f": data.get("TEMP_F", ""),
        "humidity": data.get("HUM", ""),
        "fan": data.get("FAN", ""),
        "state": data.get("STATE", "")
    }


def main():
    ensure_csv_exists()

    ser = serial.Serial(PORT, BAUD, timeout=1)
    print(f"Listening on {PORT} at {BAUD} baud...")

    try:
        with open(CSV_FILE, "a", newline="") as f:
            writer = csv.writer(f)

            while True:
                line = ser.readline().decode(errors="ignore").strip()

                if not line:
                    continue

                parsed = parse_line(line)
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

                print(f"[{timestamp}] {parsed}")

                writer.writerow([
                    timestamp,
                    parsed["temp_f"],
                    parsed["humidity"],
                    parsed["fan"],
                    parsed["state"]
                ])
                f.flush()

    except KeyboardInterrupt:
        print("\nStopped by user.")

    finally:
        ser.close()


if __name__ == "__main__":
    main()