/**
 * SECTION 1: ROLE & AUTH SELECTION
 */
function setRole(role) {
    const roleInput = document.getElementById("currentRole");
    const btnPatient = document.getElementById("btn-patient");
    const btnClinician = document.getElementById("btn-clinician");
    const selector = document.getElementById("role-selector");

    if (roleInput) {
        roleInput.value = role;
    }

    // Update Visual Toggles
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
    console.log("Current active role set to:", role);
}

/**
 * SECTION 2: LOGIN, SIGNUP & REDIRECTION LOGIC
 */
function showLoginSuccess(userName, role) {
    const notification = document.getElementById("successNotification");
    const message = document.getElementById("successMessage");

    // STRICT ROUTING
    const targetDashboard = (role === 'clinician') ? 'clinician_home.html' : 'patient_home.html';

    if (notification && message) {
        message.textContent = `Welcome, ${userName}! Opening ${role} portal...`;
        notification.style.display = "block";
        setTimeout(() => { window.location.href = targetDashboard; }, 2000);
    } else {
        window.location.href = targetDashboard;
    }
}

function login() {
    const userInput = document.getElementById("username")?.value;
    const passInput = document.getElementById("password")?.value;
    const roleInput = document.getElementById("currentRole");
    
    if (!roleInput || !roleInput.value) {
        alert("Please select a role (Patient or Clinician) first.");
        return;
    }
    
    const role = roleInput.value;

    if (!userInput || !passInput) {
        alert("Please fill in both fields.");
        return;
    }

    const savedUser = localStorage.getItem(`${role}_user`);
    const savedPass = localStorage.getItem(`${role}_pass`);

    if (userInput === savedUser && passInput === savedPass) {
        const userName = userInput.split('@')[0];
        localStorage.setItem("loggedIn", "true");
        localStorage.setItem("userRole", role);
        localStorage.setItem("userName", userName);
        showLoginSuccess(userName, role);
    } else {
        alert(`Invalid ${role} credentials. Please check your role or sign up.`);
    }
}

function signup() {
    const user = document.getElementById("username")?.value;
    const pass = document.getElementById("password")?.value;
    const roleInput = document.getElementById("currentRole");
    const role = roleInput ? roleInput.value : 'patient';

    if (!user || !pass) {
        alert("Please fill in both fields.");
        return;
    }

    localStorage.setItem(`${role}_user`, user);
    localStorage.setItem(`${role}_pass`, pass);
    alert(`${role.charAt(0).toUpperCase() + role.slice(1)} account created! You can now log in.`);
}

/**
 * SECTION 3: GOOGLE OAUTH
 */
function handleCredentialResponse(response) {
    try {
        const responsePayload = decodeJwtResponse(response.credential);
        const role = document.getElementById("currentRole")?.value || 'patient';

        localStorage.setItem("loggedIn", "true");
        localStorage.setItem("userRole", role);
        localStorage.setItem("userName", responsePayload.name);
        localStorage.setItem("userEmail", responsePayload.email);

        showLoginSuccess(responsePayload.name, role);
    } catch (error) {
        console.error("Google Sign-In error:", error);
    }
}

function decodeJwtResponse(token) {
    let base64Url = token.split('.')[1];
    let base64 = base64Url.replace(/-/g, '+').replace(/_/g, '/');
    let jsonPayload = decodeURIComponent(atob(base64).split('').map(function (c) {
        return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
    }).join(''));
    return JSON.parse(jsonPayload);
}

/**
 * SECTION 4: GLOBAL LOGOUT & SECURITY
 */
function logout() {
    localStorage.clear();
    window.location.href = "logout.html";
}

/**
 * SECTION 5: INITIALIZATION & DATA SYNC
 */
document.addEventListener("DOMContentLoaded", () => {
    const isLoggedIn = localStorage.getItem("loggedIn") === "true";
    const userRole = localStorage.getItem("userRole");
    const userName = localStorage.getItem("userName");
    
    // Extract clean filename without query strings
    const path = window.location.pathname.toLowerCase();
    const filename = path.split('/').pop().split('?')[0] || 'index.html';

    // 1. Handle URL parameters
    const urlParams = new URLSearchParams(window.location.search);
    if (urlParams.get('role')) {
        setRole(urlParams.get('role'));
    }

    // 2. Update Navigation
    const loginLink = document.querySelector('nav ul li a[href="login.html"]');
    if (isLoggedIn && loginLink) {
        loginLink.textContent = userName || "Dashboard";
        loginLink.href = (userRole === "clinician") ? "clinician_home.html" : "patient_home.html";
    }

    // 3. Security Guard
    const publicPages = ['login.html', 'logout.html', 'home.html', 'index.html', '', 'about.html', 'video.html'];
    if (!publicPages.includes(filename) && !isLoggedIn) {
        window.location.href = "login.html";
    }

    // 4. Live Data Sync (For Patient Dashboard)
    async function updateDashboardData() {
        if (!window.location.pathname.includes('patient_home.html')) return;
        
        try {
            const response = await fetch('http://127.0.0.1:5001/api/live-data', { method: 'POST' });
            const data = await response.json();
            const romDisplay = document.getElementById('rom-display');
            if (romDisplay) {
                romDisplay.innerText = `${data.liveRom}°`;
            }
        } catch (e) { /* Backend offline */ }
    }

    if (isLoggedIn && userRole === 'patient') {
        setInterval(updateDashboardData, 2000);
    }
});