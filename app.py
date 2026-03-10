# app.py
from flask import Flask, jsonify, request, render_template
from flask_cors import CORS
from pathlib import Path
import json
import os
import processor

# ===============================
# APP SETUP
# ===============================
app = Flask(__name__, template_folder="templates", static_folder="static")
CORS(app)

# ===============================
# CONFIG PATHS
# ===============================
BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"
SESSIONS_DIR = DATA_DIR / "sessions"
SUBJECT_ID = "Subject_2"

os.makedirs(SESSIONS_DIR, exist_ok=True)

# ===============================
# PAGE ROUTES (SERVE HTML)
# ===============================

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/<page>")
def load_page(page):
    try:
        return render_template(f"{page}.html")
    except:
        return "Page not found", 404


# ===============================
# HISTORICAL DATA ENDPOINT
# ===============================
@app.route("/api/patient-history", methods=["GET"])
def get_history():

    activities = {"squat": 0, "extension": 3, "gait": 6}
    response = {}

    for name, folder_id in activities.items():
        try:
            rom, rms, rom_summary, rms_summary = processor.get_weekly_progress(
                SUBJECT_ID, folder_id, DATA_DIR
            )

            response[name] = {
                "rom": rom,
                "rms": rms,
                "romSummary": rom_summary,
                "rmsSummary": rms_summary,
                "completion": [90 for _ in rom],
                "currentRom": rom[-1] if rom else 0,
                "romChange": f"{round(rom[-1] - rom[0], 1)}°" if len(rom) > 1 else "0°"
            }

        except Exception as e:
            print("History error:", e)
            response[name] = {
                "rom": [],
                "rms": [],
                "romSummary": {},
                "rmsSummary": {},
                "completion": [],
                "currentRom": 0,
                "romChange": "0°"
            }

    return jsonify(response)


# ===============================
# LIVE DATA ENDPOINT
# ===============================
@app.route("/api/live-data", methods=["POST"])
def live_data():

    try:
        data = request.get_json(silent=True)

        if not data:
            return jsonify({
                "liveRom": 0,
                "liveRms": 0,
                "message": "No data received yet"
            })

        imu_list = data.get("imu")
        emg_list = data.get("emg")

        if not imu_list or not emg_list:
            return jsonify({
                "liveRom": 0,
                "liveRms": 0,
                "message": "Waiting for IMU/EMG data"
            })

        live_rom, live_rms = processor.process_live_packet(imu_list, emg_list)

        return jsonify({
            "liveRom": round(float(live_rom), 2),
            "liveRms": round(float(live_rms), 5)
        })

    except Exception as e:
        print("Live processing error:", e)
        return jsonify({
            "liveRom": 0,
            "liveRms": 0,
            "error": "Processing failure"
        }), 500


# ===============================
# SAVE SESSION
# ===============================
@app.route("/api/save-session", methods=["POST"])
def save_session():

    try:
        data = request.get_json(silent=True)

        if not data or "summary" not in data:
            return jsonify({"error": "Invalid payload"}), 400

        session_id = data["summary"]["id"]
        file_path = SESSIONS_DIR / f"{session_id}.json"

        with open(file_path, "w") as f:
            json.dump(data, f, indent=4)

        print(f"Session {session_id} saved.")
        return jsonify({"success": True})

    except Exception as e:
        print("Save error:", e)
        return jsonify({"error": str(e)}), 500


# ===============================
# GET SESSIONS
# ===============================
@app.route("/api/get-sessions", methods=["GET"])
def get_sessions():

    sessions = []

    try:
        for filename in os.listdir(SESSIONS_DIR):

            if filename.endswith(".json"):

                with open(SESSIONS_DIR / filename, "r") as f:
                    session_data = json.load(f)
                    sessions.append(session_data["summary"])

        sessions.sort(key=lambda x: x.get("date", ""), reverse=True)

        return jsonify({"sessions": sessions})

    except Exception as e:
        print("Get sessions error:", e)
        return jsonify({"sessions": []})


# ===============================
# START SERVER
# ===============================
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001, debug=True)