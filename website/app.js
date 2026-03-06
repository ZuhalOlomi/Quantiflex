// Inside website/app.js

async function fetchRealTimeData() {
    try {
        // send a small simulated packet (replace with real sensor data when available)
        const response = await fetch('http://127.0.0.1:5001/api/live-data', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                imu: [[0, 0, 1], [0, 0.1, 0.99]],
                emg: [
                    [0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8],
                    [0.05,0.15,0.25,0.35,0.45,0.55,0.65,0.75]
                ]
            })
        });

        const data = await response.json();

        // Update any matching DOM elements if present, otherwise log
        const romEl = document.getElementById('live-rom');
        const rmsEl = document.getElementById('live-rms');
        if (romEl) romEl.innerText = (data.liveRom ?? '--') + '°';
        if (rmsEl) rmsEl.innerText = (data.liveRms ?? '--');

        console.log('Live packet response:', data);

    } catch (error) {
        console.error("Error connecting to Python backend:", error);
    }
}

// Run this function every 500 milliseconds (0.5 seconds)
setInterval(fetchRealTimeData, 500);