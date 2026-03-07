from flask import Flask, jsonify, request
from flask_cors import CORS
import numpy as np
from pathlib import Path
import json
import os
import processor

app = Flask(__name__)
CORS(app)

# ===============================
# CONFIG
# ===============================
BASE_DIR = Path(__file__).resolve().parent.parent
DATA_DIR = str(BASE_DIR / "data")
SESSIONS_DIR = str(BASE_DIR / "data" / "sessions")
ASSIGNMENTS_DIR = str(BASE_DIR / "data" / "assignments")
MESSAGES_DIR = str(BASE_DIR / "data" / "messages")
SUBJECT_ID = "Subject_2"

os.makedirs(SESSIONS_DIR, exist_ok=True)
os.makedirs(ASSIGNMENTS_DIR, exist_ok=True)
os.makedirs(MESSAGES_DIR, exist_ok=True)

# ===============================
# HISTORICAL DATA ENDPOINT
# ===============================
@app.route('/api/patient-history', methods=['GET'])
def get_history():
    print("🔥 /api/patient-history HIT")

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
# LIVE DATA ENDPOINT (FIXED)
# ===============================
@app.route('/api/live-data', methods=['POST'])
def live_data():
    try:
        data = request.get_json(silent=True)

        # If frontend sends nothing yet, return safe default instead of 400
        if not data:
            return jsonify({
                "liveRom": 0,
                "liveRms": 0,
                "message": "No data received yet"
            }), 200

        imu_list = data.get("imu")
        emg_list = data.get("emg")

        # If keys missing, return safe default instead of crashing
        if not imu_list or not emg_list:
            return jsonify({
                "liveRom": 0,
                "liveRms": 0,
                "message": "Waiting for IMU/EMG data"
            }), 200

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
@app.route('/api/save-session', methods=['POST'])
def save_session():
    try:
        data = request.get_json(silent=True)

        if not data or 'summary' not in data:
            return jsonify({"error": "Invalid payload"}), 400

        session_id = data['summary']['id']
        file_path = os.path.join(SESSIONS_DIR, f"{session_id}.json")

        with open(file_path, 'w') as f:
            json.dump(data, f, indent=4)

        print(f"✅ Session {session_id} saved.")
        return jsonify({"success": True})

    except Exception as e:
        print("Save error:", e)
        return jsonify({"error": str(e)}), 500

# ===============================
# GET SESSIONS
# ===============================
@app.route('/api/get-sessions', methods=['GET'])
def get_sessions():
    sessions = []

    try:
        for filename in os.listdir(SESSIONS_DIR):
            if filename.endswith(".json"):
                with open(os.path.join(SESSIONS_DIR, filename), 'r') as f:
                    session_data = json.load(f)
                    sessions.append(session_data['summary'])

        sessions.sort(key=lambda x: x.get('date', ''), reverse=True)

        return jsonify({"sessions": sessions})

    except Exception as e:
        print("Get sessions error:", e)
        return jsonify({"sessions": []})

# ===============================
# ASSIGNMENTS ENDPOINTS
# ===============================
@app.route('/api/assignments', methods=['POST'])
def save_assignment():
    try:
        data = request.get_json(silent=True)
        if not data or 'patientId' not in data:
            return jsonify({"error": "Invalid payload"}), 400

        patient_id = data['patientId']
        file_path = os.path.join(ASSIGNMENTS_DIR, f"{patient_id}.json")

        with open(file_path, 'w') as f:
            json.dump(data, f, indent=4)

        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/api/assignments/<patient_id>', methods=['GET'])
def get_assignment(patient_id):
    try:
        file_path = os.path.join(ASSIGNMENTS_DIR, f"{patient_id}.json")
        if not os.path.exists(file_path):
            return jsonify({"exercises": []})

        with open(file_path, 'r') as f:
            data = json.load(f)
            return jsonify(data)
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ===============================
# CHAT MESSAGES ENDPOINTS
# ===============================
@app.route('/api/messages', methods=['POST'])
def send_message():
    try:
        data = request.get_json(silent=True)
        if not data or 'patientId' not in data:
            return jsonify({"error": "Invalid payload"}), 400

        patient_id = data['patientId']
        file_path = os.path.join(MESSAGES_DIR, f"{patient_id}.json")

        messages = []
        if os.path.exists(file_path):
            with open(file_path, 'r') as f:
                messages = json.load(f)

        messages.append(data)

        with open(file_path, 'w') as f:
            json.dump(messages, f, indent=4)

        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/api/messages/<patient_id>', methods=['GET'])
def get_messages(patient_id):
    try:
        file_path = os.path.join(MESSAGES_DIR, f"{patient_id}.json")
        if not os.path.exists(file_path):
            return jsonify([])

        with open(file_path, 'r') as f:
            messages = json.load(f)
            return jsonify(messages)
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ===============================
# START SERVER
# ===============================
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001, debug=True)