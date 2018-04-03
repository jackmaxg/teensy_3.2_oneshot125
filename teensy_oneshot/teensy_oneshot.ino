
#define ESC_OUT 23


#include "kinetis.h"

//FTM page 770 of https://www.pjrc.com/teensy/K20P64M72SF1RM.pdf

void setup() {
  configOneShot125();
}

float val = 0;

void loop() {
  //generate a sawtooth, but with oneshot pwm
  delayMicroseconds(333);
  writeOneShot125(val);
  val += .0001;
  if(val > 1) val = 0;
}

void configOneShot125() {
  pinMode(ESC_OUT, OUTPUT);
  digitalWrite(ESC_OUT, LOW);
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);

  //disable write protection
  FTM2_MODE |= FTM_MODE_WPDIS;
  //enable the FTM
  FTM2_MODE |= FTM_MODE_FTMEN;
  //init
  FTM2_MODE |= FTM_MODE_INIT;
  // set 0 before changing mod
  FTM2_SC = 0x00; 
  //Shouldn't be needed (should default to this), but just in case
  FTM2_CNTIN = 0x0000; 
  //make sure count is 0
  FTM2_CNT = 0x0000; 
  //set mod to be maxiumum value
  FTM2_MOD = 0xFFFF;
  
  /*
   * clock source 
   * 00 no clock selected, counter disabled
   * 01 system clock
   * 10 fixed frequency clock (I think 32 khz)
   * 11 external clock
   * 
   * we're going to disable the clock for now, but we will be setting it to the system clock later once we want to send out a pulse
   */  
  FTM2_SC |= FTM_SC_CLKS(0b00);

  /*  
   * set prescaler value 
   * 000---divide by 1
   * 001---divide by 2
   * 010---divide by 4
   * 011---divide by 8
   * 100---divide by 16
   * 101---divide by 32
   * 110---divide by 64
   * 111---divide by 128
   */
  FTM2_SC |= FTM_SC_PS(0b000);

  //setup channel 0, see page 783 for info on the FTMx_CnSC (Channel n Status and Control) register
  //ELSB:ELSA should be 1:1 for Output Compare, set output on match (not actually that important to us tho)
  FTM2_C0SC &= FTM_CSC_ELSB; 
  FTM2_C0SC &= FTM_CSC_ELSA;
  //don't need DMA
  FTM2_C0SC &= ~FTM_CSC_DMA;
  //MSB:MSA should be 0:1 for output compare mode (MS ~ mode select)
  FTM2_C0SC &= ~FTM_CSC_MSB; 
  FTM2_C0SC |= FTM_CSC_MSA;
  //enable interrupt for this channel
  FTM2_C0SC |= FTM_CSC_CHIE;
  
  //enable IRQ Interrupt, will call the ftm2_isr() function
  NVIC_ENABLE_IRQ(IRQ_FTM2);

  //Set the initial compare value
  FTM2_C0V = 0xFFFF;
}

void writeOneShot125(float val) { 
  //calculate the compareator value, put into
  //compare value = microseconds*clock megahertz 
  //I honestly have no idea why I need to divide by two here
  FTM2_C0V = (uint16_t)((((val*125)+125)*72)/2);
  FTM2_CNT = 0x0000;
  
  //pull the pin high
  digitalWriteFast(ESC_OUT, HIGH);
  
  //enable the clock by selecting the system clock as the clock source
  FTM2_SC |= FTM_SC_CLKS(0b01);
}

//the interrupt service routine for the FTM2
void ftm2_isr(void)
{
  //TODO: check if channel 0 is the channel from FTM 2 throwing the flag, not just if any channel is calling an interrupt
  // probably not ever going to be an issue though, as I don't think anything else should be using FTM2
  
  //end the pulse
  digitalWriteFast(ESC_OUT, LOW);
  //disable the counting by selecting no clock
  FTM2_SC |= FTM_SC_CLKS(0b00);
  //clear all the flags 
  FTM2_STATUS &= 0x00;
  //only channel 2's status flag can be reset with a
  //FTM2_STATUS &= ~(0b1 << channelNum)
}
