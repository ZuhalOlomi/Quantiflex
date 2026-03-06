/**
 * SECTION 1: MANUAL LOGIN / SIGNUP (Local Storage)
 */

// Saves credentials to browser memory
function signup() {
    const user = document.getElementById("username").value;
    const pass = document.getElementById("password").value;

    if (user && pass) {
        localStorage.setItem("localUser", user);
        localStorage.setItem("localPass", pass);
        alert("Account created successfully! You can now log in.");
    } else {
        alert("Please fill in both fields.");
    }
}

// Checks input against browser memory
function login() {
    const userInput = document.getElementById("username").value;
    const passInput = document.getElementById("password").value;

    const savedUser = localStorage.getItem("localUser");
    const savedPass = localStorage.getItem("localPass");

    if (userInput === savedUser && passInput === savedPass) {
        localStorage.setItem("loggedIn", "true");
        localStorage.setItem("userName", userInput.split('@')[0]); // Use email prefix as name
        showLoginSuccess(userInput.split('@')[0]);
    } else {
        alert("Invalid username or password. (Did you Sign Up first?)");
    }
}

/**
 * SECTION 2: GOOGLE OAUTH LOGIC
 */

// This runs automatically after a successful Google login
function handleCredentialResponse(response) {
    // Decode the secure token from Google
    const responsePayload = decodeJwtResponse(response.credential);

    // Store Google details
    localStorage.setItem("loggedIn", "true");
    localStorage.setItem("userName", responsePayload.name);
    localStorage.setItem("userEmail", responsePayload.email);
    localStorage.setItem("userPic", responsePayload.picture);

    showLoginSuccess(responsePayload.name);
}

// Helper function to decode Google's encrypted user data
function decodeJwtResponse(token) {
    let base64Url = token.split('.')[1];
    let base64 = base64Url.replace(/-/g, '+').replace(/_/g, '/');
    let jsonPayload = decodeURIComponent(atob(base64).split('').map(function(c) {
        return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
    }).join(''));

    return JSON.parse(jsonPayload);
}

/**
 * Show login success confirmation and redirect
 */
function showLoginSuccess(userName) {
    const notification = document.getElementById("successNotification");
    const message = document.getElementById("successMessage");
    
    if (notification) {
        message.textContent = `Welcome, ${userName}! Redirecting to your dashboard...`;
        notification.style.display = "block";
        
        // Redirect after 2 seconds
        setTimeout(function() {
            window.location.href = "home.html";
        }, 2000);
    } else {
        // Fallback if notification element is not found
        window.location.href = "home.html";
    }
}

/**
 * SECTION 3: SESSION & UI MANAGEMENT
 */

// Runs on every page load to check if the user is logged in
document.addEventListener("DOMContentLoaded", () => {
    const isLoggedIn = localStorage.getItem("loggedIn") === "true";
    const userName = localStorage.getItem("userName");

    // If on a page with a login button, change it to the user's name
    const loginLink = document.querySelector('nav ul li a[href="login.html"]'); 
    if (isLoggedIn && loginLink) {
        loginLink.textContent = userName || "Dashboard";
        loginLink.href = "analysis.html";
    }
});

function logout() {
    localStorage.clear(); // Clears all login data
    window.location.href = "home.html";
}