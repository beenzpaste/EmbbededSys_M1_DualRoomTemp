#include <Arduino.h>

// put function declarations here:
void SetupPWM();
void SetupADC_GPIO();

void setup() {
  
  
}

void loop() {
  
}

// put function definitions here:
void SetupPWM() {
  
  // Set PWM pins as output
  DDRB |= (1 << PB7); // Set OC0A as output
  DDRD |= (1 << PD0); // Set OC0B as output

  // Configure Timer0
  TCCR0A = 0b10100011; 
  //         10 - Non-inverting mode on OC0A
  //           10 - Non-inverting mode on OC0B
  //             xx - N/A
  //               11 - WGM(1)(0): Fast PWM mode, TOP at 0xFF

  TCCR0B = 0b00000011; // No prescaling
  //         00 - Force output compare not used in Fast PWM
  //           xx - N/A
  //             0 - WGM(2): Fast PWM mode, TOP at 0xFF
  //               011 - Prescaler = 64


}

void SetupADC_GPIO() {

  ADMUX = 0b01000001;
  //        01 - AVcc as reference
  //          0 - right adjust
  //           00001 - initialized to ADC9

  ADCSRA = 0b11100011;
  //         1 - ADC enabled
  //          1 - start conversion
  //           1 - auto trigger enabled
  //            00 - no interrupt 
  //               011 - prescaler = 8

  ADCSRB = 0b00100000;
  //         0 - not high speed mode
  //          0 - not used when ADC is on
  //           1 - MUX(5)
  //            x - N/A
  //             0000 - free running mode

  

}