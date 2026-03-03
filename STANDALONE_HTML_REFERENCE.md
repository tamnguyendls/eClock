# eClock WiFi Setup UI - Standalone Code Reference

This document contains the complete HTML UI code as a standalone reference. The actual implementation is embedded in `eClock.ino` using multiple `client.println()` statements.

---

## Complete Standalone HTML

You can use this code to test the UI in a browser without the ESP8266 (for development/design purposes).

### Save as: `wifi-setup.html`

```html
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
          <span></span>
          <span></span>
          <span></span>
        </div>
      </div>
      
      <div class="form-group">
        <label for="selectedNetwork">Selected Network:</label>
        <input type="text" id="selectedNetwork" name="ssid" readonly placeholder="Select a network above..." style="background-color: #f5f5f5; cursor: not-allowed;">
      </div>
      
      <div class="form-group">
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" placeholder="Enter WiFi password" autocomplete="off">
        <div class="error" id="passwordError">Password is required</div>
      </div>
      
      <button type="submit" class="button" id="connectBtn" disabled>Connect to Network</button>
      <div class="success" id="successMsg">✓ Connected successfully!</div>
      <div class="error" id="generalError"></div>
      
    </form>
    
  </div>

  <script>
    // Sample network data (for testing without ESP8266)
    // Replace with actual WiFi.scanNetworks() output from ESP8266
    const networks = [
      { "ssid": "MyNetwork_5G", "rssi": -35, "channel": 36, "secure": true },
      { "ssid": "HomeWiFi", "rssi": -55, "channel": 6, "secure": true },
      { "ssid": "GuestNetwork", "rssi": -65, "channel": 11, "secure": false },
      { "ssid": "NeighborWiFi", "rssi": -75, "channel": 1, "secure": true }
    ];

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
      
      // For testing: simulate connection
      // In real ESP8266, this would POST to /connect
      setTimeout(() => {
        document.getElementById('wifiForm').style.display = 'none';
        document.getElementById('successMsg').style.display = 'block';
        setTimeout(() => {
          alert('In real implementation, page would redirect here');
        }, 3000);
      }, 2000);
    }

    document.getElementById('password').addEventListener('input', function() {
      document.getElementById('passwordError').style.display = 'none';
      document.getElementById('generalError').style.display = 'none';
    });

    // Simulate network scanning delay
    setTimeout(() => {
      displayNetworks();
    }, 2000);
  </script>

</body>
</html>
```

---

## How to Use This File

### For Design/Testing
1. Save the code as `wifi-setup.html`
2. Open in any web browser
3. Test the UI interactivity
4. No server required (works locally)

### For Development
1. Modify the sample `networks` array to test different scenarios
2. Update CSS colors and styling
3. Test responsive design by resizing window
4. Check browser console for any errors

### For Integration
- Use this as reference when embedding in ESP8266 code
- Each HTML line maps to one `client.println()` statement
- CSS and JavaScript are minified where possible
- All code is inline (no external files)

---

## Key JavaScript Functions Explained

### `getSignalStrength(rssi)`
Converts RSSI (Received Signal Strength Indicator) to user-friendly text
```javascript
// RSSI ranges:
// >= -50 dBm: Excellent (📶)
// -50 to -60: Good (📶)
// -60 to -70: Fair (📳)
// < -70: Weak (📴)
```

### `displayNetworks()`
Renders the network list on page load
```javascript
// 1. Hide loading spinner
// 2. Check if networks exist
// 3. Create HTML for each network
// 4. Add click handlers
```

### `selectNetwork(ssid, element)`
Handles network selection
```javascript
// 1. Remove 'selected' class from all items
// 2. Add 'selected' class to clicked item
// 3. Update the form field
// 4. Enable submit button
// 5. Focus password input
```

### `handleSubmit(event)`
Processes form submission
```javascript
// 1. Prevent default form submission
// 2. Validate required fields
// 3. Show error if invalid
// 4. Submit data to server
// 5. Show success message
// 6. Auto-redirect after 3 seconds
```

---

## CSS Breakdown

### Structure Classes
- `.container` - Main card wrapper
- `.network-list` - Network items container
- `.form-group` - Input field grouping

### Interactive Classes
- `.network-item` - Individual network button
- `.network-item:hover` - Network hover state
- `.network-item.selected` - Network selected state
- `.button:hover` - Button hover state
- `.button:disabled` - Button disabled state

### Feedback Classes
- `.spinner` - Loading animation
- `.spinner.show` - Show spinner
- `.error` - Error message styling
- `.success` - Success message styling

### Layout Classes
- `.network-name` - Network name display
- `.network-signal` - Signal strength display
- `label` - Form label
- `input` - Form inputs

---

## Network Data Structure (JSON)

The networks array contains objects with this structure:

```json
{
  "ssid": "MyNetwork",        // Network name
  "rssi": -45,                // Signal strength (-30 to -90 dBm)
  "channel": 6,               // WiFi channel (1-13 for 2.4GHz)
  "secure": true              // Encryption (true/false)
}
```

### RSSI Reference
| RSSI | Quality | Speed |
|------|---------|-------|
| -30 | Excellent | Max speed |
| -50 | Very Good | Near max speed |
| -67 | Good | Full speed |
| -70 | Fair | Reduced speed |
| -80 | Poor | Low speed |
| -90 | Very Poor | Minimal |

---

## Customization Guide

### Change Colors
```css
/* Primary gradient */
background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);

/* Change to your brand colors */
background: linear-gradient(135deg, #FF6B6B 0%, #4ECDC4 100%);
```

### Change Font
```css
font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', ...;
/* Change to */
font-family: 'Poppins', 'Roboto', sans-serif;
```

### Change Button Text
```html
<button type="submit" class="button">Connect to Network</button>
<!-- Change to -->
<button type="submit" class="button">Join Network</button>
```

### Add Logo
```html
<h1>eClock</h1>
<!-- Change to -->
<img src="logo.png" alt="eClock Logo" style="width: 100px; margin-bottom: 20px;">
<h1>eClock</h1>
```

---

## Testing Scenarios

### Test 1: No Networks
```javascript
const networks = [];
```
Expected: "No networks found" message

### Test 2: Single Network
```javascript
const networks = [
  { "ssid": "TestNetwork", "rssi": -45, "channel": 6, "secure": true }
];
```
Expected: One network displayed

### Test 3: Mixed Quality
```javascript
const networks = [
  { "ssid": "Excellent", "rssi": -30, "channel": 1, "secure": true },
  { "ssid": "Good", "rssi": -55, "channel": 6, "secure": true },
  { "ssid": "Fair", "rssi": -68, "channel": 11, "secure": false },
  { "ssid": "Weak", "rssi": -85, "channel": 1, "secure": true }
];
```
Expected: Different signal indicators for each

### Test 4: Form Validation
1. Click submit without selecting network → Error "Please select a network"
2. Select network, click submit → Error "Password is required"
3. Enter password, click submit → Success message

---

## Browser DevTools Tips

### Console Commands
```javascript
// View networks array
console.log(networks);

// Manually trigger network selection
selectNetwork('MyNetwork', document.querySelector('.network-item'));

// Clear selected network
document.getElementById('selectedNetwork').value = '';

// Check button state
console.log(document.getElementById('connectBtn').disabled);

// Simulate form submission
document.getElementById('wifiForm').dispatchEvent(new Event('submit'));
```

### Inspector
1. Right-click any element
2. Select "Inspect" or "Inspect Element"
3. View HTML structure and CSS properties
4. Test responsive design with Device Toolbar (Ctrl+Shift+M)

---

## Performance Tips

### Minified Version
For production, you can minify the CSS/JS:
- Remove comments and extra whitespace
- Reduce CSS class names
- Combine selectors
- Remove unused code

### Optimization
- Load icons from emoji (Unicode) - no external files
- Inline all CSS - no external stylesheets
- Inline all JavaScript - no external scripts
- Reduces requests and loading time

---

## Compatibility Notes

### Mobile Testing
- **iOS**: Use Safari or Chrome
- **Android**: Use Chrome
- **Desktop**: All modern browsers

### Known Issues
- Password field might autocomplete (browser feature, not UI bug)
- Emoji rendering varies by OS/browser
- Input focus keyboard behavior is browser-dependent

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | Jan 2026 | Initial UI design |
| 2.0 | Mar 2026 | Network scanning, improved styling, form validation |

---

## References

- [ESP8266 WiFi.scanNetworks()](https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.rst)
- [RSSI (Received Signal Strength Indicator)](https://en.wikipedia.org/wiki/Received_signal_strength_indication)
- [WiFi Channels](https://en.wikipedia.org/wiki/Wifi#Channels)
- [Form Validation Best Practices](https://www.smashingmagazine.com/2022/09/inline-validation-web-forms-ux/)

