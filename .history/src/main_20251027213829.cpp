#include <Arduino.h>

#define TOP 255
#define OPT_TEMP 25.0f
#define RND_MARGIN 10

// function declarations:
void SetupPWM();
void SetupADC_GPIO();
float getTemp(uint8_t);
void controlDamper(uint8_t, uint8_t);
void controlHeat(uint8_t);
void globalState(float, float);
uint8_t tempToDuty(float);
uint8_t roundOutDuty(uint8_t, uint8_t);

// declare gloal variables:
uint32_t lastTick = 0;
uint8_t dc1_prev = 0;
uint8_t dc2_prev = 0;

void setup() {
  
  SetupPWM();
  SetupADC_GPIO();

  // initialize both heaters (HIGH) and dampers to off (DC = 0%)
  controlHeat(0);
  controlDamper(1, 0); controlDamper(2, 0);

}

void loop() {
  
  // using if statement to create 1 second intervals where CPU does not halt
  if ( millis() - lastTick >= 1000){
    lastTick += 1000; 

    // read temperatures
    float T1 = getTemp(1);
    float T2 = getTemp(2);

    // determine global state and control heaters
    globalState(T1, T2);  

    // map temperatures to target damper duty cycles
    uint8_t dc1_target = tempToDuty(T1);
    uint8_t dc2_target = tempToDuty(T2);

    // Smooth with two-point moving avg
    uint8_t dc1 = roundOutDuty(dc1_prev, dc1_target);
    uint8_t dc2 = roundOutDuty(dc2_prev, dc2_target);

    controlDamper(1, dc1);
    controlDamper(2, dc2);

    // remember prev duty cycle vals
    dc1_prev = dc1; 
    dc2_prev = dc2;


  }
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

  // Set ADC0 and 1 as input
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
    ADMUX = (ADMUX & 0xF0) | 0x00; // ADC0
  } 
  else if (rmNum == 2) {
    ADMUX = (ADMUX & 0xF0) | 0x01; // ADC1
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
  
  // clamp val to 0-100%
  if (val > 100) val = 100;

  // linear map for 0-255 (0-100% DC)
  uint8_t duty = ((uint16_t)val * TOP) / 100;

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
      PORTB |= (1<<PB1);
      PORTB &= ~(1 << PB2); // stage 2 on (LOW)
      break;

    default: // default as stage 0
      PORTB |= (1<<PB1);
      PORTB |= (1<<PB2); // both heaters off
      break;
  }

}


void globalState(float T1, float T2) {

  uint8_t stage = 0;
    
  if (T1 < (OPT_TEMP-1.0) && T2 < (OPT_TEMP-1.0))        stage = 2;   // both below optimal temp
  else if (T1 < (OPT_TEMP-0.5) || T2 < (OPT_TEMP-0.5))   stage = 1;   // at least one below optimal temp
  else if (T1 > (OPT_TEMP+0.5) && T2 > (OPT_TEMP+0.5))   stage = 0;   // both above optimal temp
    
  controlHeat(stage);

}

uint8_t tempToDuty(float temp) {

  // linear map: 0% at 25C, 100% at 0C
  float duty_f = (((OPT_TEMP - temp) * 100.0f ) / 25.0f);

  // clamp to 0-100%
  if (duty_f > 100) duty_f = 100;
  if (duty_f < 0) duty_f = 0;

  // round to nearest integer
  int duty = (uint8_t)(duty_f + 0.5f);

  return duty;
}

// smooth out sudden changes using two-point moving avg
uint8_t roundOutDuty(uint8_t prev, uint8_t curr) {

  // return avg if change exceeds margin
  if (curr > prev + RND_MARGIN || curr + RND_MARGIN < prev) {
      return (uint8_t)((prev + curr)/2);
  } 
  return curr;
}