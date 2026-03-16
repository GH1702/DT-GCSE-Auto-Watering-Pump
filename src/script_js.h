#ifndef SCRIPT_JS_H
#define SCRIPT_JS_H

const char SCRIPT_JS[] PROGMEM = R"rawliteral(
/* UI Navigation */
function showView(viewId) {
  document.getElementById('main-view').classList.add('hidden');
  document.getElementById('routine-view').classList.add('hidden');
  document.getElementById('notif-view').classList.add('hidden');
  document.getElementById('debug-view').classList.add('hidden');
  document.getElementById('led-view').classList.add('hidden');
  
  document.getElementById('nav-dash').classList.remove('active');
  document.getElementById('nav-rout').classList.remove('active');
  document.getElementById('nav-notif').classList.remove('active');
  document.getElementById('nav-conf').classList.remove('active');
  document.getElementById('nav-led').classList.remove('active');
  
  document.getElementById(viewId).classList.remove('hidden');
  
  if(viewId === 'main-view') document.getElementById('nav-dash').classList.add('active');
  if(viewId === 'routine-view') document.getElementById('nav-rout').classList.add('active');
  if(viewId === 'notif-view') document.getElementById('nav-notif').classList.add('active');
  if(viewId === 'debug-view') document.getElementById('nav-conf').classList.add('active');
  if(viewId === 'led-view') document.getElementById('nav-led').classList.add('active');
}

function toggleOverride() {
  var checkBox = document.getElementById("overrideCheck");
  if (checkBox.checked) {
    if (confirm("WARNING: Disabling safety limits can flood plants. Continue?")) {
      setOverride(1);
    } else {
      checkBox.checked = false;
    }
  } else {
    setOverride(0);
  }
}

function setOverride(val) {
  fetch('/calibrate?type=override&val=' + val);
  const isAct = (val === 1);
  document.getElementById("override-warn").style.display = isAct ? "flex" : "none";
  document.getElementById("overrideCheck").checked = isAct;
}

function disableOverride() { 
  setOverride(0); 
}

function startPump(p) {
  let t = document.getElementById("t" + p).value;
  if (!t || t <= 0) t = 10;
  const warningEl = document.getElementById('run-warning');
  warningEl.style.display = 'none';

  let url = '/pump?api=1&p=' + p;
  if (runInputMode === 'ml') {
    const ml = parseFloat(t);
    const rate = pumpMlRates[p - 1] > 0 ? pumpMlRates[p - 1] : 21.5;
    const estSec = Math.ceil(ml / rate);
    if (estSec > 10 && !overrideEnabled) {
      warningEl.innerText = 'Run needs ' + estSec + 's (>10s). Enable Override to execute.';
      warningEl.style.display = 'block';
      return;
    }
    url += '&ml=' + ml;
  } else {
    url += '&t=' + t;
  }

  fetch(url)
    .then(r => r.text())
    .then(msg => {
      if (msg === 'OVERRIDE_REQUIRED') {
        warningEl.innerText = 'Override required: this run exceeds 10s watchdog.';
        warningEl.style.display = 'block';
      }
    });
}

function controlPump(a, p) { 
  fetch('/pump?' + a + '=' + p);
}

function saveCal(id, type, label) {
  let rawVal = (type.startsWith('water')) ? 
    document.getElementById('wRaw').innerText : 
    document.getElementById('raw' + id).innerText;
  if(rawVal === "--") return;
  if (confirm("Save " + label + " as " + rawVal + "?")) {
    fetch('/calibrate?sensor=' + id + '&type=' + type + '&val=' + rawVal);
  }
}

let routines = [];
let mode = 'static';
let selP = 0, selS = 0, isInverted = false, editId = null;
const dayNames = ['M', 'T', 'W', 'T', 'F', 'S', 'S'];
let runInputMode = 'sec';
let pumpMlRates = [21.5, 21.5, 21.5, 21.5];
let overrideEnabled = false;
let apNoticeShown = false;

function loadRoutinesFromESP() {
  fetch('/routines')
    .then(r => r.json())
    .then(data => {
      routines = data || [];
      renderRoutines();
    })
    .catch(e => {
      console.error('Load routines failed:', e);
      routines = [];
    });
}

function saveRoutinesToESP() {
  return fetch('/routines', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(routines)
  })
  .then(r => r.text())
  .then(() => {
    console.log('Routines saved');
    showSaveToast('Settings Stored');
  })
  .catch(e => console.error('Save failed:', e));
}

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

function closeModal() { 
  document.getElementById('modal-container').classList.add('hidden'); 
}

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

function toggleDay(btn) { 
  btn.classList.toggle('active'); 
  hideErrs(); 
}

function selectMatrix(type, val) {
  const id = type === 'p' ? 'p-matrix' : 's-matrix';
  document.querySelectorAll('#' + id + ' .matrix-box').forEach((b, i) => {
    b.classList.remove('matrix-error');
    b.classList.toggle('active', i === (val-1));
  });
  if(type==='p') selP = val; 
  else selS = val;
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
  document.getElementById('range-text').innerText = getTime(min) + '-' + getTime(max);
}

function invert() { 
  isInverted = !isInverted; 
  updateSliders(); 
}

function hideErrs() {
  document.querySelectorAll('.error-msg').forEach(e => e.style.display = 'none');
  document.querySelectorAll('input, .day-btn, .matrix-box').forEach(i => 
    i.classList.remove('input-error', 'day-error', 'matrix-error')
  );
}

function saveRoutineBtn() {
  let valid = true;
  hideErrs();
  const name = document.getElementById('r-name');
  const activeDays = Array.from(document.querySelectorAll('.day-btn')).map(b => 
    b.classList.contains('active')
  );
  
  if(!name.value.trim()) { 
    name.classList.add('input-error'); 
    document.getElementById('err-name').style.display='block'; 
    valid = false; 
  }
  if(!activeDays.includes(true)) { 
    document.getElementById('err-days').style.display='block';
    valid = false; 
  }
  if(selP === 0) { 
    document.querySelectorAll('#p-matrix .matrix-box').forEach(b => 
      b.classList.add('matrix-error')
    );
    valid = false; 
  }
  
  if(!valid) return;
  
  const data = {
    id: editId || Date.now(),
    enabled: editId ? ((routines.find(r => r.id === editId) || {}).enabled !== false) : true,
    name: name.value.trim(),
    mode: mode,
    days: activeDays,
    p: selP, 
    s: selS,
    val: mode === 'static' ? 
      document.getElementById('r-ml').value : 
      document.getElementById('r-moist').value,
    time: document.getElementById('r-time').value,
    min: document.getElementById('s-min').value,
    max: document.getElementById('s-max').value,
    inv: isInverted,
    rangeStr: mode === 'smart' ? 
      document.getElementById('range-text').innerText : ''
  };
  
  if(editId) {
    const idx = routines.findIndex(r => r.id === editId);
    routines[idx] = data;
  } else {
    routines.push(data);
  }
  
  saveRoutinesToESP();
  renderRoutines();
  closeModal();
}

function renderRoutines() {
  const list = document.getElementById('routine-list');
  if(routines.length === 0) {
    list.innerHTML = '<p id="empty-msg" style="text-align:center; color:#888;">No routines created yet.</p>';
    return;
  }
  
  list.innerHTML = "";
  routines.forEach(r => {
    const item = document.createElement('div');
    item.className = 'routine-item';
    
    const dayStr = r.days.map((on, i) => on ? dayNames[i] : '').filter(d => d).join(', ');
    const modeTag = '<span class="meta-tag">' + r.mode.toUpperCase() + '</span>';
    const pumpTag = '<span class="meta-tag">P' + r.p + '</span>';
    const sensorTag = r.mode === 'smart' ? '<span class="meta-tag">S' + r.s + '</span>' : '';
    
    let details = '';
    if(r.mode === 'static') {
      details = r.time + ' • ' + r.val + 'ml';
    } else {
      details = r.rangeStr + ' • ' + r.val + '% trigger';
    }
    
    const enabledChecked = (r.enabled === false) ? '' : 'checked';
    item.innerHTML = '<div><div><input type="checkbox" ' + enabledChecked + ' onchange="toggleRoutineEnabled(' + r.id + ', this.checked)" style="margin-right:8px;"><b>' + r.name + '</b> ' + modeTag + pumpTag + sensorTag + 
      '</div><div class="day-label">' + dayStr + '</div><div style="font-size:0.85em; color:#666; margin-top:2px;">' + 
      details + '</div></div><div style="display:flex; align-items:center;"><button class="action-btn btn-run" onclick="runRoutine(' + 
      r.id + ')" title="Run now"><svg viewBox="0 0 24 24"><path d="M8 5v14l11-7z"/></svg></button><button class="action-btn btn-edit" onclick="openModal(' + 
      r.id + ')" title="Edit">✏</button><button class="action-btn btn-del" onclick="deleteRoutine(' + r.id + 
      ')" title="Delete"><svg viewBox="0 0 24 24"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg></button></div>';
    list.appendChild(item);
  });
}

function runRoutine(id) {
  fetch('/routine/run?id=' + id)
    .then(r => r.text())
    .then(msg => alert('Routine executed!'))
    .catch(e => console.error('Run failed:', e));
}

function toggleRoutineEnabled(id, enabled) {
  const idx = routines.findIndex(r => r.id === id);
  if (idx >= 0) {
    routines[idx].enabled = enabled;
    saveRoutinesToESP();
  }
}

function deleteRoutine(id) {
  if(confirm('Delete this routine?')) {
    routines = routines.filter(r => r.id !== id);
    saveRoutinesToESP();
    renderRoutines();
  }
}

function resetForm() {
  editId = null;
  document.getElementById('r-name').value = "";
  document.getElementById('r-time').value = "";
  document.getElementById('r-ml').value = "100";
  document.getElementById('r-moist').value = "30";
  document.getElementById('moist-val').innerText = "30";
  document.getElementById('s-min').value = "16";
  document.getElementById('s-max').value = "36";
  document.querySelectorAll('.day-btn').forEach(b => b.classList.remove('active'));
  document.querySelectorAll('.matrix-box').forEach(b => b.classList.remove('active'));
  selP = 0; 
  selS = 0;
  isInverted = false;
  setMode('static');
  document.getElementById('modal-title').innerText = "Create Routine";
  updateSliders();
}

let notifications = [];
let notifEditId = null;

function loadAutomationsFromESP() {
  fetch('/automations')
    .then(r => r.json())
    .then(data => {
      notifications = data || [];
      renderNotifications();
    })
    .catch(e => {
      console.error('Load automations failed:', e);
      notifications = [];
    });
}

function saveAutomationsToESP() {
  return fetch('/automations', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(notifications)
  })
  .then(r => r.text())
  .then(() => {
    console.log('Automations saved');
    showSaveToast('Settings Stored');
  })
  .catch(e => console.error('Save failed:', e));
}

function openNotifModal(id = null) {
  notifEditId = null;
  document.getElementById('notif-name').value = "";
  document.getElementById('notif-when').value = "";
  document.getElementById('notif-if').value = "";
  document.getElementById('notif-do').value = "";
  document.getElementById('notif-message').value = "";
  document.getElementById('notif-if-lid-mins').value = "10";
  document.getElementById('notif-led-mode').value = "normal";
  document.getElementById('notif-repeat-hours').value = "0";
  
  document.getElementById('when-options').classList.add('hidden');
  document.getElementById('when-sensor-group').classList.add('hidden');
  document.getElementById('when-water-group').classList.add('hidden');
  document.getElementById('when-pump-group').classList.add('hidden');
  document.getElementById('when-time-group').classList.add('hidden');
  
  document.getElementById('if-options').classList.add('hidden');
  document.getElementById('if-sensor-group').classList.add('hidden');
  document.getElementById('if-water-group').classList.add('hidden');
  document.getElementById('if-time-group').classList.add('hidden');
  document.getElementById('if-lid-group').classList.add('hidden');
  
  document.getElementById('do-options').classList.add('hidden');
  document.getElementById('do-whatsapp-group').classList.add('hidden');
  document.getElementById('do-pump-group').classList.add('hidden');
  document.getElementById('do-pump-duration').classList.add('hidden');
  document.getElementById('do-led-group').classList.add('hidden');
  
  document.getElementById('notif-modal-title').innerText = "Create Automation";

  if (id) {
    const a = notifications.find(x => x.id === id);
    if (a) {
      notifEditId = id;
      document.getElementById('notif-modal-title').innerText = "Edit Automation";
      document.getElementById('notif-name').value = a.name || "";
      document.getElementById('notif-when').value = (a.when && a.when.type) ? a.when.type : "";
      updateWhenOptions();
      if (a.when) {
        if (a.when.sensor) document.getElementById('notif-sensor').value = a.when.sensor;
        if (a.when.threshold) document.getElementById('notif-threshold').value = a.when.threshold;
        if (a.when.threshold) document.getElementById('notif-water-threshold').value = a.when.threshold;
        if (a.when.pump) document.getElementById('notif-pump').value = a.when.pump;
        if (a.when.time) document.getElementById('notif-time').value = a.when.time;
      }

      document.getElementById('notif-if').value = (a.if && a.if.type) ? a.if.type : "";
      updateIfOptions();
      if (a.if) {
        if (a.if.sensor) document.getElementById('notif-if-sensor').value = a.if.sensor;
        if (a.if.threshold) document.getElementById('notif-if-threshold').value = a.if.threshold;
        if (a.if.threshold) document.getElementById('notif-if-water-threshold').value = a.if.threshold;
        if (a.if.timeFrom) document.getElementById('notif-if-time-from').value = a.if.timeFrom;
        if (a.if.timeTo) document.getElementById('notif-if-time-to').value = a.if.timeTo;
        if (a.if.minutes) document.getElementById('notif-if-lid-mins').value = a.if.minutes;
      }

      document.getElementById('notif-do').value = (a.do && a.do.type) ? a.do.type : "";
      updateDoOptions();
      if (a.do) {
        if (a.do.message) document.getElementById('notif-message').value = a.do.message;
        if (a.do.pump) document.getElementById('notif-do-pump').value = a.do.pump;
        if (a.do.duration) document.getElementById('notif-pump-duration').value = a.do.duration;
        if (a.do.ledMode) document.getElementById('notif-led-mode').value = a.do.ledMode;
        if (a.do.repeatHours != null) document.getElementById('notif-repeat-hours').value = a.do.repeatHours;
      }
    }
  }

  hideNotifErrs();
  document.getElementById('notif-modal-container').classList.remove('hidden');
}

function closeNotifModal() {
  document.getElementById('notif-modal-container').classList.add('hidden');
}

function updateWhenOptions() {
  const when = document.getElementById('notif-when').value;
  const whenOptions = document.getElementById('when-options');
  const sensorGroup = document.getElementById('when-sensor-group');
  const waterGroup = document.getElementById('when-water-group');
  const pumpGroup = document.getElementById('when-pump-group');
  const timeGroup = document.getElementById('when-time-group');
  
  sensorGroup.classList.add('hidden');
  waterGroup.classList.add('hidden');
  pumpGroup.classList.add('hidden');
  timeGroup.classList.add('hidden');
  
  if(when) {
    whenOptions.classList.remove('hidden');
    
    if(when.includes('moisture')) {
      sensorGroup.classList.remove('hidden');
    } else if (when.includes('water')) {
      waterGroup.classList.remove('hidden');
    } else if(when.includes('pump')) {
      pumpGroup.classList.remove('hidden');
    } else if(when === 'time') {
      timeGroup.classList.remove('hidden');
    }
  } else {
    whenOptions.classList.add('hidden');
  }
  
  hideNotifErrs();
}

function updateIfOptions() {
  const ifCond = document.getElementById('notif-if').value;
  const ifOptions = document.getElementById('if-options');
  const sensorGroup = document.getElementById('if-sensor-group');
  const waterGroup = document.getElementById('if-water-group');
  const timeGroup = document.getElementById('if-time-group');
  const lidGroup = document.getElementById('if-lid-group');
  
  sensorGroup.classList.add('hidden');
  waterGroup.classList.add('hidden');
  timeGroup.classList.add('hidden');
  lidGroup.classList.add('hidden');
  
  if(ifCond) {
    ifOptions.classList.remove('hidden');
    
    if(ifCond.includes('moisture')) {
      sensorGroup.classList.remove('hidden');
    } else if(ifCond.includes('water')) {
      waterGroup.classList.remove('hidden');
    } else if(ifCond === 'time_between') {
      timeGroup.classList.remove('hidden');
    } else if(ifCond === 'lid_off_for') {
      lidGroup.classList.remove('hidden');
    }
  } else {
    ifOptions.classList.add('hidden');
  }
}

function updateDoOptions() {
  const doAction = document.getElementById('notif-do').value;
  const doOptions = document.getElementById('do-options');
  const whatsappGroup = document.getElementById('do-whatsapp-group');
  const pumpGroup = document.getElementById('do-pump-group');
  const pumpDuration = document.getElementById('do-pump-duration');
  const ledGroup = document.getElementById('do-led-group');
  
  whatsappGroup.classList.add('hidden');
  pumpGroup.classList.add('hidden');
  pumpDuration.classList.add('hidden');
  ledGroup.classList.add('hidden');
  
  if(doAction) {
    doOptions.classList.remove('hidden');
    
    if(doAction === 'whatsapp') {
      whatsappGroup.classList.remove('hidden');
    } else if(doAction === 'pump_on' || doAction === 'pump_off') {
      pumpGroup.classList.remove('hidden');
      if(doAction === 'pump_on') {
        pumpDuration.classList.remove('hidden');
      }
    } else if (doAction === 'led_mode') {
      ledGroup.classList.remove('hidden');
    }
  } else {
    doOptions.classList.add('hidden');
  }
  
  hideNotifErrs();
}

function hideNotifErrs() {
  document.querySelectorAll('#notif-modal-container .error-msg').forEach(e => e.style.display = 'none');
  document.querySelectorAll('#notif-modal-container input, #notif-modal-container select, #notif-modal-container textarea').forEach(i => 
    i.classList.remove('input-error')
  );
}

function saveNotification() {
  const name = document.getElementById('notif-name').value.trim();
  const when = document.getElementById('notif-when').value;
  const ifCond = document.getElementById('notif-if').value;
  const doAction = document.getElementById('notif-do').value;
  
  let valid = true;
  hideNotifErrs();
  
  if(!name) {
    document.getElementById('notif-name').classList.add('input-error');
    document.getElementById('err-notif-name').style.display = 'block';
    valid = false;
  }
  
  if(!when) {
    document.getElementById('notif-when').classList.add('input-error');
    document.getElementById('err-notif-when').style.display = 'block';
    valid = false;
  }
  
  if(!doAction) {
    document.getElementById('notif-do').classList.add('input-error');
    document.getElementById('err-notif-do').style.display = 'block';
    valid = false;
  }
  
  if(!valid) return;
  
  const automation = {
    id: notifEditId || Date.now(),
    enabled: notifEditId ? ((notifications.find(n => n.id === notifEditId) || {}).enabled !== false) : true,
    name: name,
    when: {
      type: when,
      sensor: when.includes('moisture') ? document.getElementById('notif-sensor').value : null,
      threshold: when.includes('water') ? document.getElementById('notif-water-threshold').value : document.getElementById('notif-threshold').value,
      pump: document.getElementById('notif-pump') ? document.getElementById('notif-pump').value : null,
      time: document.getElementById('notif-time') ? document.getElementById('notif-time').value : null
    },
    if: ifCond ? {
      type: ifCond,
      sensor: ifCond.includes('moisture') ? document.getElementById('notif-if-sensor').value : null,
      threshold: ifCond.includes('water') ? document.getElementById('notif-if-water-threshold').value : document.getElementById('notif-if-threshold').value,
      timeFrom: document.getElementById('notif-if-time-from') ? document.getElementById('notif-if-time-from').value : null,
      timeTo: document.getElementById('notif-if-time-to') ? document.getElementById('notif-if-time-to').value : null,
      minutes: document.getElementById('notif-if-lid-mins') ? document.getElementById('notif-if-lid-mins').value : null
    } : null,
    do: {
      type: doAction,
      message: document.getElementById('notif-message') ? document.getElementById('notif-message').value : null,
      repeatHours: document.getElementById('notif-repeat-hours') ? document.getElementById('notif-repeat-hours').value : 0,
      pump: document.getElementById('notif-do-pump') ? document.getElementById('notif-do-pump').value : null,
      duration: document.getElementById('notif-pump-duration') ? document.getElementById('notif-pump-duration').value : null,
      ledMode: document.getElementById('notif-led-mode') ? document.getElementById('notif-led-mode').value : null
    },
    created: notifEditId ? ((notifications.find(n => n.id === notifEditId) || {}).created || new Date().toLocaleString()) : new Date().toLocaleString()
  };
  
  if (notifEditId) {
    const idx = notifications.findIndex(n => n.id === notifEditId);
    if (idx >= 0) notifications[idx] = automation;
  } else {
    notifications.unshift(automation);
  }
  saveAutomationsToESP();
  renderNotifications();
  closeNotifModal();
}

function getReadableCondition(type, data) {
  const conditions = {
    'moisture_below': 'Moisture sensor ' + data.sensor + ' falls below ' + data.threshold + '%',
    'moisture_above': 'Moisture sensor ' + data.sensor + ' rises above ' + data.threshold + '%',
    'water_below': 'Water tank falls below ' + data.threshold + '%',
    'water_above': 'Water tank rises above ' + data.threshold + '%',
    'lid_off': 'LID is OFF',
    'pump_starts': 'Pump ' + data.pump + ' starts',
    'pump_stops': 'Pump ' + data.pump + ' stops',
    'time': 'At ' + data.time,
    'lid_off_for': 'LID has been OFF for ' + data.minutes + ' mins',
    'time_between': 'Time is between ' + data.timeFrom + ' and ' + data.timeTo
  };
  return conditions[type] || type;
}

function getReadableAction(type, data) {
  const actions = {
    'whatsapp': 'Send WhatsApp: "' + data.message + '"' + ((data.repeatHours && Number(data.repeatHours) > 0) ? (' every ' + data.repeatHours + 'h') : ''),
    'pump_on': 'Turn pump ' + data.pump + ' ON for ' + data.duration + 's',
    'pump_off': 'Turn pump ' + data.pump + ' OFF',
    'led_mode': 'Set LED mode to ' + data.ledMode
  };
  return actions[type] || type;
}

function renderNotifications() {
  const list = document.getElementById('notif-list');
  
  if(notifications.length === 0) {
    list.innerHTML = '<p id="empty-notif-msg" style="text-align:center; color:#888;">No automations yet.</p>';
    return;
  }
  
  list.innerHTML = "";
  notifications.forEach(n => {
    const item = document.createElement('div');
    item.className = 'notif-card';
    
    let ifCondition = '';
    if(n.if) {
      ifCondition = '<div class="automation-detail"><strong>If:</strong> ' + getReadableCondition(n.if.type, n.if) + '</div>';
    }
    
    const enabledChecked = (n.enabled === false) ? '' : 'checked';
    item.innerHTML = '<div class="notif-content"><h4><input type="checkbox" ' + enabledChecked + ' onchange="toggleAutomationEnabled(' + n.id + ', this.checked)" style="margin-right:8px;">' + n.name + '</h4><div class="automation-detail"><strong>When:</strong> ' + 
      getReadableCondition(n.when.type, n.when) + '</div>' + ifCondition + '<div class="automation-detail"><strong>Do:</strong> ' + 
      getReadableAction(n.do.type, n.do) + '</div><div class="notif-timestamp">Created: ' + n.created + 
      '</div></div><div style="display:flex; align-items:center;"><button class="action-btn btn-run" onclick="runAutomation(' + n.id + 
      ')" title="Run now"><svg viewBox="0 0 24 24"><path d="M8 5v14l11-7z"/></svg></button><button class="action-btn btn-edit" onclick="editAutomation(' + 
      n.id + ')" title="Edit">✏</button><button class="action-btn btn-del" onclick="deleteNotification(' + n.id + 
      ')" title="Delete"><svg viewBox="0 0 24 24"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg></button></div>';
    list.appendChild(item);
  });
}

function runAutomation(id) {
  fetch('/automation/run?id=' + id)
    .then(r => r.text())
    .then(msg => alert('Automation executed!'))
    .catch(e => console.error('Run failed:', e));
}

function toggleAutomationEnabled(id, enabled) {
  const idx = notifications.findIndex(n => n.id === id);
  if (idx >= 0) {
    notifications[idx].enabled = enabled;
    saveAutomationsToESP();
  }
}

function editAutomation(id) {
  openNotifModal(id);
}

function deleteNotification(id) {
  if(confirm('Delete this automation?')) {
    notifications = notifications.filter(n => n.id !== id);
    saveAutomationsToESP();
    renderNotifications();
  }
}

function getSelectedLedMode() {
  const selected = document.querySelector('input[name="led-mode"]:checked');
  return selected ? selected.value : 'normal';
}

function setSelectedLedMode(mode) {
  const radio = document.querySelector('input[name="led-mode"][value="' + mode + '"]');
  if (radio) radio.checked = true;
  toggleLedControlCards();
}

function toggleLedControlCards() {
  const mode = getSelectedLedMode();
  document.getElementById('led-static-card').classList.toggle('hidden', mode !== 'static');
  document.getElementById('led-moving-card').classList.toggle('hidden', mode !== 'moving');
  document.getElementById('led-smart-card').classList.toggle('hidden', mode !== 'smart');
  document.getElementById('led-breathing-card').classList.toggle('hidden', mode !== 'breathing');
  document.getElementById('led-rgb-card').classList.toggle('hidden', mode !== 'rgb');
}

function getColorValues(prefix) {
  const values = [];
  for (let i = 1; i <= 5; i++) {
    const el = document.getElementById(prefix + i);
    values.push(el ? el.value : '#000000');
  }
  return values;
}

function setColorValues(prefix, values) {
  if (!Array.isArray(values)) return;
  for (let i = 1; i <= 5; i++) {
    const el = document.getElementById(prefix + i);
    if (el && values[i - 1]) el.value = values[i - 1];
  }
}

function updateLedWaterPreview(waterPct) {
  const pct = Math.max(0, Math.min(100, parseInt(waterPct || 0, 10)));
  document.getElementById('led-water-pct').innerText = pct;
  const smartColors = getColorValues('led-smart-');
  const litBands = Math.ceil(pct / 20);
  for (let band = 1; band <= 5; band++) {
    const el = document.getElementById('band-' + band);
    if (!el) continue;
    el.style.background = smartColors[band - 1];
    el.style.opacity = (band <= litBands && pct > 0) ? '1' : '0.15';
  }
}

function loadLedStatus() {
  fetch('/led/status')
    .then(r => r.json())
    .then(data => {
      setSelectedLedMode(data.mode || 'normal');
      if (data.static) document.getElementById('led-static-color').value = data.static;
      if (data.breathing) document.getElementById('led-breath-color').value = data.breathing;
      if (typeof data.speed === 'number') {
        document.getElementById('led-speed').value = data.speed;
        document.getElementById('led-speed-label').innerText = Number(data.speed).toFixed(2);
      }
      if (typeof data.rgbSpeed === 'number') {
        document.getElementById('led-rgb-speed').value = data.rgbSpeed;
        document.getElementById('led-rgb-speed-label').innerText = Number(data.rgbSpeed).toFixed(2);
      }
      if (typeof data.brightness === 'number') {
        document.getElementById('led-brightness').value = data.brightness;
        document.getElementById('led-brightness-label').innerText = data.brightness;
      }
      setColorValues('led-moving-', data.moving || []);
      setColorValues('led-smart-', data.smart || []);
      updateLedWaterPreview(data.water || 0);
      toggleLedControlCards();
    })
    .catch(e => console.error('LED status failed:', e));
}

function saveLedConfig() {
  const payload = {
    mode: getSelectedLedMode(),
    speed: parseFloat(document.getElementById('led-speed').value || '0.35'),
    rgbSpeed: parseFloat(document.getElementById('led-rgb-speed').value || '1.00'),
    brightness: parseInt(document.getElementById('led-brightness').value || '255', 10),
    static: document.getElementById('led-static-color').value,
    breathing: document.getElementById('led-breath-color').value,
    moving: getColorValues('led-moving-'),
    smart: getColorValues('led-smart-')
  };

  fetch('/led/config', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(payload)
  })
  .then(r => r.text())
  .then(() => {
    loadLedStatus();
    showSaveToast('Settings Stored');
  })
  .catch(e => console.error('LED save failed:', e));
}

function setLedModeQuick(mode) {
  fetch('/led/config', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({mode: mode})
  })
  .then(r => r.text())
  .then(() => {
    loadLedStatus();
    showSaveToast('LED mode updated');
  })
  .catch(e => console.error('LED quick mode failed:', e));
}

function showSaveToast(message) {
  const toast = document.getElementById('save-toast');
  toast.innerText = message || 'Settings Stored';
  toast.classList.add('show');
  setTimeout(() => toast.classList.remove('show'), 1600);
}

function syncTimeFromDevice() {
  const tzOffsetMin = new Date().getTimezoneOffset();
  fetch('/time/set', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({epochMs: Date.now(), tzOffsetMin: tzOffsetMin})
  }).catch(() => {});
}

function setRunInputMode(mode) {
  runInputMode = mode;
  const header = document.getElementById('run-mode-header');
  const inputs = [1,2,3,4].map(i => document.getElementById('t' + i));
  if (mode === 'ml') {
    header.innerText = 'ML Dispensed';
    inputs.forEach(i => { i.value = 215; i.min = 1; i.step = 1; });
  } else {
    header.innerText = 'Timed Run (s)';
    inputs.forEach(i => { i.value = 10; i.min = 1; i.step = 1; });
  }
}

function runPumpCalibration(pump) {
  fetch('/pump?api=1&p=' + pump + '&t=10');
}

function savePumpCalibration(pump) {
  const ml = parseFloat(document.getElementById('pumpMl' + pump).value || '0');
  if (ml <= 0) return;
  fetch('/calibrate?type=pumpMl&sensor=' + pump + '&ml=' + ml + '&sec=10')
    .then(() => {
      showSaveToast('Settings Stored');
      updateStatus();
    });
}

function updateStatus() {
  fetch('/status')
    .then(r => r.json())
    .then(data => {
      if (data.lidOff) {
        document.getElementById("waterLvl").innerText = "LID OFF";
      } else {
        document.getElementById("waterLvl").innerText = data.water + "%";
      }
      if (data.rtcTime) document.getElementById("dashTime").innerText = data.rtcTime;
      if (data.rtcDay) document.getElementById("dashDay").innerText = data.rtcDay;
      if (data.apMode && !apNoticeShown) {
        apNoticeShown = true;
        alert('AP mode active: WhatsApp and NTP clock sync will not work without internet.');
      }
      overrideEnabled = !!data.override;
      if (Array.isArray(data.pumpMl) && data.pumpMl.length === 4) {
        pumpMlRates = data.pumpMl.map(v => Number(v) || 21.5);
      }
      for(let i=1; i<=4; i++) {
        if (data.raw[i-1] < 900) {
          document.getElementById("m" + i).innerText = "Disconected";
        } else {
          document.getElementById("m" + i).innerText = data.m[i-1] + "%";
        }
        document.getElementById("raw" + i).innerText = data.raw[i-1];
        document.getElementById("pumpDot" + i).classList.toggle("on", !!data.pumps[i-1]);
        const calEl = document.getElementById('pumpMl' + i);
        if (calEl && pumpMlRates[i-1]) calEl.value = (pumpMlRates[i-1] * 10).toFixed(1);
        document.getElementById('temp').innerText = data.temperature;
      }
      if(data.waterRaw) {
        document.getElementById("wRaw").innerText = data.waterRaw;
      }
      updateLedWaterPreview(data.water);
      document.getElementById("status-dot").style.backgroundColor = "#4CAF50";
      document.getElementById("conn-status").innerText = "Connected";
      document.getElementById("last-upd").innerText = new Date().toLocaleTimeString();
    })
    .catch(e => {
      document.getElementById("status-dot").style.backgroundColor = "#f44336";
      document.getElementById("conn-status").innerText = "Disconnected";
    });
}

window.addEventListener("DOMContentLoaded", function() {
  updateSliders();
  setRunInputMode('sec');
  updateStatus();
  setInterval(updateStatus, 2000);
  loadRoutinesFromESP();
  loadAutomationsFromESP();
  loadLedStatus();
  syncTimeFromDevice();

  document.querySelectorAll('input[name="led-mode"]').forEach(el => {
    el.addEventListener('change', toggleLedControlCards);
  });

  document.getElementById('led-speed').addEventListener('input', function() {
    document.getElementById('led-speed-label').innerText = Number(this.value).toFixed(2);
  });
  document.getElementById('led-rgb-speed').addEventListener('input', function() {
    document.getElementById('led-rgb-speed-label').innerText = Number(this.value).toFixed(2);
  });
  document.getElementById('led-brightness').addEventListener('input', function() {
    document.getElementById('led-brightness-label').innerText = this.value;
  });

  document.querySelectorAll('#led-smart-card input[type="color"]').forEach(el => {
    el.addEventListener('input', function() {
      updateLedWaterPreview(document.getElementById('waterLvl').innerText || 0);
    });
  });
});

)rawliteral"; // <--- Added the semicolon here

#endif
