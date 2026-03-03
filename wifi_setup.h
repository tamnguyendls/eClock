/**
 * WiFi Setup Web UI - HTML/CSS/JavaScript
 * File: wifi_setup.h
 * Purpose: Contains the complete WiFi configuration web interface
 * Usage: #include "wifi_setup.h" in eClock.ino
 */

#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

// WiFi Setup HTML Page - Served when client connects
const char WIFI_SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>eClock WiFi Setup</title>
    <link rel="icon" href="data:,">
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 25px rgba(0,0,0,0.2);
            padding: 40px;
            max-width: 400px;
            width: 100%;
        }
        
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 28px;
            text-align: center;
        }
        
        .subtitle {
            color: #666;
            text-align: center;
            margin-bottom: 30px;
            font-size: 14px;
        }
        
        .network-list {
            margin-bottom: 25px;
        }
        
        .network-item {
            padding: 12px;
            margin-bottom: 8px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s ease;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .network-item:hover {
            border-color: #667eea;
            background-color: #f5f7ff;
        }
        
        .network-item.selected {
            background-color: #667eea;
            color: white;
            border-color: #667eea;
        }
        
        .network-name {
            flex: 1;
            font-weight: 500;
        }
        
        .network-signal {
            font-size: 12px;
            opacity: 0.7;
        }
        
        .network-item.selected .network-signal {
            opacity: 1;
        }
        
        .form-group {
            margin-bottom: 20px;
        }
        
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 500;
            font-size: 14px;
        }
        
        input[type='text'],
        input[type='password'] {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 14px;
            transition: border-color 0.3s ease;
        }
        
        input[type='text']:focus,
        input[type='password']:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .button {
            width: 100%;
            padding: 12px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s ease, box-shadow 0.2s ease;
        }
        
        .button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        
        .button:disabled {
            opacity: 0.6;
            cursor: not-allowed;
            transform: none;
        }
        
        .info-box {
            background-color: #f0f4ff;
            border-left: 4px solid #667eea;
            padding: 12px;
            margin-bottom: 20px;
            border-radius: 4px;
            font-size: 13px;
            color: #333;
        }
        
        .spinner {
            display: none;
            text-align: center;
            margin: 20px 0;
        }
        
        .spinner.show {
            display: block;
        }
        
        .spinner span {
            display: inline-block;
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background: #667eea;
            margin: 0 4px;
            animation: pulse 1.4s infinite;
        }
        
        .spinner span:nth-child(2) {
            animation-delay: 0.2s;
        }
        
        .spinner span:nth-child(3) {
            animation-delay: 0.4s;
        }
        
        @keyframes pulse {
            0%, 100% { opacity: 0.3; }
            50% { opacity: 1; }
        }
        
        .error {
            color: #d32f2f;
            font-size: 12px;
            margin-top: 5px;
            display: none;
        }
        
        .success {
            color: #388e3c;
            font-size: 14px;
            text-align: center;
            margin-bottom: 20px;
            display: none;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>eClock</h1>
        <p class="subtitle">WiFi Network Setup</p>
        
        <div class="info-box">📡 Available Networks</div>
        
        <form id="wifiForm" onsubmit="handleSubmit(event)">
            <div class="network-list" id="networkList">
                <div class="spinner show" id="spinner">
                    <span></span><span></span><span></span>
                </div>
            </div>
            
            <div class="form-group">
                <label for="selectedNetwork">Selected Network:</label>
                <input type="text" id="selectedNetwork" name="ssid" readonly 
                       placeholder="Select a network above..." 
                       style="background-color: #f5f5f5; cursor: not-allowed;">
            </div>
            
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" 
                       placeholder="Enter WiFi password" autocomplete="off">
                <div class="error" id="passwordError">Password is required</div>
            </div>
            
            <button type="submit" class="button" id="connectBtn" disabled>
                Connect to Network
            </button>
            <div class="success" id="successMsg">✓ Connected successfully!</div>
            <div class="error" id="generalError"></div>
        </form>
    </div>
    
    <script>
        const networks = %NETWORKS_JSON%;
        
        function getSignalStrength(rssi) {
            if (rssi >= -50) return '📶 Excellent';
            else if (rssi >= -60) return '📶 Good';
            else if (rssi >= -70) return '📳 Fair';
            else return '📴 Weak';
        }
        
        function displayNetworks() {
            const list = document.getElementById('networkList');
            const spinner = document.getElementById('spinner');
            spinner.classList.remove('show');
            
            if (networks.length === 0) {
                list.innerHTML = '<div style="color: #999; padding: 20px; text-align: center;">No networks found. Please scan again.</div>';
                return;
            }
            
            list.innerHTML = networks.map(net => `
                <div class="network-item" onclick="selectNetwork('${net.ssid}', this)">
                    <span class="network-name">${net.ssid}</span>
                    <span class="network-signal">${getSignalStrength(net.rssi)} ${net.secure ? '🔒' : '🔓'}</span>
                </div>
            `).join('');
        }
        
        function selectNetwork(ssid, element) {
            document.querySelectorAll('.network-item').forEach(el => el.classList.remove('selected'));
            element.classList.add('selected');
            document.getElementById('selectedNetwork').value = ssid;
            document.getElementById('connectBtn').disabled = false;
            document.getElementById('password').focus();
        }
        
        function handleSubmit(event) {
            event.preventDefault();
            const ssid = document.getElementById('selectedNetwork').value;
            const password = document.getElementById('password').value;
            
            if (!ssid) {
                document.getElementById('generalError').textContent = 'Please select a network';
                document.getElementById('generalError').style.display = 'block';
                return;
            }
            
            if (!password) {
                document.getElementById('passwordError').style.display = 'block';
                return;
            }
            
            document.getElementById('connectBtn').disabled = true;
            document.getElementById('connectBtn').textContent = 'Connecting...';
            
            fetch('/connect', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
            })
            .then(response => response.text())
            .then(data => {
                document.getElementById('wifiForm').style.display = 'none';
                document.getElementById('successMsg').style.display = 'block';
                setTimeout(() => {
                    document.location.href = '/';
                }, 3000);
            })
            .catch(error => {
                document.getElementById('connectBtn').disabled = false;
                document.getElementById('connectBtn').textContent = 'Connect to Network';
                document.getElementById('generalError').textContent = 'Connection failed. Please try again.';
                document.getElementById('generalError').style.display = 'block';
            });
        }
        
        document.getElementById('password').addEventListener('input', function() {
            document.getElementById('passwordError').style.display = 'none';
            document.getElementById('generalError').style.display = 'none';
        });
        
        displayNetworks();
    </script>
</body>
</html>
)rawliteral";

#endif // WIFI_SETUP_H
