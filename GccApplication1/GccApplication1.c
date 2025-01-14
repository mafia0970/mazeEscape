#define F_CPU 14745600
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <math.h> //included to support power function
#include "lcd.h"

void port_init();
void timer5_init();
void velocity(unsigned char, unsigned char);
void motors_delay();

unsigned char ADC_Conversion(unsigned char);
unsigned char ADC_Value;
unsigned char flag = 0;
unsigned char Left_white_line = 0;
unsigned char Center_white_line = 0;
unsigned char Right_white_line = 0;
unsigned int threshold = 0x10;

//ADC pin configuration
void adc_pin_config (void)
{
 DDRF = 0x00; 
 PORTF = 0x00;
 DDRK = 0x00;
 PORTK = 0x00;
}

//Function to configure ports to enable robot's motion
void motion_pin_config (void) 
{
 DDRA = DDRA | 0x0F;
 PORTA = PORTA & 0xF0;
 DDRL = DDRL | 0x18;   //Setting PL3 and PL4 pins as output for PWM generation
 PORTL = PORTL | 0x18; //PL3 and PL4 pins are for velocity control using PWM.
}

//Function to Initialize PORTS
void port_init()
{
	lcd_port_config();
	adc_pin_config();
	motion_pin_config();	
}

// Timer 5 initialized in PWM mode for velocity control
// Prescale:256
// PWM 8bit fast, TOP=0x00FF
// Timer Frequency:225.000Hz

void timer5_init()
{
	TCCR5B = 0x00;	//Stop
	TCNT5H = 0xFF;	//Counter higher 8-bit value to which OCR5xH value is compared with
	TCNT5L = 0x01;	//Counter lower 8-bit value to which OCR5xH value is compared with
	OCR5AH = 0x00;	//Output compare register high value for Left Motor
	OCR5AL = 0xFF;	//Output compare register low value for Left Motor
	OCR5BH = 0x00;	//Output compare register high value for Right Motor
	OCR5BL = 0xFF;	//Output compare register low value for Right Motor
	OCR5CH = 0x00;	//Output compare register high value for Motor C1
	OCR5CL = 0xFF;	//Output compare register low value for Motor C1
	TCCR5A = 0xA9;	/*{COM5A1=1, COM5A0=0; COM5B1=1, COM5B0=0; COM5C1=1 COM5C0=0}
 					  For Overriding normal port functionality to OCRnA outputs.
				  	  {WGM51=0, WGM50=1} Along With WGM52 in TCCR5B for Selecting FAST PWM 8-bit Mode*/
	
	TCCR5B = 0x0B;	//WGM12=1; CS12=0, CS11=1, CS10=1 (Prescaler=64)
}

void adc_init()
{
	ADCSRA = 0x00;
	ADCSRB = 0x00;		//MUX5 = 0
	ADMUX = 0x20;		//Vref=5V external --- ADLAR=1 --- MUX4:0 = 0000
	ACSR = 0x80;
	ADCSRA = 0x86;		//ADEN=1 --- ADIE=1 --- ADPS2:0 = 1 1 0
}

//Function For ADC Conversion
unsigned char ADC_Conversion(unsigned char Ch) 
{
	unsigned char a;
	if(Ch>7)
	{
		ADCSRB = 0x08;
	}
	Ch = Ch & 0x07;  			
	ADMUX= 0x20| Ch;	   		
	ADCSRA = ADCSRA | 0x40;		//Set start conversion bit
	while((ADCSRA&0x10)==0);	//Wait for conversion to complete
	a=ADCH;
	ADCSRA = ADCSRA|0x10; //clear ADIF (ADC Interrupt Flag) by writing 1 to it
	ADCSRB = 0x00;
	return a;
}

//Function To Print Sesor Values At Desired Row And Coloumn Location on LCD
void print_sensor(char row, char coloumn,unsigned char channel)
{
	
	ADC_Value = ADC_Conversion(channel);
	lcd_print(row, coloumn, ADC_Value, 3);
}

//Function for velocity control
void velocity (unsigned char left_motor, unsigned char right_motor)
{
	OCR5AL = (unsigned char)left_motor;
	OCR5BL = (unsigned char)right_motor;
}

//Function used for setting motor's direction
void motion_set (unsigned char Direction)
{
 unsigned char PortARestore = 0;

 Direction &= 0x0F; 		// removing upper nibbel for the protection
 PortARestore = PORTA; 		// reading the PORTA original status
 PortARestore &= 0xF0; 		// making lower direction nibbel to 0
 PortARestore |= Direction; // adding lower nibbel for forward command and restoring the PORTA status
 PORTA = PortARestore; 		// executing the command
}

void stop (void)
{
	motion_set (0x00);
}

void forward (void) 
{
  motion_set (0x06);
}

void left (void)
{
	motion_set (0x05);
}

void right (void)
{
	motion_set (0x0A);
}

void soft_right (void)
{
	motion_set (0x02);
}
void soft_left (void)
{
	motion_set (0x04);
}

void back(void) //both wheels backward
{
	motion_set(0x09);
}

void init_devices (void)
{
 	cli(); //Clears the global interrupts
	port_init();
	adc_init();
	timer5_init();
	sei();   //Enables the global interrupts
}

//Main Function
int main()
{
	init_devices();
	lcd_set_4bit();
	lcd_init();
	
	while(1)
	{

		Left_white_line = ADC_Conversion(3);	//Getting data of Left WL Sensor
		Center_white_line = ADC_Conversion(2);	//Getting data of Center WL Sensor
		Right_white_line = ADC_Conversion(1);	//Getting data of Right WL Sensor

		flag=0;

		print_sensor(1,1,3);	//Prints value of White Line Sensor1
		print_sensor(1,5,2);	//Prints Value of White Line Sensor2
		print_sensor(1,9,1);	//Prints Value of White Line Sensor3
		lcd_cursor(2,1);
		lcd_string("JAI SHRI RAM");
		
		 if(Center_white_line<=threshold && Left_white_line<=threshold && Right_white_line<=threshold && (flag==0)) 
		 {   
			 flag=1;
			 velocity(100,100);
			 back();
			 _delay_ms(350);
			 if(Center_white_line<=threshold && Left_white_line<=threshold && Right_white_line<=threshold && (flag==1))
			 {
				 stop();
				 _delay_ms(500);
			 }
			 else
			 {
				goto next;
			 }
		 }
		 
		 next:
		  if(Center_white_line>threshold && Left_white_line>threshold && Right_white_line>threshold && (flag==0))
		  {  
			  flag=1;
			  stop();
			  _delay_ms(250);
			  forward();
		  }
		

		if((Center_white_line<=threshold) && (Left_white_line>threshold && Right_white_line>threshold) && (flag==0))
		{
			flag=1;
			velocity(100,100);
			forward();
		}
			     /*if((Left_white_line<=threshold) && (Right_white_line<=threshold))
			     {   flag=1;
				     velocity(0,0);
			     }*/
				 
		if((Center_white_line<=threshold) && (Left_white_line<=threshold && Right_white_line>threshold) && (flag==0))
		{ 
			flag=1;
			soft_left();
			forward();
			velocity(0,150);
		}
				
				/*if((Left_white_line<=threshold && Center_white_line>=threshold && Right_white_line>threshold) && (flag==1))
				{
					flag=1;
					soft_left();
					//forward();
					velocity(0,200);
				}*/
				
		if((Center_white_line<=threshold) && (Right_white_line<=threshold && Left_white_line>threshold) && (flag==0))
		{   
			flag=1;
			soft_right();
			forward();
		    velocity(150,0);
		}
				
				/*if((Right_white_line<=threshold && Center_white_line>=threshold && Left_white_line>threshold) && (flag==1))
				{
					flag=1;
					soft_right();
					//forward();
					velocity(200,0);
				}*/
		
		
	    if((Right_white_line<=threshold) && (Center_white_line>threshold) && (Left_white_line>threshold) && (flag==0))
		{
			flag=1;
			right();
			forward();
			velocity(130,50);
			
		}

       if((Right_white_line>threshold) && (Center_white_line>threshold) && (Left_white_line<=threshold) && (flag==0))
 		{
			flag=1;
			left();
			forward();
			velocity(50,130);
		}
		
		
	}
}