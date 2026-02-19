#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

// BRICK 1: The Head and Meta tags
const char INDEX_HTML_START[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Plant Control & Automations</title>
)rawliteral";

const char HTML_STYLE_OPEN[] PROGMEM = "<style>";
const char HTML_STYLE_CLOSE[] PROGMEM = "</style>";
const char HTML_SCRIPT_OPEN[] PROGMEM = "<script>";
const char HTML_SCRIPT_CLOSE[] PROGMEM = "</script>";

// BRICK 2: Your redesigned UI Body
const char INDEX_HTML_BODY[] PROGMEM = R"rawliteral(
</head>
<body>
<div class="container">
    
  <div class="nav-group">
    <button id="nav-dash" class="nav-btn active" onclick="showView('main-view')">
      <svg class="nav-icon" viewBox="0 0 24 24"><path d="M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z"/></svg> Dashboard
    </button>
    <button id="nav-rout" class="nav-btn" onclick="showView('routine-view')">
      <svg class="nav-icon" viewBox="0 0 24 24"><path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67z"/></svg> Routines
    </button>
    <button id="nav-notif" class="nav-btn" onclick="showView('notif-view')">
      <svg class="nav-icon" viewBox="0 0 24 24">
        <path d="M12,8A4,4 0 0,1 16,12A4,4 0 0,1 12,16A4,4 0 0,1 8,12A4,4 0 0,1 12,8M12,10A2,2 0 0,0 10,12A2,2 0 0,0 12,14A2,2 0 0,0 14,12A2,2 0 0,0 12,10M10,22C9.75,22 9.54,21.82 9.5,21.58L9.13,18.93C8.5,18.68 7.96,18.34 7.44,17.94L4.95,18.95C4.73,19.03 4.46,18.95 4.34,18.73L2.34,15.27C2.21,15.05 2.27,14.78 2.46,14.63L4.57,12.97L4.5,12L4.57,11.03L2.46,9.37C2.27,9.22 2.21,8.95 2.34,8.73L4.34,5.27C4.46,5.05 4.73,4.96 4.95,5.05L7.44,6.05C7.96,5.66 8.5,5.32 9.13,5.07L9.5,2.42C9.54,2.18 9.75,2 10,2H14C14.25,2 14.46,2.18 14.5,2.42L14.87,5.07C15.5,5.32 16.04,5.66 16.56,6.05L19.05,5.05C19.27,4.96 19.54,5.05 19.66,5.27L21.66,8.73C21.79,8.95 21.73,9.22 21.54,9.37L19.43,11.03L19.5,12L19.43,12.97L21.54,14.63C21.73,14.78 21.79,15.05 21.66,15.27L19.66,18.73C19.54,18.95 19.27,19.04 19.05,18.95L16.56,17.95C16.04,18.34 15.5,18.68 14.87,18.93L14.5,21.58C14.46,21.82 14.25,22 14,22H10M11.25,4L10.88,6.61C9.68,6.86 8.62,7.5 7.85,8.39L5.44,7.35L4.69,8.65L6.8,10.2C6.4,11.37 6.4,12.64 6.8,13.8L4.68,15.36L5.43,16.66L7.86,15.62C8.63,16.5 9.68,17.14 10.87,17.38L11.24,20H12.76L13.13,17.39C14.32,17.14 15.37,16.5 16.14,15.62L18.57,16.66L19.32,15.36L17.2,13.81C17.6,12.64 17.6,11.37 17.2,10.2L19.31,8.65L18.56,7.35L16.15,8.39C15.38,7.5 14.32,6.86 13.12,6.62L12.75,4H11.25Z"/>
        <path d="M12,3 A9,9 0 0,1 21,12" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"/>
        <path d="M21,12 A9,9 0 0,1 12,21" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"/>
        <path d="M12,21 A9,9 0 0,1 3,12" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"/>
        <path d="M3,12 A9,9 0 0,1 12,3" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"/>
      </svg> Automations
    </button>
    <button id="nav-conf" class="nav-btn" onclick="showView('debug-view')">
      <svg class="nav-icon" viewBox="0 0 24 24"><path d="M22.7 19l-9.1-9.1c.9-2.3.4-5-1.5-6.9-2-2-5-2.4-7.4-1.3L9 6 6 9 1.6 4.7C.5 7.1.9 10.1 2.9 12.1c1.9 1.9 4.6 2.4 6.9 1.5l9.1 9.1c.4.4 1 .4 1.4 0l2.3-2.3c.5-.4.5-1.1.1-1.4z"/></svg> Config
    </button>
  </div>

  <div id="main-view">
    <h2>DT GCSE Auto Watering Pump</h2>
    <div id="override-warn" class="warning-box">
      <span>&#9888; OVERRIDE ACTIVE: SAFETY OFF</span>
      <button class="revert-btn" onclick="disableOverride()">REVERT TO SAFETY</button>
    </div>
    
    <div class="card status-card">
      <div><span id="status-dot"></span><span id="conn-status">Searching...</span></div>
      <div>Tank: <span id="waterLvl" class="stat">--</span>%</div>
      <div>Temp: <span id="temp" class="stat">--</span>°C</div>
      <div class="updated-text">Synced: <span id="last-upd">Never</span></div>
    </div>
    
<div class="card">
  <table>
    <tr><th>Pump</th><th>Moisture</th><th>Timed Run (s)</th><th>Manual</th></tr>
    <tr>
      <td><span id="ind0" class="indicator"></span> P1</td>
      <td><span id="m1">--</span>%</td>
      <td><input type="number" id="t1" value="10" style="width:55px"> <button onclick="startPump(1)">Go</button></td>
      <td><button class="on-btn" onclick="controlPump('on', 1)">ON</button> <button class="off-btn" onclick="controlPump('off', 1)">OFF</button></td>
    </tr>
    <tr>
      <td><span id="ind1" class="indicator"></span> P2</td>
      <td><span id="m2">--</span>%</td>
      <td><input type="number" id="t2" value="10" style="width:55px"> <button onclick="startPump(2)">Go</button></td>
      <td><button class="on-btn" onclick="controlPump('on', 2)">ON</button> <button class="off-btn" onclick="controlPump('off', 2)">OFF</button></td>
    </tr>
    <tr>
      <td><span id="ind2" class="indicator"></span> P3</td>
      <td><span id="m3">--</span>%</td>
      <td><input type="number" id="t3" value="10" style="width:55px"> <button onclick="startPump(3)">Go</button></td>
      <td><button class="on-btn" onclick="controlPump('on', 3)">ON</button> <button class="off-btn" onclick="controlPump('off', 3)">OFF</button></td>
    </tr>
    <tr>
      <td><span id="ind3" class="indicator"></span> P4</td>
      <td><span id="m4">--</span>%</td>
      <td><input type="number" id="t4" value="10" style="width:55px"> <button onclick="startPump(4)">Go</button></td>
      <td><button class="on-btn" onclick="controlPump('on', 4)">ON</button> <button class="off-btn" onclick="controlPump('off', 4)">OFF</button></td>
    </tr>
  </table>
</div>

  <div id="routine-view" class="hidden">
    <div class="card">
      <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:15px;">
        <h3 style="margin:0;">Watering Routines</h3>
        <button onclick="openModal()" style="background:var(--success); color:white; border:none; border-radius:50%; width:45px; height:45px; font-size:28px; cursor:pointer;">+</button>
      </div>
      <div id="routine-list">
        <p id="empty-msg" style="text-align:center; color:#888;">No routines created yet.</p>
      </div>
    </div>
  </div>

  <div id="notif-view" class="hidden">
    <div class="card">
      <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:15px;">
        <h3 style="margin:0;">Automations</h3>
        <button onclick="openNotifModal()" style="background:var(--success); color:white; border:none; border-radius:50%; width:45px; height:45px; font-size:28px; cursor:pointer;">+</button>
      </div>
      <div id="notif-list">
        <p id="empty-notif-msg" style="text-align:center; color:#888;">No automations yet.</p>
      </div>
    </div>
  </div>

  <div id="debug-view" class="hidden">
    <h2>System Configuration</h2>
    <div class="card" id="override-card">
      <h3>Safety Overrides</h3>
      <label style="cursor: pointer;"><input type="checkbox" id="overrideCheck" onchange="toggleOverride()"> Enable Manual Override</label>
    </div>
    
    <div class="card">
      <h3>Water Tank Calibration</h3>
      <p>Current Raw: <span id="wRaw" class="stat">--</span> cm</p>
      <button class="save-btn" onclick="saveCal(0, 'waterFull', 'Tank FULL')">Set 100%</button>
      <button class="save-btn" onclick="saveCal(0, 'waterEmpty', 'Tank EMPTY')">Set 0%</button>
    </div>
    
    <div class="card">
      <h3>Moisture Calibration (Raw)</h3>
      <table>
        <tr><th>Sensor</th><th>Raw</th><th>Wet</th><th>Dry</th></tr>
        <tr><td>M1</td><td><span id="raw1">--</span></td><td><button class="save-btn" onclick="saveCal(1, 'wet', 'M1 Wet')">Set</button></td><td><button class="save-btn" onclick="saveCal(1, 'dry', 'M1 Dry')">Set</button></td></tr>
        <tr><td>M2</td><td><span id="raw2">--</span></td><td><button class="save-btn" onclick="saveCal(2, 'wet', 'M2 Wet')">Set</button></td><td><button class="save-btn" onclick="saveCal(2, 'dry', 'M2 Dry')">Set</button></td></tr>
        <tr><td>M3</td><td><span id="raw3">--</span></td><td><button class="save-btn" onclick="saveCal(3, 'wet', 'M3 Wet')">Set</button></td><td><button class="save-btn" onclick="saveCal(3, 'dry', 'M3 Dry')">Set</button></td></tr>
        <tr><td>M4</td><td><span id="raw4">--</span></td><td><button class="save-btn" onclick="saveCal(4, 'wet', 'M4 Wet')">Set</button></td><td><button class="save-btn" onclick="saveCal(4, 'dry', 'M4 Dry')">Set</button></td></tr>
      </table>
    </div>
  </div>
</div>

<div id="modal-container" class="modal-overlay hidden">
  <div class="modal-content">
    <h3 id="modal-title">Create Routine</h3>
    
    <label class="matrix-title">Routine Name</label>
    <input type="text" id="r-name" placeholder="Routine Name" oninput="hideErrs()">
    <div id="err-name" class="error-msg">Must not be empty</div>
    <div id="err-name-exists" class="error-msg">Routine name already exists</div>
    
    <div class="btn-group">
      <button id="tab-static" class="btn-tab active" onclick="setMode('static')">Static</button>
      <button id="tab-smart" class="btn-tab" onclick="setMode('smart')">Smart</button>
    </div>
    
    <div class="day-selector" id="day-wrap">
      <button class="day-btn" onclick="toggleDay(this)">M</button>
      <button class="day-btn" onclick="toggleDay(this)">T</button>
      <button class="day-btn" onclick="toggleDay(this)">W</button>
      <button class="day-btn" onclick="toggleDay(this)">T</button>
      <button class="day-btn" onclick="toggleDay(this)">F</button>
      <button class="day-btn" onclick="toggleDay(this)">S</button>
      <button class="day-btn" onclick="toggleDay(this)">S</button>
    </div>
    <div id="err-days" class="error-msg">Select at least one day</div>
    
    <div id="content-static">
      <label class="matrix-title">Execution Time</label>
      <input type="time" id="r-time" oninput="hideErrs()">
      <div id="err-time" class="error-msg">Time cannot be blank</div>
      <label class="matrix-title" style="margin-top:10px; display:block;">Amount (ml)</label>
      <input type="number" id="r-ml" value="100" oninput="hideErrs()">
      <div id="err-ml" class="error-msg">Must be > 0</div>
    </div>
    
    <div id="content-smart" class="hidden">
      <div style="display:flex; justify-content:space-between; align-items:center;">
        <label class="matrix-title"><span id="window-label">Active Window</span>: <b id="range-text">08:00-18:00</b></label>
        <button onclick="invert()" class="invert-btn">⇌ Invert</button>
      </div>
      <div id="range-wrap" class="range-wrapper">
        <div id="slider-highlight"></div>
        <input type="range" id="s-min" min="0" max="48" value="16" oninput="updateSliders()">
        <input type="range" id="s-max" min="0" max="48" value="36" oninput="updateSliders()">
      </div>
      <label class="matrix-title">Moisture Trigger: <b id="moist-val">30</b>%</label>
      <input type="range" id="r-moist" min="0" max="100" value="30" oninput="document.getElementById('moist-val').innerText=this.value" style="width:100%;">
      <div id="err-moist" class="error-msg">Must be > 0</div>
    </div>
    
    <label class="matrix-title">Select Pump</label>
    <div class="matrix" id="p-matrix">
      <div class="matrix-box" onclick="selectMatrix('p', 1)">P1</div>
      <div class="matrix-box" onclick="selectMatrix('p', 2)">P2</div>
      <div class="matrix-box" onclick="selectMatrix('p', 3)">P3</div>
      <div class="matrix-box" onclick="selectMatrix('p', 4)">P4</div>
    </div>
    
    <div id="sensor-section" class="hidden">
      <label class="matrix-title">Select Sensor</label>
      <div class="matrix" id="s-matrix">
        <div class="matrix-box" onclick="selectMatrix('s', 1)">S1</div>
        <div class="matrix-box" onclick="selectMatrix('s', 2)">S2</div>
        <div class="matrix-box" onclick="selectMatrix('s', 3)">S3</div>
        <div class="matrix-box" onclick="selectMatrix('s', 4)">S4</div>
      </div>
    </div>
    
    <div class="btn-group">
      <button class="btn-tab" onclick="closeModal()">Cancel</button>
      <button class="btn-tab active" onclick="saveRoutineBtn()" style="background:var(--success); border-color:var(--success);">Save Routine</button>
    </div>
  </div>
</div>

<div id="notif-modal-container" class="modal-overlay hidden">
  <div class="modal-content">
    <h3 id="notif-modal-title">Create Automation</h3>
    
    <label class="matrix-title">Automation Name</label>
    <input type="text" id="notif-name" placeholder="e.g., Low Water Alert" oninput="hideNotifErrs()">
    <div id="err-notif-name" class="error-msg">Name cannot be empty</div>
    
    <label class="matrix-title">When</label>
    <select id="notif-when" class="dropdown-select" onchange="updateWhenOptions()">
      <option value="">Select trigger...</option>
      <option value="moisture_below">Moisture falls below</option>
      <option value="moisture_above">Moisture rises above</option>
      <option value="water_below">Water tank falls below</option>
      <option value="water_above">Water tank rises above</option>
      <option value="pump_starts">Pump starts</option>
      <option value="pump_stops">Pump stops</option>
      <option value="time">At specific time</option>
    </select>
    <div id="err-notif-when" class="error-msg">Select a trigger</div>
    
    <div id="when-options" class="hidden">
      <div id="when-sensor-group" class="hidden condition-group">
        <label class="matrix-title">Sensor</label>
        <select id="notif-sensor" class="dropdown-select">
          <option value="1">Sensor 1</option>
          <option value="2">Sensor 2</option>
          <option value="3">Sensor 3</option>
          <option value="4">Sensor 4</option>
        </select>
        <label class="matrix-title">Threshold (%)</label>
        <input type="number" id="notif-threshold" min="0" max="100" value="30" class="dropdown-select">
      </div>
      
      <div id="when-pump-group" class="hidden condition-group">
        <label class="matrix-title">Pump</label>
        <select id="notif-pump" class="dropdown-select">
          <option value="1">Pump 1</option>
          <option value="2">Pump 2</option>
          <option value="3">Pump 3</option>
          <option value="4">Pump 4</option>
        </select>
      </div>
      
      <div id="when-time-group" class="hidden condition-group">
        <label class="matrix-title">Time</label>
        <input type="time" id="notif-time" class="dropdown-select">
      </div>
    </div>
    
    <label class="matrix-title">If (Optional Condition)</label>
    <select id="notif-if" class="dropdown-select" onchange="updateIfOptions()">
      <option value="">No additional condition</option>
      <option value="moisture_below">Moisture is below</option>
      <option value="moisture_above">Moisture is above</option>
      <option value="water_below">Water tank is below</option>
      <option value="water_above">Water tank is above</option>
      <option value="time_between">Time is between</option>
    </select>
    
    <div id="if-options" class="hidden">
      <div id="if-sensor-group" class="hidden condition-group">
        <label class="matrix-title">Sensor</label>
        <select id="notif-if-sensor" class="dropdown-select">
          <option value="1">Sensor 1</option>
          <option value="2">Sensor 2</option>
          <option value="3">Sensor 3</option>
          <option value="4">Sensor 4</option>
        </select>
        <label class="matrix-title">Threshold (%)</label>
        <input type="number" id="notif-if-threshold" min="0" max="100" value="50" class="dropdown-select">
      </div>
      
      <div id="if-time-group" class="hidden condition-group">
        <label class="matrix-title">From</label>
        <input type="time" id="notif-if-time-from" class="dropdown-select" value="08:00">
        <label class="matrix-title">To</label>
        <input type="time" id="notif-if-time-to" class="dropdown-select" value="20:00">
      </div>
    </div>
    
    <label class="matrix-title">Do (Action)</label>
    <select id="notif-do" class="dropdown-select" onchange="updateDoOptions()">
      <option value="">Select action...</option>
      <option value="whatsapp">Send WhatsApp message</option>
      <option value="pump_on">Turn pump on</option>
      <option value="pump_off">Turn pump off</option>
    </select>
    <div id="err-notif-do" class="error-msg">Select an action</div>
    
    <div id="do-options" class="hidden">
      <div id="do-whatsapp-group" class="hidden condition-group">
        <label class="matrix-title">Message</label>
        <textarea id="notif-message" placeholder="Enter message..." class="dropdown-select" style="min-height:80px; resize:vertical;"></textarea>
        <div style="font-size:0.75em; color:#666; margin-top:5px;">
          Variables: {sensor}, {value}, {pump}, {tank}
        </div>
      </div>
      
      <div id="do-pump-group" class="hidden condition-group">
        <label class="matrix-title">Pump</label>
        <select id="notif-do-pump" class="dropdown-select">
          <option value="1">Pump 1</option>
          <option value="2">Pump 2</option>
          <option value="3">Pump 3</option>
          <option value="4">Pump 4</option>
        </select>
        <div id="do-pump-duration" class="hidden">
          <label class="matrix-title">Duration (seconds)</label>
          <input type="number" id="notif-pump-duration" min="1" value="10" class="dropdown-select">
        </div>
      </div>
    </div>
    
    <div class="btn-group">
      <button class="btn-tab" onclick="closeNotifModal()">Cancel</button>
      <button class="btn-tab active" onclick="saveNotification()" style="background:var(--success); border-color:var(--success);">Save Automation</button>
    </div>
  </div>
</div>
)rawliteral";

#endif

const char INDEX_HTML_FOOTER[] PROGMEM = "</body></html>";