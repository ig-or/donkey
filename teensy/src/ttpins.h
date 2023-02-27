#pragma once 

const int ledPin = 13;
const int bvPin = 25;   ///< main battery voltage from the bridge

/*Motor speed inputs: A PWM (pulse-width modulation) signal 
on these pins corresponds to a PWM output on the corresponding channel’s motor outputs. 
When a PWM pin is low, the corresponding motor brakes low 
(both A and B are shorted together through ground). 
When it is high, the motor is on. 
The maximum allowed PWM frequency is 100 kHz.
*/
const int m1pwm = 14;
const int m2pwm = 36;

/*Current sense outputs: These pins output voltages proportional to the motor currents 
when the H-bridges are driving (but not while they are braking, including when current limiting is active). 
The output voltage is about 10 mV/A for the 18v22 and 20 mV/A for the other versions, 
plus an approximate offset of 50 mV.
*/
const int m1cs = 41;
const int m2cs = 38;

/*
Motor direction inputs: When DIR is low, 
motor current flows from output A to output B; 
when DIR is high, current flows from B to A.
*/
const int m1dir = 15;
const int m2dir = 37;

/*Fault indicators: These open-drain outputs drive low when a fault is occurring. 
In order to use these outputs, they must be pulled up to your system’s logic voltage 
(such as by enabling the internal pull-ups on the Arduino pins they are connected to). 
*/
const int m1flt = 17;
const int m2flt = 40;

/*
	Inverted sleep inputs: These pins are pulled high, enabling the driver by default. 
    Driving these pins low puts the motor driver channels into a low-current sleep mode 
    and disables the motor outputs (setting them to high impedance).
*/
const int m1slp = 16;
const int m2slp = 39;

/*
const int m1encA = 2;
const int m1encB = 3;
const int m2encA = 4;
const int m2encB = 5;

*/
const int rcv_ch1pin = 2;
const int rcv_ch2pin = 3;

/**   IR
 * 
 * */
const int IR_RECV_PIN = 6;

const int led1_pin = 33;

	// =========  memsic IMU ==============
const int   memsicDataReadyPin	=	32;	// Data Ready (SPI Communication Data Ready) / SPI-UART Port Select
const int   memsicSPISelectPin	=	0;	// SPI Chip Select (SS)
//const int   memsicResetPin	=		17;	// External Reset (NRST)
const int imuPwrPin = 34;
//const int   memsicSamplingPin	=	15;	// Inertial-Sensor Sampling Indicator (sampling upon falling edge)
//const int   memsicSyncInputPin	=	24;	// Synchronization Input (1KHz Pulse used to synchronize SPI Comm) / 1PPS Input (External GPS)
const int   memsicDataRate_		=	200;			
#define   memsicSPI   SPI1

