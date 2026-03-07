/**
 * SECTION 1: ROLE & AUTH SELECTION
 */

// Handles the visual toggle and updates the hidden input
function setRole(role) {
    const roleInput = document.getElementById("currentRole");
    if (roleInput) roleInput.value = role;
    
    const btnPatient = document.getElementById("btn-patient");
    const btnClinician = document.getElementById("btn-clinician");
    const selector = document.getElementById("role-selector");

    if (!btnPatient || !btnClinician || !selector) return;

    if (role === 'patient') {
        btnPatient.classList.add("active");
        btnClinician.classList.remove("active");
        selector.style.transform = 'translateX(0)';
    } else {
        btnPatient.classList.remove("active");
        btnClinician.classList.add("active");
        selector.style.transform = 'translateX(100%)';
    }
}

/**
 * SECTION 2: MANUAL LOGIN / SIGNUP
 */

function signup() {
    const user = document.getElementById("username")?.value;
    const pass = document.getElementById("password")?.value;
    const role = document.getElementById("currentRole")?.value; 

    if (!user || !pass) {
        alert("Please fill in both fields.");
        return;
    }

    localStorage.setItem(`${role}_user`, user);
    localStorage.setItem(`${role}_pass`, pass);
    alert(`${role.charAt(0).toUpperCase() + role.slice(1)} account created!`);
}

function login() {
    const userInput = document.getElementById("username")?.value;
    const passInput = document.getElementById("password")?.value;
    const role = document.getElementById("currentRole")?.value; 

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
        alert("Failed to authenticate with Google. Please try again.");
    }
}

function decodeJwtResponse(token) {
    let base64Url = token.split('.')[1];
    let base64 = base64Url.replace(/-/g, '+').replace(/_/g, '/');
    let jsonPayload = decodeURIComponent(atob(base64).split('').map(function(c) {
        return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
    }).join(''));
    return JSON.parse(jsonPayload);
}

/**
 * SECTION 4: REDIRECTION & NAVIGATION
 */

function showLoginSuccess(userName, role) {
    const notification = document.getElementById("successNotification");
    const message = document.getElementById("successMessage");
    
    const targetDashboard = (role === 'clinician') ? 'clinician_dashboard.html' : 'analysis.html';
    
    if (notification && message) {
        message.textContent = `Welcome, ${userName}! Opening ${role} portal...`;
        notification.style.display = "block";
        setTimeout(() => { window.location.href = targetDashboard; }, 2000);
    } else {
        window.location.href = targetDashboard;
    }
}

// Single DOMContentLoaded event listener
document.addEventListener("DOMContentLoaded", () => {
    const isLoggedIn = localStorage.getItem("loggedIn") === "true";
    const userRole = localStorage.getItem("userRole");
    const userName = localStorage.getItem("userName");
    
    const path = window.location.pathname.toLowerCase();
    const filename = path.split('/').pop() || ''; // Get just the filename

    console.log("--- Auth Check ---");
    console.log("Full Path:", path);
    console.log("Filename:", filename);
    console.log("Logged In:", isLoggedIn);
    console.log("User Role:", userRole);
    console.log("localStorage Contents:", {...localStorage});

    // 1. Update Navigation Links
    const loginLink = document.querySelector('nav ul li a[href="login.html"]'); 
    if (isLoggedIn && loginLink) {
        loginLink.textContent = userName || "Dashboard";
        loginLink.href = (userRole === "clinician") ? "clinician_dashboard.html" : "analysis.html";
    }

    // 2. Add logout button if logged in (and it doesn't already exist)
    if (isLoggedIn) {
        const nav = document.querySelector('nav ul');
        if (nav && !document.querySelector('.logout-btn')) {
            const logoutLi = document.createElement('li');
            logoutLi.innerHTML = '<button onclick="logout()" class="logout-btn" style="background:none; border:none; color:#007bff; cursor:pointer; font-size:1rem;">Logout</button>';
            nav.appendChild(logoutLi);
        }
    }

    // 3. SECURITY GUARD - FIXED VERSION
    // Skip security checks on login page and logout page
    if (filename === 'login.html' || filename === 'logout.html') {
        console.log("On login/logout page - skipping security check");
        return;
    }

    // Check if user is not logged in at all
    if (!isLoggedIn) {
        console.log("User not logged in - redirecting to login");
        window.location.href = "login.html";
        return;
    }

    // Role-based access control
    const isClinicianPage = filename === 'clinician_dashboard.html';
    const isPatientPage = filename === 'analysis.html';

    if (isClinicianPage && userRole !== 'clinician') {
        console.warn(`Clinician page access denied. User role: ${userRole}`);
        alert("Access denied. This page is for clinicians only.");
        window.location.href = userRole === 'patient' ? 'analysis.html' : 'login.html';
        return;
    }

    if (isPatientPage && userRole !== 'patient') {
        console.warn(`Patient page access denied. User role: ${userRole}`);
        alert("Access denied. This page is for patients only.");
        window.location.href = userRole === 'clinician' ? 'clinician_dashboard.html' : 'login.html';
        return;
    }

    console.log("Access granted - staying on current page");
});

function logout() {
    console.log("Logging out...");
    localStorage.clear(); 
    window.location.href = "logout.html";
}

document.addEventListener("DOMContentLoaded", () => {
    // 1. Get the 'role' parameter from the URL (e.g., login.html?role=clinician)
    const urlParams = new URLSearchParams(window.location.search);
    const roleParam = urlParams.get('role');

    // 2. If the URL says clinician, toggle the UI automatically
    if (roleParam === 'clinician') {
        setRole('clinician');
    } else if (roleParam === 'patient') {
        setRole('patient');
    }

    // ... (Keep the rest of your existing session check logic below) ...
    const isLoggedIn = localStorage.getItem("loggedIn") === "true";
    // ...
});

async function updateDashboardData() {
    try {
        const response = await fetch('http://127.0.0.1:5001/api/live-data', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' }
        });
        const data = await response.json();

        // Update your UI elements with the backend response
        // data.liveRom = 5.8, data.liveRms = 0.48298
        if(document.getElementById('rom-display')) {
            document.getElementById('rom-display').innerText = `${data.liveRom}°`;
        }
        console.log("Live Data synced:", data);
        
    } catch (error) {
        console.error("Could not connect to the knee sleeve backend:", error);
    }
}

// Refresh data every 2 seconds
setInterval(updateDashboardData, 2000);