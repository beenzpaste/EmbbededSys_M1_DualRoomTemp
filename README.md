# Dual-Room Temperature Control System (Embedded C / AVR)

This project implements a **two-room temperature control system** using the ATmega32U4 microcontroller (Arduino-compatible).  
It was developed for an **Electronics & Embedded Systems laboratory assignment** on **ADC, PWM, and digital control**.

---

## Objective

To design and implement a **feedback-based temperature control system** that:

- Measures two room temperatures using analog sensors connected to **ADC0** and **ADC1**.
- Controls a **two-stage heating system** (active-low GPIO outputs) based on global temperature conditions.
- Modulates **two independent dampers** via **PWM signals** to distribute warm air proportionally between rooms.
- Updates once per second using a non-blocking timing mechanism (`millis()`).

The goal was to combine **analog data acquisition**, **PWM generation**, and **finite-state control** on bare-metal AVR hardware without relying on high-level Arduino libraries.

---

## 💡 Things I Struggled With & What I Learned

**1. Understanding the ADC pipeline**  
At first, it wasn’t clear why the first conversion after switching MUX had to be discarded.  
Learning the *sample-and-hold* mechanism of the AVR ADC made this much clearer.

**2. Bit-level register configuration**  
Manually setting `ADMUX`, `ADCSRA`, `TCCR0A/B` taught me how each bit affects the MCU hardware.  
I now feel comfortable reading datasheets and writing low-level setup code instead of relying on library wrappers.

**3. Timing without blocking**  
Replacing `delay()` with a `millis()`-based scheduler was my first introduction to *non-blocking control loops*.  
It changed how I think about embedded programs — not as sequences but as continuous real-time systems.

**4. Stability near thresholds**  
Initially, the heater stage oscillated between on/off near 25 °C.  
Adding *hysteresis* and later *smoothing margins* taught me how small logic changes can greatly affect system stability.

---

## Author

**Hsin (Sheena) Chiang**  
Electrical Engineering  
Project for *ECE 4144 – Embedded Systems Midterm (Fall 2025)*  
@ NYU Tandon
