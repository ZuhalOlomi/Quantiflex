/**
 * SECTION 1: ROLE SELECTION
 */
function setRole(role) {
    const roleInput = document.getElementById("currentRole");
    const btnPatient = document.getElementById("btn-patient");
    const btnClinician = document.getElementById("btn-clinician");
    const selector = document.getElementById("role-selector");

    if (roleInput) roleInput.value = role;

    if (btnPatient && btnClinician) {
        if (role === 'patient') {
            btnPatient.classList.add("active");
            btnClinician.classList.remove("active");
            if (selector) selector.style.transform = 'translateX(0)';
        } else {
            btnPatient.classList.remove("active");
            btnClinician.classList.add("active");
            if (selector) selector.style.transform = 'translateX(100%)';
        }
    }
}

/**
 * SECTION 2: AUTHENTICATION
 */
function login() {
    const userInput = document.getElementById("username")?.value;
    const passInput = document.getElementById("password")?.value;
    const role = document.getElementById("currentRole")?.value;
    
    if (!userInput || !passInput) {
        alert("Please fill in all fields.");
        return;
    }

    const savedUser = localStorage.getItem(`${role}_user`);
    const savedPass = localStorage.getItem(`${role}_pass`);

    if (userInput === savedUser && passInput === savedPass) {
        localStorage.setItem("loggedIn", "true");
        localStorage.setItem("userRole", role);
        localStorage.setItem("userName", userInput.split('@')[0]);
        
        // Redirect logic
        const target = (role === 'clinician') ? 'clinician_home.html' : 'patient_home.html';
        window.location.href = target;
    } else {
        alert("Invalid credentials for the selected role.");
    }
}

function signup() {
    const user = document.getElementById("username")?.value;
    const pass = document.getElementById("password")?.value;
    const role = document.getElementById("currentRole")?.value || 'patient';

    if (!user || !pass) return alert("Fill in fields.");
    
    localStorage.setItem(`${role}_user`, user);
    localStorage.setItem(`${role}_pass`, pass);
    alert(`${role} account created!`);
}

function logout() {
    localStorage.clear();
    window.location.href = "index.html";
}

/**
 * SECTION 3: SECURITY & INITIALIZATION
 */
document.addEventListener("DOMContentLoaded", () => {
    const isLoggedIn = localStorage.getItem("loggedIn") === "true";
    const userRole = localStorage.getItem("userRole");
    const path = window.location.pathname.toLowerCase();
    const filename = path.split('/').pop().split('?')[0] || 'index.html';

    // 1. Navigation Update
    const loginLink = document.querySelector('nav ul li a[href="login.html"]');
    if (loginLink) {
        if (isLoggedIn) {
            loginLink.textContent = "Dashboard";
            loginLink.href = (userRole === "clinician") ? "clinician_home.html" : "patient_home.html";
        } else {
            loginLink.textContent = "Login";
            loginLink.href = "login.html";
        }
    }

    // 2. The Security Guard
    const publicPages = ['login.html', 'index.html', 'about.html', 'video.html', ''];
    if (!publicPages.includes(filename) && !isLoggedIn) {
        window.location.href = "login.html";
        return;
    }

    // 3. Role-Based Access Control
    if (filename === 'clinician_home.html' && userRole !== 'clinician') {
        window.location.href = "patient_home.html";
    }
    if (filename === 'patient_home.html' && userRole !== 'patient') {
        window.location.href = "clinician_home.html";
    }

    // 4. Live Data Polling (Patient Only)
    if (isLoggedIn && userRole === 'patient' && filename === 'patient_home.html') {
        setInterval(async () => {
            try {
                const response = await fetch('http://127.0.0.1:5001/api/live-data', { method: 'POST' });
                const data = await response.json();
                if(document.getElementById('rom-display')) {
                    document.getElementById('rom-display').innerText = `${data.liveRom}°`;
                }
            } catch (e) { console.log("Sleeve disconnected"); }
        }, 2000);
    }
});