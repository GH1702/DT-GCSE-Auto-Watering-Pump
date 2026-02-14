#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Plant Control & Debug</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f9; padding: 20px; color: #333; }
    .container { max-width: 800px; margin: auto; }
    .card { background: white; padding: 15px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-bottom: 20px; }
    table { width: 100%; margin-bottom: 20px; border-collapse: collapse; background: white; }
    th, td { padding: 12px; border: 1px solid #ddd; }
    th { background-color: #4CAF50; color: white; }
    .status-card { display: flex; justify-content: space-around; align-items: center; }
    .stat { font-size: 1.2em; font-weight: bold; }
    .hidden { display: none; }
    
    /* Buttons and Icons */
    button { cursor: pointer; padding: 8px 12px; border-radius: 4px; border: 1px solid #ccc; font-weight: bold; display: inline-flex; align-items: center; justify-content: center; gap: 8px; }
    .on-btn { background-color: #e7f3e7; color: #2e7d32; border: 1px solid #2e7d32; }
    .off-btn { background-color: #ffebee; color: #c62828; border: 1px solid #c62828; }
    .debug-btn { background-color: #555; color: white; margin: 10px auto; border: none; }
    .save-btn { background-color: #2196F3; color: white; border: none; padding: 5px 10px; }
    .reset-btn { background-color: #f44336; color: white; border: none; padding: 8px; }
    
    /* New Warning Box Design */
    .warning-box { 
      background-color: #ffeb3b; 
      color: #000; 
      padding: 15px; 
      border: 3px solid #f44336; 
      border-radius: 10px; 
      margin-bottom: 20px; 
      font-weight: bold; 
      display: none; 
      align-items: center; 
      justify-content: space-between;
    }
    .warning-content { display: flex; align-items: center; gap: 10px; font-size: 1.1em; }
    .revert-btn { background-color: #f44336; color: white; border: none; padding: 8px 15px; border-radius: 5px; font-size: 0.9em; }
    .override-active { border: 2px solid #f44336 !important; background-color: #fff5f5; }

    svg { width: 18px; height: 18px; fill: currentColor; }
    #status-dot { height: 12px; width: 12px; background-color: #bbb; border-radius: 50%; display: inline-block; margin-right: 5px; }
    .updated-text { font-size: 0.8em; color: #666; margin-top: 5px; }
  </style>
</head>
<body>
  <div class="container">
    
    <div id="main-view">
      <h2>DT GCSE Auto Watering</h2>
      
      <div id="override-warn" class="warning-box">
        <div class="warning-content">
          <span class="excl">&#9888;</span>
          <span>OVERRIDE ACTIVE: SAFETY OFF</span>
        </div>
        <button class="revert-btn" onclick="disableOverride()">REVERT TO SAFETY</button>
      </div>

      <div class="card status-card">
        <div>
          <span id="status-dot"></span>
          <span id="conn-status">Searching...</span>
        </div>
        <div>Tank: <span id="waterLvl" class="stat">--</span>%</div>
        <div class="updated-text">Synced: <span id="last-upd">Never</span></div>
      </div>

      <div class="card">
        <table>
          <tr><th>Pump</th><th>Moisture</th><th>Timed Run</th><th>Manual</th></tr>
          <script>
            for(let i=1; i<=4; i++) {
              document.write(`
                <tr>
                  <td>${i}</td>
                  <td><span id="m${i}">--</span>%</td>
                  <td><input type="number" id="t${i}" value="10" style="width:45px"> <button onclick="startPump(${i})">Go</button></td>
                  <td>
                    <button class="on-btn" onclick="controlPump('on', ${i})">ON</button>
                    <button class="off-btn" onclick="controlPump('off', ${i})">OFF</button>
                  </td>
                </tr>`);
            }
          </script>
        </table>
        <p style="font-size: 0.85em; color: #666;">Standard mode: 10s max safety limit applied.</p>
      </div>

      <button class="debug-btn" onclick="toggleDebug(true)">
        <svg viewBox="0 0 24 24"><path d="M22.7 19l-9.1-9.1c.9-2.3.4-5-1.5-6.9-2-2-5-2.4-7.4-1.3L9 6 6 9 1.6 4.7C.5 7.1.9 10.1 2.9 12.1c1.9 1.9 4.6 2.4 6.9 1.5l9.1 9.1c.4.4 1 .4 1.4 0l2.3-2.3c.5-.4.5-1.1.1-1.4z"/></svg>
        Open Calibration & Debug
      </button>
    </div>

    <div id="debug-view" class="hidden">
      <h2>System Calibration</h2>
      <button class="debug-btn" onclick="toggleDebug(false)">
        <svg viewBox="0 0 24 24"><path d="M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z"/></svg>
        Back to Dashboard
      </button>

      <div class="card" id="override-card">
        <h3>Safety Overrides</h3>
        <p>Warning: Enabling this allows pumps to run indefinitely.</p>
        <label style="font-size: 1.2em; cursor: pointer; display: block; padding: 10px;">
          <input type="checkbox" id="overrideCheck" onchange="toggleOverride()"> 
          Enable Manual Override
        </label>
      </div>
      
      <div class="card">
        <h3>Water Tank Calibration</h3>
        <p>Current Raw: <span id="wRaw" class="stat">--</span> cm</p>
        <div style="display:flex; justify-content:center; gap:10px; flex-wrap: wrap;">
          <button class="save-btn" onclick="saveCal(0, 'waterFull', 'Tank FULL')">Set 100%</button>
          <button class="save-btn" onclick="saveCal(0, 'waterEmpty', 'Tank EMPTY')">Set 0%</button>
        </div>
      </div>

      <div class="card">
        <h3>Moisture Calibration (Raw)</h3>
        <table>
          <tr><th>Sensor</th><th>Raw</th><th>Wet</th><th>Dry</th></tr>
          <script>
            for(let i=1; i<=4; i++) {
              document.write(`
                <tr>
                  <td>M${i}</td>
                  <td><span id="raw${i}">--</span></td>
                  <td><button class="save-btn" onclick="saveCal(${i}, 'wet', 'M${i} Wet')">Set</button></td>
                  <td><button class="save-btn" onclick="saveCal(${i}, 'dry', 'M${i} Dry')">Set</button></td>
                </tr>`);
            }
          </script>
        </table>
      </div>
    </div>
  </div>

  <script>
    function toggleDebug(show) {
      document.getElementById('main-view').classList.toggle('hidden', show);
      document.getElementById('debug-view').classList.toggle('hidden', !show);
    }

    function toggleOverride() {
      var checkBox = document.getElementById("overrideCheck");
      if (checkBox.checked) {
        if (confirm("DANGER: Disabling safety limits can flood plants. Continue?")) {
          setOverride(1);
        } else {
          checkBox.checked = false;
        }
      } else {
        setOverride(0);
      }
    }

    function disableOverride() {
      setOverride(0);
    }

    function setOverride(val) {
      fetch('/calibrate?type=override&val=' + val);
      const isAct = (val === 1);
      document.getElementById("override-warn").style.display = isAct ? "flex" : "none";
      document.getElementById("overrideCheck").checked = isAct;
      if(isAct) document.getElementById("override-card").classList.add("override-active");
      else document.getElementById("override-card").classList.remove("override-active");
    }

    function syncTime() {
      fetch("/sync?epoch=" + Math.floor(Date.now() / 1000));
    }

    function startPump(p) { 
      let t = document.getElementById("t" + p).value;
      fetch("/pump?p=" + p + "&t=" + t); 
    }

    function controlPump(a, p) { fetch("/pump?" + a + "=" + p); }

    function saveCal(id, type, label) {
      let rawVal = (type.startsWith('water')) ? 
                   document.getElementById('wRaw').innerText : 
                   document.getElementById('raw' + id).innerText;
      if(rawVal === "--") return;
      if (confirm("Save " + label + " as " + rawVal + "?")) {
        fetch(`/calibrate?sensor=${id}&type=${type}&val=${rawVal}`);
      }
    }

    function updateStatus() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          document.getElementById("waterLvl").innerText = data.water;
          for(let i=1; i<=4; i++) {
            let mEl = document.getElementById("m" + i);
            if(mEl) mEl.innerText = data.m[i-1];
            let rEl = document.getElementById("raw" + i);
            if(rEl) rEl.innerText = data.raw[i-1];
          }
          if(!document.getElementById('debug-view').classList.contains('hidden')) {
            document.getElementById("wRaw").innerText = data.waterRaw;
          }
          document.getElementById("status-dot").style.backgroundColor = "#4CAF50";
          document.getElementById("conn-status").innerText = "Connected";
          document.getElementById("last-upd").innerText = new Date().toLocaleTimeString();
        })
        .catch(err => {
          document.getElementById("status-dot").style.backgroundColor = "#f44336";
          document.getElementById("conn-status").innerText = "Offline";
        });
    }

    setInterval(updateStatus, 2000); 
    setTimeout(syncTime, 1000); 
    updateStatus();
  </script>
</body>
</html>
)rawliteral";

#endif