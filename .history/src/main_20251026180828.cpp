#include <Arduino.h>

#define TOP 255

// put function declarations here:
void SetupPWM();
void SetupADC_GPIO();
uint16_t getTemp(uint8_t);
void controlDamper(uint8_t, uint8_t);

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

  // Set ADC9 and 10 as input
  DDRD &= ~((1<<PD6) | (1<<PD7));  // ADC9, ADC10
  // Set GPIO pins as input
  DDRB &= ~((1<<PB1) | (1<<PB2));            // inputs
  
  // Pull-up resistor activated for GPIO input pins
  PORTB |= (1<<PB1) | (1<<PB2);

  // Configure ADC
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

uint16_t getTemp(uint8_t rmNum) {

  // Select ADC channel based on room number
  if (rmNum == 1) {
    ADMUX = (ADMUX & 0b11100000) | 0b00000001; // ADC9
  } else if (rmNum == 2) {
    ADMUX = (ADMUX & 0b11100000) | 0b00000010; // ADC10
  }

  // read ADC value and convert
  uint16_t readVal = ADCL;           // read ADCL first as per datasheet
  readVal |= ((uint16_t)ADCH) << 8;  // read ADCH and combine

  float Vtemp = (readVal / 1023.0) * 3.3; // convert ADC value to voltage
  float Temp = 199.6 * Vtemp - 303.2; // given formula

  return Temp;

}

void controlDamper(uint8_t rmNum, uint8_t val) {
  
  if (rmNum == 1) {
    OCR0A = (val * TOP) / 100; // sets DC to val% for Room 1
  } else if (rmNum == 2) {
    OCR0B = (val * TOP) / 100; // sets DC to val% for Room 2
  }

}
