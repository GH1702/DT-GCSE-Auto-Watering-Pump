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


// BRICK 2: Full UI Body (merged from v1 + v2)
const char INDEX_HTML_BODY[] PROGMEM = R"rawliteral(
</head>
<body>
<div class="container">

  <!-- ===== NAV BAR ===== -->
  <div class="nav-group">
    <button id="nav-dash" class="nav-btn active" onclick="showView('main-view')">
      <svg class="nav-icon" viewBox="0 0 24 24"><path d="M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z"/></svg> Dashboard
    </button>
    <button id="nav-rout" class="nav-btn" onclick="showView('routine-view')">
      <svg class="nav-icon" viewBox="0 0 24 24"><path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67z"/></svg> Routines
    </button>
    <button id="nav-notif" class="nav-btn" onclick="showView('notif-view')">
      <svg class="nav-icon" viewBox="0 0 24 24"><path d="M12,22c1.1,0 2-.9 2-2h-4c0,1.1.9,2 2,2zm6-6v-5c0-3.07-1.63-5.64-4.5-6.32V4c0-.83-.67-1.5-1.5-1.5s-1.5.67-1.5,1.5v.68C7.64,5.36 6,7.92 6,11v5l-2,2v1h16v-1l-2-2zm-2 1H8v-6c0-2.48 1.51-4.5 4-4.5s4 2.02 4 4.5v6z"/></svg> Automations
    </button>
    <button id="nav-led" class="nav-btn" onclick="showView('led-view')">
      <svg class="nav-icon" viewBox="0 0 24 24"><path d="M12,2A7,7 0 0,0 5,9C5,11.38 6.19,13.47 8,14.74V17A1,1 0 0,0 9,18H15A1,1 0 0,0 16,17V14.74C17.81,13.47 19,11.38 19,9A7,7 0 0,0 12,2M9,21A1,1 0 0,0 10,22H14A1,1 0 0,0 15,21V20H9V21Z"/></svg> LED Ring
    </button>
    <button id="nav-conf" class="nav-btn" onclick="showView('debug-view')">
      <svg class="nav-icon" viewBox="0 0 24 24"><path d="M19.14,12.94c.04-.3.06-.61.06-.94c0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24,0-.43.17-.47.41l-.36,2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47,0-.59.22L3.16,8.87c-.12.21-.08.47.12.61l2.03,1.58c-.05.3-.09.63-.09.94s.04.64.09.94l-2.03,1.58c-.18.14-.23.41-.12.61l1.92,3.32c.12.22.37.29.59.22l2.39-.96c.5.38,1.03.7 1.62.94l.36,2.54c.05.24.24.41.48.41h3.84c.24,0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47,0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61L19.14,12.94z M12,15.6c-1.98,0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6s3.6,1.62 3.6,3.6S13.98,15.6 12,15.6z"/></svg> Config
    </button>
  </div>


  <!-- ===== DASHBOARD VIEW ===== -->
  <div id="main-view">
    <h2>DT GCSE Auto Watering Pump</h2>
    <div id="override-warn" class="warning-box">
      <span>&#9888; OVERRIDE ACTIVE: SAFETY OFF</span>
      <button class="revert-btn" onclick="disableOverride()">REVERT TO SAFETY</button>
    </div>

    <div class="card status-card">
      <div><span id="status-dot"></span><span id="conn-status">Connecting...</span></div>
      <div>Tank: <span id="waterLvl" class="stat">--</span>%</div>
      <div>Temp: <span id="temp" class="stat">--</span>°C</div>
      <div class="updated-text">Last Update: <span id="last-upd">--:--</span></div>
    </div>

    <div class="card">
      <table>
        <tr><th>Pump</th><th>Moisture</th><th>Timed Run (s)</th><th>Manual</th></tr>
        <tr>
          <td class="pump-label"><span id="p-dot-1" class="p-dot"></span>P1</td>
          <td><span id="m1">--</span>%</td>
          <td><input type="number" id="t1" value="10" style="width:55px"> <button onclick="startPump(1)">Go</button></td>
          <td><button class="on-btn" onclick="controlPump('on', 1)">ON</button> <button class="off-btn" onclick="controlPump('off', 1)">OFF</button></td>
        </tr>
        <tr>
          <td class="pump-label"><span id="p-dot-2" class="p-dot"></span>P2</td>
          <td><span id="m2">--</span>%</td>
          <td><input type="number" id="t2" value="10" style="width:55px"> <button onclick="startPump(2)">Go</button></td>
          <td><button class="on-btn" onclick="controlPump('on', 2)">ON</button> <button class="off-btn" onclick="controlPump('off', 2)">OFF</button></td>
        </tr>
        <tr>
          <td class="pump-label"><span id="p-dot-3" class="p-dot"></span>P3</td>
          <td><span id="m3">--</span>%</td>
          <td><input type="number" id="t3" value="10" style="width:55px"> <button onclick="startPump(3)">Go</button></td>
          <td><button class="on-btn" onclick="controlPump('on', 3)">ON</button> <button class="off-btn" onclick="controlPump('off', 3)">OFF</button></td>
        </tr>
        <tr>
          <td class="pump-label"><span id="p-dot-4" class="p-dot"></span>P4</td>
          <td><span id="m4">--</span>%</td>
          <td><input type="number" id="t4" value="10" style="width:55px"> <button onclick="startPump(4)">Go</button></td>
          <td><button class="on-btn" onclick="controlPump('on', 4)">ON</button> <button class="off-btn" onclick="controlPump('off', 4)">OFF</button></td>
        </tr>
      </table>
    </div>
  </div>


  <!-- ===== ROUTINES VIEW ===== -->
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


  <!-- ===== AUTOMATIONS VIEW ===== -->
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


  <!-- ===== LED RING VIEW ===== -->
  <div id="led-view" class="hidden">
    <h2>LED Ring Control</h2>

    <div class="card">
      <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:15px;">
        <h3 style="margin:0;">Master Power</h3>
        <div>
          <button class="on-btn" onclick="setLedPower('on')" style="padding:10px 20px;">ON</button>
          <button class="off-btn" onclick="setLedPower('off')" style="padding:10px 20px;">OFF</button>
        </div>
      </div>

      <label class="matrix-title">Ring Color</label>
      <div style="display:flex; align-items:center; gap:10px; margin-bottom:15px;">
        <input type="color" id="led-color-picker" value="#ff0000" style="height:50px; width:100px; border:none; cursor:pointer;" onchange="setLedColor(this.value)">
        <span id="hex-val" style="font-family:monospace; font-size:1.2em;">#FF0000</span>
      </div>

      <label class="matrix-title">Brightness: <span id="bright-val">128</span></label>
      <div class="range-wrapper">
        <input type="range" id="led-brightness" min="0" max="255" value="128" oninput="document.getElementById('bright-val').innerText=this.value; setLedBrightness(this.value)">
      </div>
    </div>

    <div class="card">
      <h3 style="margin-top:0;">Lighting Effects</h3>
      <div class="matrix" id="led-matrix">
        <div class="matrix-box" onclick="setLedMode('solid')">Solid</div>
        <div class="matrix-box" onclick="setLedMode('breathe')">Breathe</div>
        <div class="matrix-box" onclick="setLedMode('rainbow')">Rainbow</div>
        <div class="matrix-box" onclick="setLedMode('chase')">Chase</div>
        <div class="matrix-box" onclick="setLedMode('pulse')">Pulse</div>
        <div class="matrix-box" onclick="setLedMode('grow')">Grow Light</div>
        <div class="matrix-box" onclick="setLedMode('wave')">Wave</div>
      </div>
    </div>
  </div>


  <!-- ===== CONFIG / DEBUG VIEW ===== -->
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
        <tr>
          <td>M1</td><td><span id="raw1">--</span></td>
          <td><button class="save-btn" onclick="saveCal(1, 'wet', 'M1 Wet')">Set</button></td>
          <td><button class="save-btn" onclick="saveCal(1, 'dry', 'M1 Dry')">Set</button></td>
        </tr>
        <tr>
          <td>M2</td><td><span id="raw2">--</span></td>
          <td><button class="save-btn" onclick="saveCal(2, 'wet', 'M2 Wet')">Set</button></td>
          <td><button class="save-btn" onclick="saveCal(2, 'dry', 'M2 Dry')">Set</button></td>
        </tr>
        <tr>
          <td>M3</td><td><span id="raw3">--</span></td>
          <td><button class="save-btn" onclick="saveCal(3, 'wet', 'M3 Wet')">Set</button></td>
          <td><button class="save-btn" onclick="saveCal(3, 'dry', 'M3 Dry')">Set</button></td>
        </tr>
        <tr>
          <td>M4</td><td><span id="raw4">--</span></td>
          <td><button class="save-btn" onclick="saveCal(4, 'wet', 'M4 Wet')">Set</button></td>
          <td><button class="save-btn" onclick="saveCal(4, 'dry', 'M4 Dry')">Set</button></td>
        </tr>
      </table>
    </div>
  </div>

</div><!-- /.container -->


<!-- ===== ROUTINE MODAL ===== -->
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
        <button onclick="invert()" class="invert-btn">&#8652; Invert</button>
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


<!-- ===== AUTOMATION MODAL ===== -->
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

const char INDEX_HTML_FOOTER[] PROGMEM = "</body></html>";

#endif