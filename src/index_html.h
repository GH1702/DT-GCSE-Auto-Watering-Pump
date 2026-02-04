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
    table { margin: auto; border-collapse: collapse; background: white; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
    th, td { padding: 15px; border: 1px solid #ddd; }
    th { background-color: #4CAF50; color: white; }
    button { cursor: pointer; padding: 8px 12px; border-radius: 4px; border: 1px solid #ccc; }
    input[type=number] { width: 60px; padding: 5px; }
    .on-btn { background-color: #e7f3e7; color: #2e7d32; border: 1px solid #2e7d32; }
    .off-btn { background-color: #ffebee; color: #c62828; border: 1px solid #c62828; }
    button:active { transform: translateY(2px); }
  </style>
</head>
<body>

  <h2>DT GCSE Auto Watering Pump</h2>

  <table>
    <tr>
      <th>Pump</th>
      <th>Timed Run (Seconds)</th>
      <th>Manual Control</th>
    </tr>

    <tr>
      <td>1</td>
      <td>
        <input type="number" id="t1" min="1" max="3600" value="10">
        <button onclick="startPump(1)">Start</button>
      </td>
      <td>
        <button class="on-btn" onclick="controlPump('on', 1)">ON</button>
        <button class="off-btn" onclick="controlPump('off', 1)">OFF</button>
      </td>
    </tr>

    <tr>
      <td>2</td>
      <td>
        <input type="number" id="t2" min="1" max="3600" value="10">
        <button onclick="startPump(2)">Start</button>
      </td>
      <td>
        <button class="on-btn" onclick="controlPump('on', 2)">ON</button>
        <button class="off-btn" onclick="controlPump('off', 2)">OFF</button>
      </td>
    </tr>

    <tr>
      <td>3</td>
      <td>
        <input type="number" id="t3" min="1" max="3600" value="10">
        <button onclick="startPump(3)">Start</button>
      </td>
      <td>
        <button class="on-btn" onclick="controlPump('on', 3)">ON</button>
        <button class="off-btn" onclick="controlPump('off', 3)">OFF</button>
      </td>
    </tr>

    <tr>
      <td>4</td>
      <td>
        <input type="number" id="t4" min="1" max="3600" value="10">
        <button onclick="startPump(4)">Start</button>
      </td>
      <td>
        <button class="on-btn" onclick="controlPump('on', 4)">ON</button>
        <button class="off-btn" onclick="controlPump('off', 4)">OFF</button>
      </td>
    </tr>
  </table>

  <script>
    function startPump(pump) {
      let time = document.getElementById("t" + pump).value;
      console.log("Starting pump " + pump + " for " + time + "s");
      fetch("/pump?p=" + pump + "&t=" + time);
    }

    function controlPump(action, pump) {
      console.log("Turning pump " + pump + " " + action);
      fetch("/pump?" + action + "=" + pump);
    }
  </script>

</body>
</html>
)rawliteral";

#endif