#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
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
    
    button { cursor: pointer; padding: 8px 12px; border-radius: 4px; border: 1px solid #ccc; font-weight: bold; display: inline-flex; align-items: center; justify-content: center; gap: 8px; }
    .on-btn { background-color: #e7f3e7; color: #2e7d32; border: 1px solid #2e7d32; }
    .off-btn { background-color: #ffebee; color: #c62828; border: 1px solid #c62828; }
    .debug-btn { background-color: #555; color: white; margin: 10px auto; border: none; }
    .save-btn { background-color: #2196F3; color: white; border: none; padding: 5px 10px; }
    
    /* Warning Box */
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

    #status-dot { height: 12px; width: 12px; background-color: #bbb; border-radius: 50%; display: inline-block; margin-right: 5px; }
    .updated-text { font-size: 0.8em; color: #666; margin-top: 5px; }
    .excl { font-size: 1.5em; color: #d32f2f; }
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
          <span class="excl">&#9888;</span>
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

      <button class="debug-btn" onclick="toggleDebug(true)">Open Calibration & Debug</button>
    </div>

    <div id="debug-view" class="hidden">
      <h2>System Calibration</h2>
      <button class="debug-btn" onclick="toggleDebug(false)">Back to Dashboard</button>

      <div class="card" id="override-card">
        <h3>Safety Overrides</h3>
        <p>Enabling this allows pumps to run indefinitely (Service Mode).</p>
        <label style="font-size: 1.2em; cursor: pointer; display: block; padding: 10px;">
          <input type="checkbox" id="overrideCheck" onchange="toggleOverride()"> 
          Enable Manual Override
        </label>
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
        if (confirm("ARE YOU SURE? This disables the 10-second safety limit. The pumps will stay on until you manually turn them off.")) {
          setOverride(1);
        } else {
          checkBox.checked = false;
        }
      } else {
        setOverride(0);
      }
    }

    function disableOverride() {
      if(confirm("Return to safety mode? (10s limits will be re-applied)")) {
        setOverride(0);
      }
    }

    function setOverride(val) {
      fetch('/calibrate?type=override&val=' + val);
      const isAct = (val === 1);
      document.getElementById("override-warn").style.display = isAct ? "flex" : "none";
      document.getElementById("overrideCheck").checked = isAct;
      if(isAct) document.getElementById("override-card").classList.add("override-active");
      else document.getElementById("override-card").classList.remove("override-active");
    }

    function startPump(p) { 
      let t = document.getElementById("t" + p).value;
      fetch("/pump?p=" + p + "&t=" + t); 
    }

    function controlPump(a, p) { fetch("/pump?" + a + "=" + p); }

    function updateStatus() {
      fetch('/status').then(response => response.json()).then(data => {
        document.getElementById("waterLvl").innerText = data.water;
        for(let i=1; i<=4; i++) {
          let mEl = document.getElementById("m" + i);
          if(mEl) mEl.innerText = data.m[i-1];
        }
        document.getElementById("status-dot").style.backgroundColor = "#4CAF50";
        document.getElementById("conn-status").innerText = "Connected";
        document.getElementById("last-upd").innerText = new Date().toLocaleTimeString();
      }).catch(err => {
        document.getElementById("status-dot").style.backgroundColor = "#f44336";
        document.getElementById("conn-status").innerText = "Offline";
      });
    }

    setInterval(updateStatus, 2000); 
    updateStatus();
  </script>
</body>
</html>
)rawliteral";

#endif