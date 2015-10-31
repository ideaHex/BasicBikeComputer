//#include <SD.h>
//#include <Adafruit_PCD8544.h>
/*  _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _
 * / \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \
 * \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/
 *
 * Sets an E-Bike throttle proportinal to the Cadence of the rider
 * Displays Speed and Cadence on graphic LCD
 *
 * 	for more information visit
 * 		www.ideahex.com
 *
 * Written by Damian Kleiss
 *  _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _
 * / \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \
 * \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \*/

// Includes /
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>


// Definitions //
// Pin outs
//#define	_PIN	1		// Not Used
// Motor control and sensors
const int	CADENCE_PIN		=2;	// external interrupt 0, cadence sensor connected to pin 2
const int	SPEED_PIN		=3;	// external interrupt 1, PWM, Speed sensor connected to pin 3
const int	OVERIDE_PIN		=4;	// overide switch sensor connected to pin 4
const int	THROTTLE_PIN	=5;	// PWM, throttle output connected to pin 5
// LCD
const int	LCD_DC_PIN		=6;	// PWM, LCD Data or Command Select pin
const int	LCD_LIGHT_PIN	=7;	// Base of transistor controlling LEDs on LCD Breakout board
const int	LCD_RST_PIN		=8;	// LCD Reset Pin
// SPI
const int	LCD_CE			=9;	// Chip Enable for LCD
const int	SD_CS_PIN		=10;	// Chip Select for SD Card
const int	MOSI_PIN		=11;	// Master Oout Slave In for SPI, LCD and SD Card
const int	MISO_PIN		=12;	// Master In Slave Out for SPI, LCD and SD Card
const int	SCK_PIN			=13;	// Clock for SPI, LCD and SD Card
// Switches
const int	SW1_PIN			=A0;	// Use Pin Change Interupts
const int	SW2_PIN			=A1;	// Use Pin Change Interupts
const int	SW3_PIN			=A2;	// Use Pin Change Interupts
const int	SW4_PIN			=A3;	// Use Pin Change Interupts
// Battery Monitoring
const int	BATT_V_PIN		=A4;	// E-Bike Battery Voltage
const int	BATT_A_PIN		=A5;	// E-Bike Battery Current
//const int	_PIN		=A6;	// Not used
//const int	_PIN		=A7;	// Not used

//										   (SCLK),  (DIN),	   (D/C),     (CS),    (RST)
Adafruit_PCD8544 LCD_disp = Adafruit_PCD8544(SCK_PIN, MOSI_PIN, LCD_DC_PIN, LCD_CE, LCD_RST_PIN);

// pin 5 - Data/Command select
// pin 4 - LCD chip select
// pin 3 - LCD reset
//										(D/C),(CS),(RST)
// Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_DC_PIN, LCD_CE, LCD_RST_PIN);


const int CADENCE_MAGNETS =5;	// The number of Magnets passing the Cadence sensor for one revolution of the cranks
const int SPEED_MAGENTS	=1;	// The number of Magnets passing the Speed sensor for one revolution of the wheel
//const int WHEEL_CIRCUMFERENCE

const int THROTTLE_MIN =80;		// Min value required before power is applied to the motor
const int THROTTLE_MAX =240;	// A larger throttle value will not result in more power
const int THROTTLE_OFF =0;		// Value to ensure that the motor is off

const int CADENCE_MIN	=15;	// minimum cadence for motor to run
const int CADENCE_MAX	=80;	// cadence value that will result in full throttle
const int MAX_CADENCE_PERIOD =60000/(CADENCE_MIN *CADENCE_MAGNETS);	// Convert rpm to period
const int THROTTLE_STEP =(THROTTLE_MAX-THROTTLE_MIN)/(CADENCE_MAX-CADENCE_MIN);	// Throttle ramp value

// Global Variables //
volatile int throttleValue = 0;

// Cadence Variables
volatile float currentCadence = 0;
volatile int cadenceState = 0;
volatile long cadencePositiveTimer = 0;
volatile long cadencePositivePeriod = 0;
volatile long cadenceNegativeTimer = 0;
volatile long cadenceNegativePeriod = 0;
volatile int cadenceInteruptFlag = 0;

unsigned long cadenceInterruptMillis = 0;
int outputValue=0;

// Variables will change :
int ledState = LOW;             // ledState used to set the LED

// Generally, you shuould use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change :
const long interval = 250;           // interval at which to blink (milliseconds)


void setup()
{
	pinMode(CADENCE_PIN, INPUT);
	pinMode(OVERIDE_PIN, INPUT_PULLUP);	// Use internal pullups
	pinMode(THROTTLE_PIN, OUTPUT);
	pinMode(LCD_LIGHT_PIN, OUTPUT);
	digitalWrite(LCD_LIGHT_PIN, HIGH);
	
	Serial.begin(9600);
	Serial.println("ideaHex Throttle Test");
	Serial.println("Visit ideahex.com\r\n");
	//Serial.print("Y0:256:16\n");
	//Serial.print("ATemperature:255:0:0\n");

	LCD_disp.begin();
	LCD_disp.setContrast(30); // you can change the contrast around to adapt the display
	LCD_disp.clearDisplay();   // clears the screen and buffer
	LCD_disp.setRotation(2); 	// Flip display

	LCD_disp.setTextColor(BLACK);
	LCD_disp.setCursor(0,7);
	LCD_disp.setTextSize(2);
	LCD_disp.println("ideaHex");

	LCD_disp.setTextSize(1);
	LCD_disp.setCursor(0,25);
	LCD_disp.print("E-Bike Cadence");
	LCD_disp.setCursor(13,35);
	LCD_disp.println("Controller");

	LCD_disp.display(); // show splashscreen
	delay(5000);
	//LCD_disp.clearDisplay();   // clears the screen and buffer


	attachInterrupt(0, CadenceInterruptHandler, CHANGE);	// External interrupt for cadence sensor
}

void loop()
{
	unsigned char brightness;
	unsigned long currentMillis = millis();
	if(currentMillis - previousMillis >= interval)
	{

		LCD_disp.clearDisplay();   // clears the screen and buffer
		LCD_disp.setCursor(0,0);
		LCD_disp.setTextSize(1);
		LCD_disp.print("RPM:");
		LCD_disp.setTextSize(2);
		LCD_disp.println("220.0");//currentCadence,1);
		LCD_disp.setTextSize(1);
		LCD_disp.print("PWR:");
		LCD_disp.setTextSize(2);
		LCD_disp.println(throttleValue);
		LCD_disp.display();

		// save the last time you blinked the LED
		previousMillis = currentMillis;
	}

	//if(outputValue++ >= 255) outputValue = 0;
	//analogWrite(THROTTLE_PIN, outputValue);
	//Serial.print("V");
	//Serial.print(outputValue);
	//Serial.print(":\n");
	//delay(10);
	// Just set Throttle to Max if Over ride pressed
	while(digitalRead(OVERIDE_PIN)==LOW)	// Switch pulls to ground when pressed
	{
		analogWrite(THROTTLE_PIN, 255);
	}
	// As there won't be an interrupt if the pedals have stopped
	// check if a period longer than the minimum rpm has passed since
	// the last interrupt. If so set the throttle to minimum
	if(millis() - cadenceInterruptMillis > MAX_CADENCE_PERIOD)
	{
		throttleValue = THROTTLE_OFF;
		//throttleValue++;
	}
	
	// Check if an interrupt has occured
	if(cadenceInteruptFlag == HIGH)
	{
		cadenceInterruptMillis = millis(); // reset time since last interrupt for min rpm
		ProcessCadence();	// Update cadence rpm value
		cadenceInteruptFlag = LOW; // reset flag
			Serial.print("cad = ");
			Serial.print(currentCadence,1);	// Show 1 decimal place
		if(currentCadence > CADENCE_MAX) throttleValue = THROTTLE_MAX;
		else if(currentCadence < CADENCE_MIN) throttleValue = THROTTLE_OFF;
		else throttleValue = ((currentCadence-CADENCE_MIN)*THROTTLE_STEP)+THROTTLE_MIN;
			Serial.print(", Throttle = ");
			Serial.println(throttleValue);
	}
	analogWrite(THROTTLE_PIN, throttleValue); // Actualy output the throttle value
}

void CadenceInterruptHandler()
{
	if(digitalRead(CADENCE_PIN) == HIGH) // Positive period
	{
		cadencePositiveTimer = millis();
		cadenceNegativePeriod = millis() - cadenceNegativeTimer;
	}

	else if(digitalRead(CADENCE_PIN) == LOW) // Negative period
	{
		cadencePositivePeriod = millis() - cadencePositiveTimer;
		cadenceNegativeTimer = millis();
		cadenceInteruptFlag = HIGH;	// set a flag so we know a pulse has occurred
	}
}

void ProcessCadence()
{
	// Check if pedaling forward
	if(cadencePositivePeriod > cadenceNegativePeriod)
	{
		float period = float(cadencePositivePeriod + cadenceNegativePeriod)/60000;
		currentCadence = (1/period)/CADENCE_MAGNETS;
	}
	// If not pedalling forward set cadence to zero
	else currentCadence = 0;
}











