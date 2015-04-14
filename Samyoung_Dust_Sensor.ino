//****************************************************************

// OSBSS PM2.5 datalogger code - for Dust sensor DSM501A
// Based on code by Christopher Nafis interface to Shinyei Model PPD42NS Particle Sensor (April 2012)

// Last edited on April 14, 2015

//****************************************************************

#include <EEPROM.h>
#include <DS3234lib3.h>
#include <PowerSaver.h>
#include <SdFat.h>

// Launch Variables   ******************************
char filename[15] = "log.csv";    // Set filename Format: "12345678.123". Cannot be more than 8 characters in length, contain spaces or begin with a number

// Global objects and variables   ******************************
PowerSaver chip;  	// declare object for PowerSaver class
DS3234 RTC;    // declare object for DS3234 class
SdFat sd; 		// declare object for SdFat class
SdFile file;		// declare object for SdFile class

#define POWA 4    // pin 4 supplies power to microSD card breakout
#define LED 7  // pin 7 controls LED
int SDcsPin = 9; // pin 9 is CS pin for MicroSD breakout
int pin = 6; // data from dust sensor
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 60000;  // set sample time in milliseconds. Leave this at 60,000 for current version of code
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
float particle = 0;

// setup ****************************************************************
void setup()
{
  Serial.begin(19200);
  
  pinMode(6,INPUT); // set pin 6 to input to read data from dust sensor
  
  pinMode(POWA, OUTPUT);  // set output pins
  pinMode(LED, OUTPUT);
  
  digitalWrite(POWA, HIGH);    // turn on SD card
  delay(1);    // give some delay to ensure SD card is turned on properly

  if(!sd.begin(SDcsPin, SPI_FULL_SPEED))  // initialize SD card on the SPI bus
  {
    delay(10);
    SDcardError();
  }
  else
  {
    delay(10);
    file.open(filename, O_CREAT | O_APPEND | O_WRITE);  // open file in write mode and append data to the end of file
    delay(1);
    String time = RTC.timeStamp();    // get date and time from RTC
    file.println();
    file.print("Date/Time,LowPulseOccupancy,Ratio,Concentration,Particle");    // Print header to file
    file.println();
    PrintFileTimeStamp();
    file.close();    // close file - very important
                     // give some delay by blinking status LED to wait for the file to properly close
    digitalWrite(LED, HIGH);
    delay(10);
    digitalWrite(LED, LOW);    
  }
}

// loop ****************************************************************
void loop()
{
  duration = pulseIn(pin, LOW);
  lowpulseoccupancy = lowpulseoccupancy + duration;
  
  RTC.timeStamp();    // get date and time from RTC
  if(RTC.second==0)  // check if seconds are 0 - this will log data every minute
  {
    printParticle(); // print particle data to SD card
  }
  
  delay(10);
}

// calcualte and print particle measurement data ****************************************************************
void printParticle()
{
  if(!sd.begin(SDcsPin, SPI_FULL_SPEED))    // very important - reinitialize SD card on the SPI bus
  {
    delay(10);
    SDcardError();
  }
  else
  {
    delay(10);
    file.open(filename, O_WRITE | O_AT_END);  // open file in write mode
    delay(1);
    
    String time = RTC.timeStamp();    // get date and time from RTC
    
    // calculate particle data
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
    particle = 0.2899*pow(ratio,4)-13.787*pow(ratio,3)+224.57*pow(ratio,2)-825.71*ratio+3032.0;
    
    file.print(time);
    file.print(",");
    file.print(lowpulseoccupancy);
    file.print(",");
    file.print(ratio);
    file.print(",");
    file.print(concentration);
    file.print(",");
    file.print(particle/283.0);
    file.println();
    PrintFileTimeStamp();
    file.close();    // close file - very important
                       // give some delay by blinking status LED to wait for the file to properly close
    digitalWrite(LED, HIGH);
    delay(10);
    digitalWrite(LED, LOW);
    
    lowpulseoccupancy = 0; // reset lowpulse occupancy for next measurement
    delay(1000); // put sufficient delay so that second is no longer 0
  }
}

// file timestamps ****************************************************************
void PrintFileTimeStamp() // Print timestamps to data file. Format: year, month, day, hour, min, sec
{ 
  file.timestamp(T_WRITE, RTC.year, RTC.month, RTC.day, RTC.hour, RTC.minute, RTC.second);    // edit date modified
  file.timestamp(T_ACCESS, RTC.year, RTC.month, RTC.day, RTC.hour, RTC.minute, RTC.second);    // edit date accessed
}

// SD card Error response ****************************************************************
void SDcardError()
{
    for(int i=0;i<3;i++)   // blink LED 3 times to indicate SD card write error
    {
      digitalWrite(LED, HIGH);
      delay(50);
      digitalWrite(LED, LOW);
      delay(150);
    }
}

//****************************************************************
