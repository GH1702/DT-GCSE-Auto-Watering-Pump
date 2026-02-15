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
    :root { --primary: #2196F3; --success: #4CAF50; --danger: #f44336; --dark: #333; --gray: #ddd; --text-gray: #555; }
    body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f9; padding: 20px; color: var(--dark); margin: 0; }
    .container { max-width: 800px; margin: auto; }
    .card { background: white; padding: 15px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-bottom: 20px; text-align: left; }
    .hidden { display: none !important; }
    
    /* Navigation */
    .nav-group { margin-bottom: 20px; display: flex; justify-content: center; gap: 10px; flex-wrap: wrap; }
    .nav-btn { cursor: pointer; padding: 10px 18px; border-radius: 4px; border: none; font-weight: bold; display: inline-flex; align-items: center; justify-content: center; gap: 8px; background-color: #555; color: white; transition: 0.2s; }
    .nav-btn:hover { background-color: #333; }
    .nav-btn.active { background-color: var(--primary); }

    /* Dashboard Styles */
    table { width: 100%; margin-bottom: 20px; border-collapse: collapse; background: white; }
    th, td { padding: 12px; border: 1px solid #ddd; text-align: center; }
    th { background-color: var(--success); color: white; }
    .status-card { display: flex; justify-content: space-around; align-items: center; }
    .stat { font-size: 1.2em; font-weight: bold; }
    .on-btn { background-color: #e7f3e7; color: #2e7d32; border: 1px solid #2e7d32; padding: 5px 10px; border-radius: 4px; cursor: pointer; }
    .off-btn { background-color: #ffebee; color: #c62828; border: 1px solid #c62828; padding: 5px 10px; border-radius: 4px; cursor: pointer; }
    .save-btn { background-color: var(--primary); color: white; border: none; padding: 8px 12px; border-radius: 4px; cursor: pointer; }
    
    .warning-box { background-color: #ffeb3b; color: #000; padding: 15px; border: 3px solid var(--danger); border-radius: 10px; margin-bottom: 20px; font-weight: bold; display: none; align-items: center; justify-content: space-between; }
    .revert-btn { background-color: var(--danger); color: white; border: none; padding: 8px 15px; border-radius: 5px; cursor: pointer; }
    #status-dot { height: 12px; width: 12px; background-color: #bbb; border-radius: 50%; display: inline-block; margin-right: 5px; }
    .updated-text { font-size: 0.8em; color: #666; }

    /* Modal & Overlay */
    .modal-overlay { position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.7); display: flex; align-items: center; justify-content: center; z-index: 1000; }
    .modal-content { background: white; padding: 25px; border-radius: 12px; width: 95%; max-width: 450px; max-height: 90vh; overflow-y: auto; text-align: left; }
    
    /* Day Selector */
    .day-selector { display: flex; justify-content: space-between; margin: 15px 0; }
    .day-btn { width: 35px; height: 35px; border-radius: 50%; border: 1px solid #ddd; background: #fff; cursor: pointer; font-size: 12px; transition: 0.2s; font-weight: bold; }
    .day-btn.active { background: var(--primary); color: white; border-color: var(--primary); }
    .day-error { border: 2px solid var(--danger) !important; background: #fff8f8; }

    /* Matrix Selection */
    .matrix-title { font-size: 0.85em; font-weight: bold; margin-top: 15px; color: var(--text-gray); text-transform: uppercase; }
    .matrix { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; margin: 8px 0; }
    .matrix-box { border: 1px solid #ddd; padding: 10px 5px; text-align: center; border-radius: 6px; cursor: pointer; font-size: 0.85em; background: #fff; transition: 0.2s; }
    .matrix-box.active { background: #e3f2fd; border: 2px solid var(--primary); font-weight: bold; }
    .matrix-error { border: 2px solid var(--danger) !important; }

    /* Dual Range Slider */
    .range-wrapper { position: relative; height: 10px; margin: 25px 0; background: var(--gray); border-radius: 5px; }
    #slider-highlight { position: absolute; height: 100%; background: var(--primary); border-radius: 5px; z-index: 5; }
    .range-wrapper input { position: absolute; width: 100%; pointer-events: none; appearance: none; height: 0; background: none; top: 5px; z-index: 10; }
    input[type="range"]::-webkit-slider-thumb { pointer-events: all; appearance: none; width: 22px; height: 22px; border-radius: 50%; background: var(--primary); cursor: pointer; border: 2px solid white; box-shadow: 0 1px 3px rgba(0,0,0,0.3); }

    .invert-btn { background: var(--primary); color: white; border: none; padding: 5px 12px; border-radius: 4px; cursor: pointer; font-size: 13px; font-weight: bold; }

    /* Routine Cards */
    .routine-item { display: flex; justify-content: space-between; align-items: center; padding: 15px; border-bottom: 1px solid #eee; background: #fff; margin-bottom: 10px; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
    .meta-tag { background: #e3f2fd; color: var(--primary); font-size: 10px; padding: 2px 6px; border-radius: 4px; font-weight: bold; border: 1px solid #bbdefb; margin-right: 4px; }
    .day-label { color: var(--primary); font-size: 11px; font-weight: bold; margin-top: 3px; }

    /* Action Buttons */
    .action-btn { width: 38px; height: 38px; border: none; border-radius: 6px; cursor: pointer; color: white; display: inline-flex; align-items: center; justify-content: center; margin-left: 6px; }
    .btn-edit { background: #757575; font-size: 18px; }
    .btn-del { background: var(--danger); }
    .btn-del svg { width: 20px; height: 20px; stroke: white; stroke-width: 3.5; fill: none; }

    /* Form Inputs */
    input[type="text"], input[type="number"], input[type="time"] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 6px; box-sizing: border-box; }
    .input-error { border: 2px solid var(--danger) !important; background-color: #fff8f8; }
    .error-msg { color: var(--danger); font-size: 11px; font-weight: bold; display: none; margin-top: 4px; }

    .btn-group { display: flex; gap: 10px; margin: 15px 0; }
    .btn-tab { flex: 1; padding: 10px; border: 1px solid #ccc; background: #eee; cursor: pointer; border-radius: 6px; font-weight: bold; }
    .btn-tab.active { background: var(--primary); color: white; border-color: var(--primary); }
    svg.nav-icon { width: 18px; height: 18px; fill: currentColor; }
  </style>
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
        <button id="nav-conf" class="nav-btn" onclick="showView('debug-view')">
            <svg class="nav-icon" viewBox="0 0 24 24"><path d="M22.7 19l-9.1-9.1c.9-2.3.4-5-1.5-6.9-2-2-5-2.4-7.4-1.3L9 6 6 9 1.6 4.7C.5 7.1.9 10.1 2.9 12.1c1.9 1.9 4.6 2.4 6.9 1.5l9.1 9.1c.4.4 1 .4 1.4 0l2.3-2.3c.5-.4.5-1.1.1-1.4z"/></svg> Config
        </button>
    </div>

    <div id="main-view">
      <h2>System Dashboard</h2>
      <div id="override-warn" class="warning-box">
        <span>&#9888; OVERRIDE ACTIVE: SAFETY OFF</span>
        <button class="revert-btn" onclick="disableOverride()">REVERT TO SAFETY</button>
      </div>

      <div class="card status-card">
        <div><span id="status-dot"></span><span id="conn-status">Searching...</span></div>
        <div>Tank: <span id="waterLvl" class="stat">--</span>%</div>
        <div class="updated-text">Synced: <span id="last-upd">Never</span></div>
      </div>

      <div class="card">
        <table>
          <tr><th>Pump</th><th>Moisture</th><th>Timed Run</th><th>Manual</th></tr>
          <script>
            for(let i=1; i<=4; i++) {
              document.write(`<tr>
                <td>P${i}</td>
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
      </div>
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
          <script>
            for(let i=1; i<=4; i++) {
              document.write(`<tr>
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

  <script>
    /* UI Navigation */
    function showView(viewId) {
      document.getElementById('main-view').classList.add('hidden');
      document.getElementById('routine-view').classList.add('hidden');
      document.getElementById('debug-view').classList.add('hidden');
      document.getElementById(viewId).classList.remove('hidden');
      
      document.getElementById('nav-dash').classList.remove('active');
      document.getElementById('nav-rout').classList.remove('active');
      document.getElementById('nav-conf').classList.remove('active');
      
      if(viewId === 'main-view') document.getElementById('nav-dash').classList.add('active');
      if(viewId === 'routine-view') document.getElementById('nav-rout').classList.add('active');
      if(viewId === 'debug-view') document.getElementById('nav-conf').classList.add('active');
    }

    /* Core Control Functions */
    function toggleOverride() {
      var checkBox = document.getElementById("overrideCheck");
      if (checkBox.checked) {
        if (confirm("WARNING: Disabling safety limits can flood plants. Continue?")) setOverride(1);
        else checkBox.checked = false;
      } else setOverride(0);
    }

    function setOverride(val) {
      fetch('/calibrate?type=override&val=' + val);
      const isAct = (val === 1);
      document.getElementById("override-warn").style.display = isAct ? "flex" : "none";
      document.getElementById("overrideCheck").checked = isAct;
    }

    function disableOverride() { setOverride(0); }
    function startPump(p) { fetch("/pump?p=" + p + "&t=" + document.getElementById("t" + p).value); }
    function controlPump(a, p) { fetch("/pump?" + a + "=" + p); }

    function saveCal(id, type, label) {
      let rawVal = (type.startsWith('water')) ? document.getElementById('wRaw').innerText : document.getElementById('raw' + id).innerText;
      if(rawVal === "--") return;
      if (confirm("Save " + label + " as " + rawVal + "?")) fetch(`/calibrate?sensor=${id}&type=${type}&val=${rawVal}`);
    }

    /* ROUTINE LOGIC */
    let routines = [];
    let mode = 'static';
    let selP = 0, selS = 0, isInverted = false, editId = null;
    const dayNames = ['M', 'T', 'W', 'T', 'F', 'S', 'S'];

    function openModal(id = null) {
        resetForm();
        if(id) {
            editId = id;
            const r = routines.find(x => x.id === id);
            document.getElementById('modal-title').innerText = "Edit Routine";
            document.getElementById('r-name').value = r.name;
            setMode(r.mode);
            selectMatrix('p', r.p);
            if(r.mode === 'smart') selectMatrix('s', r.s);
            const btns = document.querySelectorAll('.day-btn');
            r.days.forEach((on, i) => { if(on) btns[i].classList.add('active'); });
            if(r.mode === 'static') {
                document.getElementById('r-time').value = r.time;
                document.getElementById('r-ml').value = r.val;
            } else {
                isInverted = r.inv;
                document.getElementById('s-min').value = r.min;
                document.getElementById('s-max').value = r.max;
                document.getElementById('r-moist').value = r.val;
                document.getElementById('moist-val').innerText = r.val;
                updateSliders();
            }
        }
        document.getElementById('modal-container').classList.remove('hidden');
    }

    function closeModal() { document.getElementById('modal-container').classList.add('hidden'); }

    function setMode(m) {
        mode = m;
        hideErrs();
        document.getElementById('tab-static').classList.toggle('active', m==='static');
        document.getElementById('tab-smart').classList.toggle('active', m==='smart');
        document.getElementById('content-static').classList.toggle('hidden', m==='smart');
        document.getElementById('content-smart').classList.toggle('hidden', m==='static');
        document.getElementById('sensor-section').classList.toggle('hidden', m==='static');
        if(m==='static') selS = 0;
    }

    function toggleDay(btn) { btn.classList.toggle('active'); hideErrs(); }

    function selectMatrix(type, val) {
        const id = type === 'p' ? 'p-matrix' : 's-matrix';
        document.querySelectorAll('#' + id + ' .matrix-box').forEach((b, i) => {
            b.classList.remove('matrix-error');
            b.classList.toggle('active', i === (val-1));
        });
        if(type==='p') selP = val; else selS = val;
    }

    function updateSliders() {
        let min = parseInt(document.getElementById('s-min').value);
        let max = parseInt(document.getElementById('s-max').value);
        if (min > max) [min, max] = [max, min];
        const high = document.getElementById('slider-highlight');
        const wrap = document.getElementById('range-wrap');
        const label = document.getElementById('window-label');

        high.style.left = (min/48)*100 + '%';
        high.style.width = ((max-min)/48)*100 + '%';
        high.style.background = isInverted ? 'var(--gray)' : 'var(--primary)';
        wrap.style.background = isInverted ? 'var(--primary)' : 'var(--gray)';
        label.innerText = isInverted ? "Inactive Window" : "Active Window";

        const getTime = (v) => {
            let h = Math.floor(v/2).toString().padStart(2,'0');
            return h + (v%2==0 ? ':00' : ':30');
        };
        document.getElementById('range-text').innerText = `${getTime(min)}-${getTime(max)}`;
    }

    function invert() { isInverted = !isInverted; updateSliders(); }

    function hideErrs() {
        document.querySelectorAll('.error-msg').forEach(e => e.style.display = 'none');
        document.querySelectorAll('input, .day-btn, .matrix-box').forEach(i => i.classList.remove('input-error', 'day-error', 'matrix-error'));
    }

    function saveRoutineBtn() {
        let valid = true;
        hideErrs();
        const name = document.getElementById('r-name');
        const activeDays = Array.from(document.querySelectorAll('.day-btn')).map(b => b.classList.contains('active'));

        if(!name.value.trim()) { name.classList.add('input-error'); document.getElementById('err-name').style.display='block'; valid = false; }
        else if (routines.some(r => r.name.toLowerCase() === name.value.trim().toLowerCase() && r.id !== editId)) {
            name.classList.add('input-error'); document.getElementById('err-name-exists').style.display='block'; valid = false;
        }

        if(!activeDays.includes(true)) {
            document.querySelectorAll('.day-btn').forEach(b => b.classList.add('day-error'));
            document.getElementById('err-days').style.display = 'block';
            valid = false;
        }

        if(mode === 'static') {
            const time = document.getElementById('r-time');
            const ml = document.getElementById('r-ml');
            if(!time.value) { time.classList.add('input-error'); document.getElementById('err-time').style.display='block'; valid = false; }
            if(parseFloat(ml.value) <= 0) { ml.classList.add('input-error'); document.getElementById('err-ml').style.display='block'; valid = false; }
        } else {
            if(parseInt(document.getElementById('r-moist').value) <= 0) { document.getElementById('err-moist').style.display='block'; valid = false; }
            if(selS === 0) { document.querySelectorAll('#s-matrix .matrix-box').forEach(b => b.classList.add('matrix-error')); valid = false; }
        }

        if(selP === 0) { document.querySelectorAll('#p-matrix .matrix-box').forEach(b => b.classList.add('matrix-error')); valid = false; }

        if(!valid) return;

        const data = {
            id: editId || Date.now(),
            name: name.value.trim(),
            mode: mode,
            days: activeDays,
            p: selP, s: selS,
            val: mode === 'static' ? document.getElementById('r-ml').value : document.getElementById('r-moist').value,
            time: document.getElementById('r-time').value,
            min: document.getElementById('s-min').value,
            max: document.getElementById('s-max').value,
            inv: isInverted,
            rangeStr: mode === 'smart' ? document.getElementById('range-text').innerText : ''
        };

        // Note: In a real ESP32 project, you would add a fetch() here to send 'data' to your server
        if(editId) {
            const idx = routines.findIndex(r => r.id === editId);
            routines[idx] = data;
        } else routines.push(data);

        renderRoutines();
        closeModal();
    }

    function renderRoutines() {
        const list = document.getElementById('routine-list');
        list.innerHTML = routines.length === 0 ? '<p id="empty-msg" style="text-align:center; color:#888;">No routines created yet.</p>' : "";
        routines.forEach(r => {
            const item = document.createElement('div');
            item.className = 'routine-item';
            
            let timeInfo = r.mode === 'static' ? r.time : `${r.rangeStr} (${r.inv ? 'Inactive' : 'Active'})`;
            let triggerInfo = r.mode === 'static' ? `${r.val}ml` : `${r.val}% Threshold`;
            let sched = r.days.map((on, i) => on ? dayNames[i] : '').filter(n => n!=='').join(' ');
            
            item.innerHTML = `
                <div style="flex:1;">
                    <div style="font-weight:bold;">${r.name} <small style="color:#888;">(${r.mode})</small></div>
                    <div style="font-size:12px; color:#444; margin:2px 0;"><b>Time:</b> ${timeInfo}</div>
                    <div style="font-size:12px; color:#666; margin-bottom:4px;"><b>Trigger:</b> ${triggerInfo}</div>
                    <div class="day-label">${sched}</div>
                    <div style="margin-top:6px;">
                        <span class="meta-tag">PUMP ${r.p}</span>
                        ${r.mode === 'smart' ? `<span class="meta-tag">SENSOR ${r.s}</span>` : ''}
                    </div>
                </div>
                <div style="display:flex;">
                    <button class="action-btn btn-edit" onclick="openModal(${r.id})">✎</button>
                    <button class="action-btn btn-del" onclick="delRoutine(${r.id})">
                        <svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="3"><path d="M3 6h18M19 6v14a2 2 0 01-2 2H7a2 2 0 01-2-2V6m3 0V4a2 2 0 012-2h4a2 2 0 012 2v2M10 11v6M14 11v6"/></svg>
                    </button>
                </div>`;
            list.appendChild(item);
        });
    }

    function delRoutine(id) { routines = routines.filter(x => x.id !== id); renderRoutines(); }

    function resetForm() {
        editId = null;
        document.getElementById('modal-title').innerText = "Create Routine";
        document.getElementById('r-name').value = "";
        document.getElementById('r-time').value = "";
        document.querySelectorAll('.day-btn').forEach(b => b.classList.remove('active'));
        document.querySelectorAll('.matrix-box').forEach(b => b.classList.remove('active', 'matrix-error'));
        hideErrs();
        selP = 0; selS = 0; isInverted = false;
        setMode('static');
        updateSliders();
    }

    /* STATUS UPDATER */
    function updateStatus() {
      fetch('/status').then(r => r.json()).then(data => {
        document.getElementById("waterLvl").innerText = data.water;
        for(let i=1; i<=4; i++) {
          if(document.getElementById("m" + i)) document.getElementById("m" + i).innerText = data.m[i-1];
          if(document.getElementById("raw" + i)) document.getElementById("raw" + i).innerText = data.raw[i-1];
        }
        if(!document.getElementById('debug-view').classList.contains('hidden')) document.getElementById("wRaw").innerText = data.waterRaw;
        document.getElementById("status-dot").style.backgroundColor = "#4CAF50";
        document.getElementById("conn-status").innerText = "Connected";
        document.getElementById("last-upd").innerText = new Date().toLocaleTimeString();
      }).catch(() => {
        document.getElementById("status-dot").style.backgroundColor = "#f44336";
        document.getElementById("conn-status").innerText = "Offline";
      });
    }

    setInterval(updateStatus, 2000); 
    updateStatus();
    updateSliders();
  </script>
</body>
</html>
)rawliteral";

#endif