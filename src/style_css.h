/*Code from VS Code - NOT WORKING*/

//style_css.h - Contains the CSS styles for the web interface, stored in PROGMEM to save RAM on the microcontroller.
#ifndef STYLE_CSS_H
#define STYLE_CSS_H

#include <Arduino.h>

const char STYLE_CSS[] PROGMEM = R"rawliteral(
:root { 
  --primary: #2196F3; 
  --success: #4CAF50; 
  --danger: #f44336; 
  --dark: #333; 
  --gray: #ddd; 
  --text-gray: #555; 
}

body { 
  font-family: Arial, sans-serif; 
  text-align: center; 
  background-color: #f4f4f9; 
  padding: 20px; 
  color: var(--dark); 
  margin: 0; 
}

.container { 
  max-width: 800px; 
  margin: auto; 
}

.card { 
  background: white; 
  padding: 15px; 
  border-radius: 10px; 
  box-shadow: 0 2px 5px rgba(0,0,0,0.1); 
  margin-bottom: 20px; 
  text-align: left; 
}

#debug-view .card {
  text-align: center;
}

#debug-view .card h3 {
  margin-top: 0;
}

#debug-view .card label {
  display: inline-block;
}

#debug-view .card .save-btn {
  margin: 0 5px;
}

.hidden { 
  display: none !important; 
}
    
/* Navigation */
.nav-group { 
  margin-bottom: 20px; 
  display: flex; 
  justify-content: center; 
  gap: 10px; 
  flex-wrap: wrap; 
}

.nav-btn { 
  cursor: pointer; 
  padding: 10px 18px; 
  border-radius: 4px; 
  border: none; 
  font-weight: bold; 
  display: inline-flex; 
  align-items: center; 
  justify-content: center; 
  gap: 8px; 
  background-color: #555; 
  color: white; 
  transition: 0.2s; 
}

.nav-btn:hover { 
  background-color: #333; 
}

.nav-btn.active { 
  background-color: var(--primary); 
}

/* Dashboard Styles */
table { 
  width: 100%; 
  margin-bottom: 20px; 
  border-collapse: collapse; 
  background: white; 
}

th, td { 
  padding: 12px; 
  border: 1px solid #ddd; 
  text-align: center; 
}

th { 
  background-color: var(--success); 
  color: white; 
}

.status-card { 
  display: flex; 
  justify-content: space-around; 
  align-items: center; 
}

.stat { 
  font-size: 1.2em; 
  font-weight: bold; 
}

.on-btn { 
  background-color: #e7f3e7; 
  color: #2e7d32; 
  border: 1px solid #2e7d32; 
  padding: 5px 10px; 
  border-radius: 4px; 
  cursor: pointer; 
}

.off-btn { 
  background-color: #ffebee; 
  color: #c62828; 
  border: 1px solid #c62828; 
  padding: 5px 10px; 
  border-radius: 4px; 
  cursor: pointer; 
}

.save-btn { 
  background-color: var(--primary); 
  color: white; 
  border: none; 
  padding: 8px 12px; 
  border-radius: 4px; 
  cursor: pointer; 
}
    
.warning-box { 
  background-color: #ffeb3b; 
  color: #000; 
  padding: 15px; 
  border: 3px solid var(--danger); 
  border-radius: 10px; 
  margin-bottom: 20px; 
  font-weight: bold; 
  display: none; 
  align-items: center; 
  justify-content: space-between; 
}

.revert-btn { 
  background-color: var(--danger); 
  color: white; 
  border: none; 
  padding: 8px 15px; 
  border-radius: 5px; 
  cursor: pointer; 
}

#status-dot { 
  height: 12px; 
  width: 12px; 
  background-color: #bbb; 
  border-radius: 50%; 
  display: inline-block; 
  margin-right: 5px; 
}

.updated-text { 
  font-size: 0.8em; 
  color: #666; 
}

/* Modal & Overlay */
.modal-overlay { 
  position: fixed; 
  top: 0; 
  left: 0; 
  width: 100%; 
  height: 100%; 
  background: rgba(0,0,0,0.7); 
  display: flex; 
  align-items: center; 
  justify-content: center; 
  z-index: 1000; 
}

.modal-content { 
  background: white; 
  padding: 25px; 
  border-radius: 12px; 
  width: 95%; 
  max-width: 450px; 
  max-height: 90vh; 
  overflow-y: auto; 
  text-align: left; 
}
    
/* Day Selector */
.day-selector { 
  display: flex; 
  justify-content: space-between; 
  margin: 15px 0; 
}

.day-btn { 
  width: 35px; 
  height: 35px; 
  border-radius: 50%; 
  border: 1px solid #ddd; 
  background: #fff; 
  cursor: pointer; 
  font-size: 12px; 
  transition: 0.2s; 
  font-weight: bold; 
}

.day-btn.active { 
  background: var(--primary); 
  color: white; 
  border-color: var(--primary); 
}

.day-error { 
  border: 2px solid var(--danger) !important; 
  background: #fff8f8; 
}

/* Matrix Selection */
.matrix-title { 
  font-size: 0.85em; 
  font-weight: bold; 
  margin-top: 15px; 
  color: var(--text-gray); 
  text-transform: uppercase; 
}

.matrix { 
  display: grid; 
  grid-template-columns: repeat(4, 1fr); 
  gap: 8px; 
  margin: 8px 0; 
}

.matrix-box { 
  border: 1px solid #ddd; 
  padding: 10px 5px; 
  text-align: center; 
  border-radius: 6px; 
  cursor: pointer; 
  font-size: 0.85em; 
  background: #fff; 
  transition: 0.2s; 
}

.matrix-box.active { 
  background: #e3f2fd; 
  border: 2px solid var(--primary); 
  font-weight: bold; 
}

.matrix-error { 
  border: 2px solid var(--danger) !important; 
}

/* Dual Range Slider */
.range-wrapper { 
  position: relative; 
  height: 10px; 
  margin: 25px 0; 
  background: var(--gray); 
  border-radius: 5px; 
}

#slider-highlight { 
  position: absolute; 
  height: 100%; 
  background: var(--primary); 
  border-radius: 5px; 
  z-index: 5; 
}

.range-wrapper input { 
  position: absolute; 
  width: 100%; 
  pointer-events: none; 
  appearance: none; 
  height: 0; 
  background: none; 
  top: 5px; 
  z-index: 10; 
}

input[type="range"]::-webkit-slider-thumb { 
  pointer-events: all; 
  appearance: none; 
  width: 22px; 
  height: 22px; 
  border-radius: 50%; 
  background: var(--primary); 
  cursor: pointer; 
  border: 2px solid white; 
  box-shadow: 0 1px 3px rgba(0,0,0,0.3); 
}

input[type="range"]::-moz-range-thumb { 
  pointer-events: all; 
  width: 22px; 
  height: 22px; 
  border-radius: 50%; 
  background: var(--primary); 
  cursor: pointer; 
  border: 2px solid white; 
  box-shadow: 0 1px 3px rgba(0,0,0,0.3); 
}

.invert-btn { 
  background: var(--primary); 
  color: white; 
  border: none; 
  padding: 5px 12px; 
  border-radius: 4px; 
  cursor: pointer; 
  font-size: 13px; 
  font-weight: bold; 
}

/* Routine Cards */
.routine-item { 
  display: flex; 
  justify-content: space-between; 
  align-items: center; 
  padding: 15px; 
  border-bottom: 1px solid #eee; 
  background: #fff; 
  margin-bottom: 10px; 
  border-radius: 8px; 
  box-shadow: 0 1px 3px rgba(0,0,0,0.1); 
}

.meta-tag { 
  background: #e3f2fd; 
  color: var(--primary); 
  font-size: 10px; 
  padding: 2px 6px; 
  border-radius: 4px; 
  font-weight: bold; 
  border: 1px solid #bbdefb; 
  margin-right: 4px; 
}

.day-label { 
  color: var(--primary); 
  font-size: 11px; 
  font-weight: bold; 
  margin-top: 3px; 
}

/* Action Buttons */
.action-btn { 
  width: 36px; 
  height: 36px; 
  border: none; 
  border-radius: 6px; 
  cursor: pointer; 
  color: white; 
  display: inline-flex; 
  align-items: center; 
  justify-content: center; 
  margin-left: 6px; 
}

.btn-edit { 
  background: #757575; 
  font-size: 16px;
  transform: rotate(-45deg);
}

.btn-run {
  background: var(--success);
  padding: 0;
}

.btn-run svg {
  width: 16px;
  height: 16px;
  fill: white;
}

.btn-del { 
  background: var(--danger); 
}

.btn-del svg { 
  width: 18px; 
  height: 18px; 
  stroke: white; 
  stroke-width: 3.5; 
  fill: none; 
}

/* Form Inputs */
input[type="text"], 
input[type="number"], 
input[type="time"] { 
  width: 100%; 
  padding: 10px; 
  border: 1px solid #ddd; 
  border-radius: 6px; 
  box-sizing: border-box; 
}

.dropdown-select {
  width: 100%;
  padding: 10px;
  border: 1px solid #ddd;
  border-radius: 6px;
  box-sizing: border-box;
  font-family: Arial, sans-serif;
  font-size: 14px;
  background-color: white;
  cursor: pointer;
}

.dropdown-select:focus {
  outline: none;
  border-color: var(--primary);
}

.condition-group {
  margin-top: 10px;
  padding: 15px;
  background: #f9f9f9;
  border-radius: 6px;
  border: 1px solid #e0e0e0;
}

textarea.dropdown-select {
  resize: vertical;
  cursor: text;
}

.input-error { 
  border: 2px solid var(--danger) !important; 
  background-color: #fff8f8; 
}

.error-msg { 
  color: var(--danger); 
  font-size: 11px; 
  font-weight: bold; 
  display: none; 
  margin-top: 4px; 
}

.btn-group { 
  display: flex; 
  gap: 10px; 
  margin: 15px 0; 
}

.btn-tab { 
  flex: 1; 
  padding: 10px; 
  border: 1px solid #ccc; 
  background: #eee; 
  cursor: pointer; 
  border-radius: 6px; 
  font-weight: bold; 
}

.btn-tab.active { 
  background: var(--primary); 
  color: white; 
  border-color: var(--primary); 
}

svg.nav-icon { 
  width: 18px; 
  height: 18px; 
  fill: currentColor; 
}

/* Notifications List */
.notif-item { 
  padding: 10px; 
  border-bottom: 1px solid #eee; 
  font-size: 0.9em; 
}

.notif-time { 
  color: #888; 
  font-size: 0.8em; 
  margin-bottom: 2px; 
}

.notif-card {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  padding: 15px;
  border-bottom: 1px solid #eee;
  background: #fff;
  margin-bottom: 10px;
  border-radius: 8px;
  box-shadow: 0 1px 3px rgba(0,0,0,0.1);
}

.notif-content h4 {
  margin: 0 0 8px 0;
  font-size: 1.1em;
  color: var(--dark);
}

.notif-content p {
  margin: 0;
  font-size: 0.9em;
  color: #666;
}

.notif-timestamp {
  font-size: 0.75em;
  color: #999;
  margin-top: 8px;
}

.automation-detail {
  font-size: 0.85em;
  color: #555;
  margin: 5px 0;
  padding: 6px 10px;
  background: #f5f5f5;
  border-radius: 4px;
  border-left: 3px solid var(--primary);
}

.automation-detail strong {
  color: var(--primary);
}

.led-mode-list {
  display: grid;
  grid-template-columns: repeat(2, minmax(140px, 1fr));
  gap: 10px;
}

.led-mode-list label {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 10px;
  border: 1px solid #ddd;
  border-radius: 8px;
  background: #fafafa;
}

.led-mode-list input[type="radio"] {
  transform: scale(1.1);
}

.led-color-grid {
  display: grid;
  grid-template-columns: repeat(5, minmax(55px, 1fr));
  gap: 8px;
  margin-top: 8px;
}

.led-color-grid label {
  font-size: 12px;
  color: #555;
}

.led-color-grid input[type="color"] {
  width: 100%;
  height: 38px;
  border: 1px solid #ccc;
  border-radius: 6px;
  padding: 2px;
  background: #fff;
}

.led-smart-layout {
  display: flex;
  gap: 18px;
  align-items: center;
  flex-wrap: wrap;
}

.cylinder-wrap {
  min-width: 140px;
}

.cylinder {
  width: 80px;
  height: 190px;
  margin: 0 auto 8px auto;
  border: 3px solid #555;
  border-radius: 40px;
  padding: 6px;
  box-sizing: border-box;
  background: #eef3f8;
  display: flex;
  flex-direction: column;
}

.water-band {
  flex: 1;
  border-radius: 20px;
  margin: 2px 0;
  transition: opacity 0.2s ease;
}

@media (max-width: 650px) {
  .led-mode-list {
    grid-template-columns: 1fr;
  }
  .led-color-grid {
    grid-template-columns: repeat(3, minmax(55px, 1fr));
  }
}
)rawliteral";

#endif
