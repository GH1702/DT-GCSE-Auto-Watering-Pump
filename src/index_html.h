#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Plant Control</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 30px; background-color: #f4f4f9; }
    .container { max-width: 600px; margin: auto; }
    table { width: 100%; margin-bottom: 20px; border-collapse: collapse; background: white; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
    th, td { padding: 12px; border: 1px solid #ddd; }
    th { background-color: #4CAF50; color: white; }
    .status-card { display: flex; justify-content: space-around; align-items: center; background: #fff; padding: 15px; margin-bottom: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
    .stat { font-size: 1.2em; font-weight: bold; }
    #conn-icon { font-size: 24px; margin-right: 10px; }
    .online { color: #4CAF50; }
    .offline { color: #f44336; }
    .updated-text { font-size: 0.8em; color: #666; }
    button { cursor: pointer; padding: 8px 12px; border-radius: 4px; border: 1px solid #ccc; }
    .on-btn { background-color: #e7f3e7; color: #2e7d32; border: 1px solid #2e7d32; }
    .off-btn { background-color: #ffebee; color: #c62828; border: 1px solid #c62828; }
  </style>
</head>
<body>
  <div class="container">
    <h2>DT GCSE Auto Watering</h2>

<div class="status-card">
  <div>
    <span id="status-dot" style="height: 10px; width: 10px; background-color: #bbb; border-radius: 50%; display: inline-block;"></span>
    <span id="conn-status">Searching...</span>
  </div>
  <div>Tank: <span id="waterLvl" class="stat">--</span>%</div>
  <div class="updated-text">Updated: <span id="last-upd">Never</span></div>
</div>

    <table>
      <tr><th>Pump</th><th>Moisture</th><th>Timed Run</th><th>Manual</th></tr>
      <tr><td>1</td><td><span id="m1">--</span>%</td><td><input type="number" id="t1" value="10" style="width:40px"> <button onclick="startPump(1)">Go</button></td><td><button class="on-btn" onclick="controlPump('on', 1)">ON</button><button class="off-btn" onclick="controlPump('off', 1)">OFF</button></td></tr>
      <tr><td>2</td><td><span id="m2">--</span>%</td><td><input type="number" id="t2" value="10" style="width:40px"> <button onclick="startPump(2)">Go</button></td><td><button class="on-btn" onclick="controlPump('on', 2)">ON</button><button class="off-btn" onclick="controlPump('off', 2)">OFF</button></td></tr>
      <tr><td>3</td><td><span id="m3">--</span>%</td><td><input type="number" id="t3" value="10" style="width:40px"> <button onclick="startPump(3)">Go</button></td><td><button class="on-btn" onclick="controlPump('on', 3)">ON</button><button class="off-btn" onclick="controlPump('off', 3)">OFF</button></td></tr>
      <tr><td>4</td><td><span id="m4">--</span>%</td><td><input type="number" id="t4" value="10" style="width:40px"> <button onclick="startPump(4)">Go</button></td><td><button class="on-btn" onclick="controlPump('on', 4)">ON</button><button class="off-btn" onclick="controlPump('off', 4)">OFF</button></td></tr>
    </table>
  </div>

<script>
    function syncTime() {
      let epoch = Math.floor(Date.now() / 1000);
      fetch("/sync?epoch=" + epoch).then(() => console.log("Time Synced"));
    }

    function startPump(p) { 
      let t = document.getElementById("t" + p).value;
      fetch("/pump?p=" + p + "&t=" + t); 
    }

    function controlPump(a, p) { fetch("/pump?" + a + "=" + p); }

    function updateStatus() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          document.getElementById("waterLvl").innerText = data.water;
          document.getElementById("m1").innerText = data.m[0];
          document.getElementById("m2").innerText = data.m[1];
          document.getElementById("m3").innerText = data.m[2];
          document.getElementById("m4").innerText = data.m[3];
          
          // Connection success
          document.getElementById("status-dot").style.backgroundColor = "#4CAF50";
          document.getElementById("conn-status").innerText = "Connected";
          document.getElementById("last-upd").innerText = new Date().toLocaleTimeString();
        })
        .catch(err => {
          // Connection failed
          document.getElementById("status-dot").style.backgroundColor = "#f44336";
          document.getElementById("conn-status").innerText = "Offline";
        });
    }

    // Initialize
    setInterval(updateStatus, 2000); 
    setTimeout(syncTime, 1000); // Sync laptop time 1 sec after load
    updateStatus();
  </script>

// Run sync 1 second after page load
setTimeout(syncTime, 1000);
    }

    setInterval(updateStatus, 2000); 
    updateStatus();
  </script>
</body>
</html>
)rawliteral";

#endif