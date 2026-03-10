import numpy as np
import os
from pathlib import Path


def _summarize_list(values):
    """Return basic summary statistics for a list of numeric values.
    Note: solely used for displaying historical trends in the terminal?
    Returns a dict with count, mean, std, median, min, max, and pct_change (first->last).
    """
    arr = np.array(values, dtype=float)
    if arr.size == 0:
        return {
            "count": 0,
            "mean": 0.0,
            "std": 0.0,
            "median": 0.0,
            "min": 0.0,
            "max": 0.0,
            "pct_change": 0.0,
        }

    pct_change = 0.0
    if arr.size > 1 and arr[0] != 0:
        pct_change = float((arr[-1] - arr[0]) / abs(arr[0]) * 100.0)

    return {
        "count": int(arr.size),
        "mean": float(np.mean(arr)),
        "std": float(np.std(arr)),
        "median": float(np.median(arr)),
        "min": float(np.min(arr)),
        "max": float(np.max(arr)),
        "pct_change": float(pct_change),
    }

def calculate_rms(emg_signal):
    """
    Optimized RMS calculation.
    Supports both single channel (1D) and multiple channels (2D).
    """
    if emg_signal.ndim == 2:
        # Vectorized RMS across time axis for all 8 channels
        return np.sqrt(np.mean(emg_signal**2, axis=1))
    return np.sqrt(np.mean(emg_signal**2))

def calculate_rom(imu_data):
    """Calculates Max Flexion using gravity vector trigonometry."""
    if imu_data.shape[1] < 3:
        return 0.0
    
    # Using Ay and Az to find the inclination angle

# Amplitude? Strength of both muscles?

    ay = imu_data[:, 1]
    az = imu_data[:, 2]
    
    # Calculate angles for every sample point in the trial
    angles = np.abs(np.degrees(np.arctan2(ay, az)))
    return np.max(angles)

def process_live_packet(imu_list, emg_list):
    """
    Processes a single live data packet from the ESP32.
    Expects imu_list and emg_list as lists of lists (2D arrays).
    """
    imu = np.array(imu_list)
    emg = np.array(emg_list)
    
    live_rom = calculate_rom(imu)
    # Average RMS across all 8 channels for a single intensity metric
    live_rms = np.mean(calculate_rms(emg))
    
    return round(float(live_rom), 1), round(float(live_rms), 5)

def get_weekly_progress(subject_id, activity_folder, base_path):
    """Fetches historical data for trends."""
    rom_data = []
    rms_data = []
    
    path = Path(base_path) / subject_id / str(activity_folder)
    if not path.exists():
        # return empty lists and zeroed summaries so callers can display placeholders
        return [0], [0], _summarize_list([]), _summarize_list([])

    # Numeric sort for Trial_1, Trial_2, etc.
    trials = sorted(path.glob("Trial_*"), key=lambda x: int(x.name.split('_')[1]))
    
    for trial in trials:
        imu_file = trial / "imu.npy"
        emg_file = trial / "emg.npy"

        if imu_file.exists():
            imu = np.load(imu_file)
            rom_data.append(round(float(calculate_rom(imu)), 1))
        
        if emg_file.exists():
            emg = np.load(emg_file)
            # Calculate mean intensity across all 8 channels
            avg_rms = np.mean(calculate_rms(emg))
            rms_data.append(round(float(avg_rms), 5))

    rom_list = rom_data if rom_data else [0]
    rms_list = rms_data if rms_data else [0]

    rom_summary = _summarize_list(rom_list)
    rms_summary = _summarize_list(rms_list)

    return rom_list, rms_list, rom_summary, rms_summary