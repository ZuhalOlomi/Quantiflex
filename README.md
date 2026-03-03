<p align="center">
  <img src="UTBIOME-Logo-1.jpg" alt="UTBiome Logo" width="150"/>
</p>

# UTBiome Knee Sleeve – ACL Rehabilitation Tracking Platform

## Project Overview
The **UTBiome Knee Sleeve** is a clinically-integrated wearable system designed to quantify recovery following **Anterior Cruciate Ligament (ACL) reconstruction**. Standard rehabilitation currently lacks objective, home-based quantitative tools, leading to a high risk of re-injury. 

Our platform bridges this gap by combining **Inertial Measurement Units (IMUs)** and **Surface Electromyography (sEMG)** to provide real-time neuromuscular and kinematic data through a non-invasive, home-based wearable.

---

## Technical Specifications
### 1. Hardware Subsystem
- **Microcontrollers:** Dual ESP32-S3-Zero modules.
- **Kinematics:** Two MPU-6050 IMUs configured as a digital goniometer to measure knee flexion/extension.
- **Neuromuscular:** Two MyoWare 2.0 sEMG sensors targeting the *vastus medialis* and *biceps femoris*.
- **Fabric:** Moisture-wicking elastomeric rib knit with 3D printed TPU lateral supports for joint stability.

### 2. Software Architecture
- **Backend:** Python Flask API (`app.py`) running on **Port 5001**. Handles real-time signal filtering, outlier removal, and metric computation.
- **Frontend:** Responsive web suite (HTML5/CSS3/JS) using **GSAP** for animations and **Chart.js** for live telemetry visualization.
- **Authentication:** Integrated **Google OAuth 2.0** with role-based access control (RBAC) for Patients and Clinicians.

---

## Recovery Metrics & Clinical Targets
The system monitors the **Limb Symmetry Index (LSI)** to provide objective "Return to Sport" (RTS) milestones:
- **Kinematic Target:** LSI $\ge$ 0.95 for Range of Motion (ROM).
- **Neuromuscular Target:** LSI $\le$ 1.10 for Muscle Activation (EMG).
- **Sampling Rate:** 2 kHz for high-fidelity muscle capture.
- **Range:** 0° (full extension) to 150° (flexion).



---

## Competitive Differentiation
| Feature | UTBiome | Consumer Trackers | Clinical Labs (Vicon) |
| :--- | :--- | :--- | :--- |
| **Data Modality** | sEMG + IMU | IMU Only | IR Markers + EMG |
| **Portability** | High (In-Home) | High | Low (Lab Bound) |
| **Cost** | < $500 | $100 - $300 | $50,000+ |
| **Metric Depth** | LSI & Activation | Step Count/ROM | Full Biomechanics |

---

## Project Structure
```bash
tnbc-website/
├── main.html          # Central landing page
├── login.html         # Google OAuth & Role Toggle gatekeeper
├── analysis.html      # Patient-facing dashboard (ROM/LSI)
├── clinician_dashboard.html # Clinician monitoring portal (Live Telemetry)
├── logout.html        # Secure session termination UI
├── auth.js            # Unified Auth, Security Guard & Backend sync
├── style.css          # Global UTBiome design system
├── backend/
│   ├── app.py         # Flask API (Port 5001)
│   └── processor.py   # Signal processing engine
└── data/              # Subject data and clinical PDFs

