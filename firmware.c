// PC0 - I2C1_SDA
// PC1 - I2C1_SCL
// PC2 - USART1_RX
// PC3 - USART1_TX
#include "iostm8l152c6.h"

long X1,X2,B5,T,B6,X3,B3,p;
unsigned long B4,B7;
long UT, UP;

short AC1, AC2, AC3, B1, B2, MB, MC, MD;     
unsigned short AC4, AC5, AC6;
    
long pow(int number, int power) {
  long result = 1;
  for (int i=0; i<power; i++) {
      result *= number;
  }
  return result;
}

void startMeasurement(char action) {
  I2C1_CR2_bit.START = 1;       
  while(!(I2C1_SR1_bit.SB));
  
  I2C1_DR = 0xEE;
  while(I2C1_SR1_bit.ADDR);
  while(!(I2C1_SR1_bit.TXE));
  while(!(I2C1_SR3_bit.TRA));
  
  I2C1_DR = 0xF4;
  while(!(I2C1_SR1_bit.TXE));
  
  I2C1_DR = action;
  while(!(I2C1_SR1_bit.TXE));
  while(!(I2C1_SR1_bit.BTF));
  
  I2C1_CR2_bit.STOP = 1;
}

char getData(char address) {
  char result;
  
  I2C1_CR2_bit.START = 1; 
  while(!(I2C1_SR1_bit.SB));
  
  I2C1_DR = 0xEE;
  while(!(I2C1_SR1_bit.ADDR));
  while(!(I2C1_SR1_bit.TXE));
  while(!(I2C1_SR3_bit.TRA));
  
  I2C1_DR = address;
  while(!(I2C1_SR1_bit.TXE));
  while(!(I2C1_SR1_bit.BTF));
    
  I2C1_CR2_bit.START = 1; // Repeated Start
  while(!(I2C1_SR1_bit.SB));
  
  I2C1_DR = 0xEF;
  while(!(I2C1_SR1_bit.ADDR));
  while(I2C1_SR3_bit.TRA);
 
  while(!(I2C1_SR1_bit.RXNE));  
  result = I2C1_DR;  
  
  while(!(I2C1_SR1_bit.BTF));
  
  I2C1_CR2_bit.STOP = 1;
  
  return result;
}

unsigned short getTwoByte(char address) {
  char MSB = getData(address);
  char LSB = getData(address+1);   
  return (MSB<<8) + LSB;
}

void delay (int time) {
  for (int i=0; i<time*1000; i++);
}

void outputLong(long value) {
  while(!(USART1_SR_bit.TXE));
  USART1_DR = value>>24;
  while(!(USART1_SR_bit.TXE));
  USART1_DR = value>>16;
  while(!(USART1_SR_bit.TXE));
  USART1_DR = value>>8;
  while(!(USART1_SR_bit.TXE));
  USART1_DR = value;
}

void clear() {
  X1=0; X2=0; B5=0; T=0; B6=0; X3=0; B3=0; p=0;
  B4=0;B7=0;
}

int main ( void )
{
  CLK_CKDIVR = 0x04;           // System clock source /16 == 1Mhz
  CLK_ICKCR_bit.HSION = 1;     // Clock ready
  CLK_PCKENR1_bit.PCKEN13 = 1; // I2C clock enable
  CLK_PCKENR1_bit.PCKEN15 = 1; // UART clock enable 
  
  // UART init
  PC_DDR_bit.DDR3 = 1;
  PC_CR1_bit.C13 = 1;
  PC_CR2_bit.C23 = 0;

  USART1_CR1 = 0;
  USART1_CR3 = 0;
  USART1_CR4 = 0;
  USART1_CR5 = 0;
  
  USART1_BRR2 = 0x08;
  USART1_BRR1 = 0x06;
  
  USART1_CR2_bit.TEN = 1;
  USART1_CR2_bit.REN = 1;
  USART1_CR2_bit.RIEN = 1;
  
  // I2C init
  I2C1_FREQR = 0x01;            // Program the peripheral input clock in I2C_FREQR Register 
                                // in order to generate correct timings.
  I2C1_CCRL = 0x32;             // Configure the clock control registers
  I2C1_TRISER = 0x02;           // Configure the rise time register
  I2C1_CR1_bit.PE = 1;          // Program the I2C_CR1 register to enable the peripheral
  
  I2C1_OARL = 0xA0;
  I2C1_OARH_bit.ADDCONF = 1;
  
  // Get module registers
  AC1 = getTwoByte(0xAA);
  AC2 = getTwoByte(0xAC);        
  AC3 = getTwoByte(0xAE);     
  AC4 = getTwoByte(0xB0);
  AC5 = getTwoByte(0xB2);
  AC6 = getTwoByte(0xB4);
  B1 = getTwoByte(0xB6); 
  B2 = getTwoByte(0xB8);
  MB = getTwoByte(0xBA);
  MC= getTwoByte(0xBC);
  MD = getTwoByte(0xBE);
  
  asm("RIM");
  
  while (1);
}

#pragma vector=USART_R_OR_vector
__interrupt void USART_RXNE(void)
{ 
  char recive = USART1_DR;
  
  if (recive == '1') {
    clear();
    // Start temperature measurement
    startMeasurement(0x2E);
    delay(5);
    // Get data    
    UT = getTwoByte(0xF6);
        
    // Start pressure measurement 
    startMeasurement(0xF4);
    delay(5);
    // Get data
    UP = (getTwoByte(0xF6)<<8) + getData(0xF8);
    
    // Calculate true temperature
    X1 = (UT-AC6)*AC5/pow(2,15);
    X2 = MC*pow(2,11)/(X1+MD);
    B5 = X1 + X2;
    T = (B5+8)/pow(2,4);
    
    outputLong(T);
    
    // Calculate true pressure
    B6 = B5 - 4000;
    X1 = (B2 * (B6*B6/pow(2,12)))/pow(2,11);
    X2 = AC2*B6/pow(2,11);
    X3 = X1 + X2;
    B3 = ((AC1*4+X3)+2)/4;
    X1 = AC3*B6/pow(2,13);
    X2 = (B1*(B6*B6/pow(2,12)))/pow(2,16);
    X3 = ((X1+X2) + 2)/4;
    B4 = AC4 * (unsigned long)(X3 + 32768)/pow(2,15);
    B7 = ((unsigned long)UP - B3)*(50000);
      if (B7 < 0x80000000) {
        p = (B7*2)/B4;
      } else {
        p = (B7/B4)*2;
      }
    X1 = (p/pow(2,8))*(p/pow(2,8));
    X1 = (X1*3038)/pow(2,16);
    X2 = (-7357*p)/pow(2,16);
    p = p + (X1+X2+3791)/pow(2,4);
    
    outputLong(p);
  }
}