#ifndef ROUTINES_H
#define ROUTINES_H

#include <Arduino.h>

// Link to functions/variables in Main.cpp
extern void activatePump(int idx, int seconds);
extern bool pumpActive[4];
extern bool manualOverride;

struct Routine {
    bool enabled = false;
    int hour = 8;
    int minute = 0;
    int durationS = 5;
    bool days[7] = {true, true, true, true, true, true, true}; 
    bool moistureTrigger = false;
    int moistureThreshold = 30;
};

Routine pumpRoutines[4];

void checkRoutines(int currentHour, int currentMinute, int currentDay, int moistureLevels[]) {
    if (manualOverride) return; // Safety first

    for (int i = 0; i < 4; i++) {
        if (!pumpRoutines[i].enabled) continue;

        // A. Scheduled Time Check
        if (currentHour == pumpRoutines[i].hour && currentMinute == pumpRoutines[i].minute) {
            if (pumpRoutines[i].days[currentDay]) {
                if (!pumpActive[i]) { // Only start if not already running
                    activatePump(i, pumpRoutines[i].durationS);
                    Serial.printf("ROUTINE: Scheduled water for Pump %d\n", i + 1);
                }
            }
        }

        // B. Moisture Threshold Check
        if (pumpRoutines[i].moistureTrigger) {
            if (moistureLevels[i] < pumpRoutines[i].moistureThreshold) {
                if (!pumpActive[i]) {
                    activatePump(i, pumpRoutines[i].durationS);
                    Serial.printf("ROUTINE: Low moisture (%d%%) on Pump %d\n", moistureLevels[i], i + 1);
                }
            }
        }
    }
}

#endif