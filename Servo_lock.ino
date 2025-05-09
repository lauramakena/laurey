#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Servo configuration
const int servoPin = 13;  // GPIO pin for servo
const int LOCK_POSITION = 0;    // Degrees for locked position
const int UNLOCK_POSITION = 90;  // Degrees for unlocked position

WebServer server(80);
Servo lockServo;
bool isLocked = true;

void handleRoot() {
  String html = "<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Smart Lock</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f0f0f0;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            margin: 0;
            color: #333;
        }
        
        .lock-container {
            background-color: white;
            border-radius: 15px;
            padding: 30px;
            box-shadow: 0 10px 20px rgba(0,0,0,0.1);
            width: 300px;
            text-align: center;
        }
        
        .status-display {
            margin: 20px 0;
            font-size: 18px;
            font-weight: bold;
        }
        
        .locked { color: #d9534f; }
        .unlocked { color: #5cb85c; }
        
        .pin-display {
            display: flex;
            justify-content: center;
            margin: 20px 0;
            gap: 10px;
        }
        
        .pin-circle {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            border: 2px solid #ccc;
            transition: all 0.2s;
        }
        
        .pin-circle.filled {
            background-color: #2c3e50;
            border-color: #2c3e50;
        }
        
        .keypad {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 15px;
            margin-top: 20px;
        }
        
        .key {
            padding: 15px;
            font-size: 20px;
            border: none;
            border-radius: 50%;
            background-color: #f0f0f0;
            cursor: pointer;
            transition: all 0.2s;
        }
        
        .key:hover { background-color: #e0e0e0; }
        .key:active { transform: scale(0.95); }
        
        .action-btn {
            padding: 10px 20px;
            margin-top: 20px;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            cursor: pointer;
            transition: all 0.2s;
        }
        
        .lock-btn { background-color: #d9534f; color: white; }
        
        .error-message {
            color: #d9534f;
            margin-top: 10px;
            height: 20px;
        }
        
        .lockout-display {
            color: #d9534f;
            font-weight: bold;
            margin: 20px 0;
        }
        
        .timeout-display {
            margin-top: 15px;
            font-size: 14px;
            color: #666;
        }
        
        .attempts-display {
            margin-top: 10px;
            font-size: 14px;
            color: #666;
        }
        
        .modal {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background-color: rgba(0,0,0,0.8);
            display: flex;
            justify-content: center;
            align-items: center;
            z-index: 1000;
        }
        
        .modal-content {
            background-color: white;
            padding: 30px;
            border-radius: 15px;
            max-width: 400px;
            text-align: center;
            animation: fadeIn 0.5s;
        }
        
        .modal-title { font-size: 24px; font-weight: bold; margin-bottom: 15px; }
        .success-title { color: #5cb85c; }
        .modal-text { margin-bottom: 20px; line-height: 1.5; }
        
        .modal-btn {
            padding: 10px 20px;
            background-color: #5cb85c;
            color: white;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 16px;
            transition: all 0.2s;
        }
        
        .modal-btn:hover { background-color: #4cae4c; }
        .hidden { display: none; }
        .disabled { opacity: 0.5; pointer-events: none; }
        
        @keyframes fadeIn {
            from { opacity: 0; transform: scale(0.9); }
            to { opacity: 1; transform: scale(1); }
        }
        
        @keyframes shake {
            0%, 100% { transform: translateX(0); }
            20%, 60% { transform: translateX(-5px); }
            40%, 80% { transform: translateX(5px); }
        }
    </style>
</head>
<body>
    <div id="welcomeModal" class="modal">
        <div class="modal-content">
            <div class="modal-title">ESP32 Smart Lock</div>
            <div class="modal-text">
                <p>Welcome to your smart lock system</p>
                <p>Please enter your PIN to unlock the device</p>
            </div>
            <button id="closeWelcome" class="modal-btn">Continue</button>
        </div>
    </div>

    <div id="successMessage" class="modal hidden">
        <div class="modal-content success-message">
            <div class="modal-title success-title">
                <i class="fas fa-check-circle"></i> Access Granted
            </div>
            <div class="modal-text">
                Device unlocked successfully<br>
                <small>Auto-locking in <span id="successCountdown">30</span> seconds</small>
            </div>
            <button id="closeSuccess" class="modal-btn">OK</button>
        </div>
    </div>

    <div class="lock-container">
        <div class="title">ESP32 Smart Lock</div>
        
        <div class="status-display locked">
            <i class="fas fa-lock"></i> Locked
        </div>
        
        <div id="pinEntry">
            <div class="pin-display">
                <div class="pin-circle" id="pin1"></div>
                <div class="pin-circle" id="pin2"></div>
                <div class="pin-circle" id="pin3"></div>
                <div class="pin-circle" id="pin4"></div>
            </div>
            
            <div class="attempts-display">
                Attempts remaining: <span id="attemptsRemaining">3</span>
            </div>
            
            <div class="keypad">
                <button class="key" data-value="1">1</button>
                <button class="key" data-value="2">2</button>
                <button class="key" data-value="3">3</button>
                <button class="key" data-value="4">4</button>
                <button class="key" data-value="5">5</button>
                <button class="key" data-value="6">6</button>
                <button class="key" data-value="7">7</button>
                <button class="key" data-value="8">8</button>
                <button class="key" data-value="9">9</button>
                <button class="key" id="clearBtn"><i class="fas fa-backspace"></i></button>
                <button class="key" data-value="0">0</button>
            </div>
            
            <div class="error-message" id="errorMsg"></div>
            <div class="lockout-display hidden" id="lockoutMsg"></div>
        </div>
        
        <div id="unlockedControls" class="hidden">
            <button id="lockBtn" class="action-btn lock-btn">
                <i class="fas fa-lock"></i> Lock Device
            </button>
            <div class="timeout-display">
                Auto-locking in: <span id="timeoutDisplay">30</span> seconds
            </div>
        </div>
    </div>

    <script>
        // Configuration
        const CORRECT_PIN = "1234";
        const AUTO_LOCK_TIMEOUT = 30;
        const MAX_ATTEMPTS = 3;
        const LOCKOUT_INCREMENT = 30;
        
        // State variables
        let enteredPin = "";
        let timeoutTimer;
        let remainingTime = AUTO_LOCK_TIMEOUT;
        let attemptsRemaining = MAX_ATTEMPTS;
        let lockoutTimer;
        let isLockedOut = false;
        
        // DOM Elements
        const pinCircles = document.querySelectorAll('.pin-circle');
        const errorMsg = document.getElementById('errorMsg');
        const lockoutMsg = document.getElementById('lockoutMsg');
        const pinEntrySection = document.getElementById('pinEntry');
        const unlockedControls = document.getElementById('unlockedControls');
        const statusDisplay = document.querySelector('.status-display');
        const timeoutDisplay = document.getElementById('timeoutDisplay');
        const attemptsDisplay = document.getElementById('attemptsRemaining');
        const keypad = document.querySelector('.keypad');
        const welcomeModal = document.getElementById('welcomeModal');
        const closeWelcome = document.getElementById('closeWelcome');
        const successMsg = document.getElementById('successMessage');
        const closeSuccess = document.getElementById('closeSuccess');
        const successCountdown = document.getElementById('successCountdown');
        
        // Initialize
        document.addEventListener('DOMContentLoaded', () => {
            updateAttemptsDisplay();
            
            // Keypad functionality
            document.querySelectorAll('.key:not(#clearBtn)').forEach(key => {
                key.addEventListener('click', () => {
                    if (!isLockedOut && enteredPin.length < 4) {
                        enteredPin += key.dataset.value;
                        updatePinDisplay();
                    }
                });
            });
            
            // Clear button
            document.getElementById('clearBtn').addEventListener('click', () => {
                if (!isLockedOut) {
                    enteredPin = "";
                    updatePinDisplay();
                    errorMsg.textContent = "";
                }
            });
            
            // Lock button
            document.getElementById('lockBtn').addEventListener('click', lockDevice);
            
            // Keyboard support
            document.addEventListener('keydown', (e) => {
                if (isLockedOut) return;
                
                if (e.key >= '0' && e.key <= '9' && enteredPin.length < 4) {
                    enteredPin += e.key;
                    updatePinDisplay();
                } else if (e.key === 'Backspace') {
                    enteredPin = "";
                    updatePinDisplay();
                    errorMsg.textContent = "";
                } else if (e.key === 'Enter') {
                    verifyPin();
                }
            });
            
            // Welcome modal
            closeWelcome.addEventListener('click', () => {
                welcomeModal.style.display = 'none';
            });
            
            // Success message
            closeSuccess.addEventListener('click', () => {
                successMsg.classList.add('hidden');
            });
            
            // Check lock status every 5 seconds
            setInterval(checkLockStatus, 5000);
        });
        
        function updatePinDisplay() {
            pinCircles.forEach((circle, index) => {
                circle.classList.toggle('filled', index < enteredPin.length);
            });
            
            if (enteredPin.length === 4 && !isLockedOut) {
                setTimeout(verifyPin, 100);
            }
        }
        
        function updateAttemptsDisplay() {
            attemptsDisplay.textContent = attemptsRemaining;
        }
        
        function verifyPin() {
            if (isLockedOut) return;
            if (enteredPin.length !== 4) return;
            
            if (enteredPin === CORRECT_PIN) {
                unlockDevice();
                attemptsRemaining = MAX_ATTEMPTS;
                updateAttemptsDisplay();
            } else {
                handleFailedAttempt();
            }
        }
        
        function handleFailedAttempt() {
            attemptsRemaining--;
            updateAttemptsDisplay();
            
            if (attemptsRemaining > 0) {
                errorMsg.textContent = `Incorrect PIN. ${attemptsRemaining} attempt${attemptsRemaining > 1 ? 's' : ''} remaining.`;
            } else {
                const lockoutTime = LOCKOUT_INCREMENT * (MAX_ATTEMPTS - attemptsRemaining);
                startLockout(lockoutTime);
            }
            
            enteredPin = "";
            updatePinDisplay();
            pinEntrySection.style.animation = 'shake 0.5s';
            setTimeout(() => pinEntrySection.style.animation = '', 500);
        }
        
        function startLockout(lockoutTime) {
            isLockedOut = true;
            lockoutMsg.textContent = `Too many attempts. Try again in ${lockoutTime} seconds.`;
            lockoutMsg.classList.remove('hidden');
            keypad.classList.add('disabled');
            
            let remainingLockout = lockoutTime;
            lockoutTimer = setInterval(() => {
                remainingLockout--;
                lockoutMsg.textContent = `Too many attempts. Try again in ${remainingLockout} seconds.`;
                
                if (remainingLockout <= 0) {
                    clearInterval(lockoutTimer);
                    isLockedOut = false;
                    attemptsRemaining = MAX_ATTEMPTS;
                    updateAttemptsDisplay();
                    lockoutMsg.classList.add('hidden');
                    keypad.classList.remove('disabled');
                    errorMsg.textContent = "";
                }
            }, 1000);
        }
        
        function unlockDevice() {
            fetch('/unlock')
                .then(response => {
                    if (!response.ok) throw new Error('Network response was not ok');
                    return response.text();
                })
                .then(data => {
                    console.log("Unlock response:", data);
                    
                    // Show success message
                    successMsg.classList.remove('hidden');
                    let countdown = AUTO_LOCK_TIMEOUT;
                    successCountdown.textContent = countdown;
                    
                    const countdownInterval = setInterval(() => {
                        countdown--;
                        successCountdown.textContent = countdown;
                        if (countdown <= 0) clearInterval(countdownInterval);
                    }, 1000);
                    
                    // Update UI
                    pinEntrySection.classList.add('hidden');
                    unlockedControls.classList.remove('hidden');
                    statusDisplay.classList.replace('locked', 'unlocked');
                    statusDisplay.innerHTML = '<i class="fas fa-lock-open"></i> Unlocked';
                    
                    // Start auto-lock timer
                    remainingTime = AUTO_LOCK_TIMEOUT;
                    updateTimeoutDisplay();
                    timeoutTimer = setInterval(() => {
                        remainingTime--;
                        updateTimeoutDisplay();
                        if (remainingTime <= 0) {
                            successMsg.classList.add('hidden');
                            lockDevice();
                        }
                    }, 1000);
                })
                .catch(error => {
                    console.error("Unlock error:", error);
                    errorMsg.textContent = "Failed to communicate with device";
                });
        }
        
        function lockDevice() {
            fetch('/lock')
                .then(response => {
                    if (!response.ok) throw new Error('Network response was not ok');
                    return response.text();
                })
                .then(data => {
                    console.log("Lock response:", data);
                    clearInterval(timeoutTimer);
                    enteredPin = "";
                    updatePinDisplay();
                    errorMsg.textContent = "";
                    
                    pinEntrySection.classList.remove('hidden');
                    unlockedControls.classList.add('hidden');
                    statusDisplay.classList.replace('unlocked', 'locked');
                    statusDisplay.innerHTML = '<i class="fas fa-lock"></i> Locked';
                })
                .catch(error => {
                    console.error("Lock error:", error);
                    errorMsg.textContent = "Failed to communicate with device";
                });
        }
        
        function updateTimeoutDisplay() {
            timeoutDisplay.textContent = remainingTime;
        }
        
        function checkLockStatus() {
            fetch('/checkStatus')
                .then(response => response.text())
                .then(data => {
                    if (data === "unlocked" && statusDisplay.classList.contains('locked')) {
                        // Device was unlocked elsewhere
                        unlockDevice();
                    } else if (data === "locked" && statusDisplay.classList.contains('unlocked')) {
                        // Device was locked elsewhere
                        lockDevice();
                    }
                })
                .catch(error => console.error("Status check error:", error));
        }
    </script>
</body>
</html>
  server.send(200, "text/html", html);
}

void handleLock() {
  lockServo.write(LOCK_POSITION);
  isLocked = true;
  server.send(200, "text/plain", "Locked");
  Serial.println("Device locked");
}

void handleUnlock() {
  lockServo.write(UNLOCK_POSITION);
  isLocked = false;
  server.send(200, "text/plain", "Unlocked");
  Serial.println("Device unlocked");
}

void handleStatusCheck() {
  server.send(200, "text/plain", isLocked ? "locked" : "unlocked");
}

void setup() {
  Serial.begin(115200);
  
  // Initialize servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  lockServo.setPeriodHertz(50);  // Standard 50hz servo
  lockServo.attach(servoPin, 500, 2400);
  lockServo.write(LOCK_POSITION);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  
  // Set up server routes
  server.on("/", handleRoot);
  server.on("/lock", handleLock);
  server.on("/unlock", handleUnlock);
  server.on("/checkStatus", handleStatusCheck);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}