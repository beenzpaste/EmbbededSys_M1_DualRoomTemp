#include <Arduino.h>

#define TOP 255

// put function declarations here:
void SetupPWM();
void SetupADC_GPIO();
float getTemp(uint8_t);
void controlDamper(uint8_t, uint8_t);
void controlHeat(uint8_t);
uint8_t determineState(float);
void configRoom(uint8_t, uint8_t);

void setup() {
  
  SetupPWM();
  SetupADC_GPIO();

  // initialize both heaters (HIGH) and dampers to off (DC = 0%)
  controlHeat(0);
  controlDamper(1, 0); controlDamper(2, 0);

}

void loop() {
  
  float T1 = getTemp(1);
  float T2 = getTemp(2);

  uint8_t state1 = determineState(T1);
  uint8_t state2 = determineState(T2);

  configRoom(1, state1);
  configRoom(2, state2);

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

  TCCR0B = 0b00000011;
  //         00 - Force output compare not used in Fast PWM
  //           xx - N/A
  //             0 - WGM(2): Fast PWM mode, TOP at 0xFF
  //               011 - Prescaler = 64

  // duty cycle initialize to 0%
  OCR0A = 0;
  OCR0B = 0;

}

void SetupADC_GPIO() {

  // Set ADC9 and 10 as input
  DDRF &= ~((1<<PF0) | (1<<PF1));  // ADC0, ADC1

  // Set GPIO pins as outputs
  DDRB |= (1<<PB1) | (1<<PB2);
  PORTB |= (1<<PB1) | (1<<PB2); // initialize HIGH (heaters off)
  

  // Configure ADC
  ADMUX = 0b01000000;
  //        01 - AVcc as reference
  //          0 - right adjust
  //           00000 - initialized to ADC0
  ADCSRA = 0b11100011;
  //         1 - ADC enabled
  //          1 - start conversion
  //           1 - auto trigger enabled
  //            00 - no interrupt 
  //               011 - prescaler = 8
  ADCSRB = 0b00000000;
  //         0 - not high speed mode
  //          0 - not used when ADC is on
  //           0 - MUX(5)
  //            x - N/A
  //             0000 - free running mode
}

float getTemp(uint8_t rmNum) {

  // Select ADC channel based on room number
  if (rmNum == 1) {
    ADMUX = (ADMUX & 0b11100000) | 0b00000000; // ADC0
  } 
  else if (rmNum == 2) {
    ADMUX = (ADMUX & 0b11100000) | 0b00000001; // ADC1
  }

  // read ADC value and convert
  while (!(ADCSRA & (1<<ADIF))) {}   // wait for conversion to be complete
  ADCSRA |= (1<<ADIF);               // clear flag (write 1)
  (void)ADCL; (void)ADCH;           // discard first sample

  while (!(ADCSRA & (1<<ADIF))) {}
  ADCSRA |= (1<<ADIF);
  uint16_t readVal = ADCL;           // read ADCL first as per datasheet
  readVal |= ((uint16_t)ADCH) << 8;  // read ADCH and combine

  float Vtemp = (readVal / 1023.0) * 3.3; // convert ADC value to voltage
  float Temp = 199.6 * Vtemp - 303.2; // given formula

  return Temp;

}

void controlDamper(uint8_t rmNum, uint8_t val) {
  
  // linear map for 0-255 (0-100% DC)
  uint8_t duty = (val * TOP) / 100;

  if (rmNum == 1) {
    OCR0A = duty; // sets DC to val% for Room 1
  } else if (rmNum == 2) {
    OCR0B = duty; // sets DC to val% for Room 2
  }


}

void controlHeat(uint8_t stage) {

  switch (stage) {

    case 0: // stage 0: heaters off
      PORTB |= (1<<PB1);
      PORTB |= (1<<PB2);  // all off (HIGH)
      break;

    case 1: // stage 1: low heat
      PORTB &= ~(1 << PB1);  // stage 1 on (LOW)
      PORTB |= (1 << PB2); // stage 2 off (HIGH)
      break;

    case 2:  // stage 2: high heat
      PORTB &= ~(1 << PB2); // stage 2 on (LOW)
      break;

    default: // default as stage 0
      PORTB |= (1<<PB1);
      PORTB |= (1<<PB2); // both heaters off
      break;
  }

}


uint8_t determineState(float temp) {

  if (temp > 30){
    return 0; // HOT
  }
  else if (temp < 20 && temp > 10) {
    return 2; // COOL
}
  else if (temp < 10) {
    return 3; // COLD
  }
  else {
    return 1; // default to PERFECT
  }
}

void configRoom(uint8_t roomNum, uint8_t state) {

  switch (state) {
    case 0: // HOT
      // linear map for damper control: 0% at 25C --> 100% at 50C
      uint8_t coolDeg = (uint8_t)((getTemp(roomNum) - 25) / 4) * 100;

      controlDamper(roomNum, coolDeg); // damper set in proportion to coolDeg
      controlHeat(0); // heater off
      break;

    case 1: // perfect state
      controlDamper(roomNum, 0); // damper closed
      controlHeat(0); // heater off  
      break;

    case 2: // COOL
      controlDamper(roomNum, 0); // damper fully closed
      controlHeat(1); // low heat
      break;

    case 3: // COLD
      controlDamper(roomNum, 0); // damper off
      controlHeat(2); // high heat

    default:
      controlDamper(roomNum, 0); // damper closed
      controlHeat(0); // heater off
      break;
  }

}

