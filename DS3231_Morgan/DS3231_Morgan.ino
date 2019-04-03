/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FileName: DS3231_Morgan.ino
ProjectName: DS3231 + LCM + RELAY
FunctionDesc: 
CreateDate:
Version:
Author: Morgan
ModifyHistory:	2019'4'3
Remark:
<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#include <DS3231.h>
#include <LCD4Bit_mod.h>
//create object to control an LCD.
//number of lines in display=1
LCD4Bit_mod lcd = LCD4Bit_mod(2);
#define RELAY_PIN 3
#define LCD_BackLight 10
#define LCD_BL_DURATION (unsigned long)15000 // unit: ms

// millis(): Get System Tick

int adc_key_val[5] = {30, 150, 360, 535, 760};
int NUM_KEYS = 5;
int adc_key_in;
int key = -1;
int oldkey = -1;

int SprinkleTime;
byte SprinkleVal, SprinkleTmp;
volatile unsigned long LongSprinkleTime;

char blink[] = "                ";
char SEC[] = "Sec:            ";
char TempStr[5], TmpArray[16], *pTmpArray;
byte BTN_No; // 0: no pressed, 1~5: BTN No
boolean Adjust_MIN;
Time t;
byte T1Hour, T1Min, T2Hour, T2Min;

// Timer
volatile unsigned char TASK_DS3231_STEP;
volatile unsigned long TASK_DS3231_TIMER_START;

volatile unsigned char TASK_LCD_STEP;
volatile unsigned long TASK_LCD_TIMER_START;

volatile unsigned char TASK_BTN_STEP;
volatile unsigned long TASK_BTN_TIMER_START;

volatile boolean TASK_RELAY_SHOW_GO;
volatile unsigned char TASK_RELAY_SHOW_STEP;
volatile unsigned long TASK_RELAY_SHOW_TIMER_START;

volatile boolean TASK_RELAY_ACTION_GO;
volatile unsigned char TASK_RELAY_ACTION_STEP;
volatile unsigned long TASK_RELAY_ACTION_TIMER_START;

volatile unsigned char TASK_LCD_BL_STEP;
volatile unsigned long TASK_LCD_BL_TIMER_START;

// Init the DS3231 using the hardware interface
DS3231 rtc(SDA, SCL);

void TASK_INIT();
void TASK_DS3231();
void TASK_LCD();
void TASK_BTN();
void TASK_DS3231_NORMAL();
void TASK_RELAY();
void TimeIntToStr(byte data, char *output);
char IntToStr(int data);
void LCD_BL_ON();
// --------------------------------------------------------------
void setup()
{
	pinMode(RELAY_PIN, OUTPUT); //we'll use the debug LED to output a heartbeat
	pinMode(LCD_BackLight, OUTPUT);

	lcd.init();
	//optionally, now set up our application-specific display settings, overriding whatever the lcd did in lcd.init()
	//lcd.commandWrite(0x0F);//cursor on, display on, blink on.  (nasty!)
	lcd.clear();
	lcd.printIn("                ");
	lcd.cursorTo(2, 0); //line=2, x=0
	lcd.printIn("Normal          ");

	// Setup Serial connection
	Serial.begin(115200);

	// Initialize the rtc object
	rtc.begin();

	TASK_INIT();

	// The following lines can be uncommented to set the date and time
	//rtc.setDOW(WEDNESDAY);     // Set Day-of-Week to SUNDAY
	//rtc.setTime(12, 0, 0);     // Set the time to 12:00:00 (24hr format)
	//rtc.setDate(1, 1, 2014);   // Set the date to January 1st, 2014
}
// --------------------------------------------------------------
void loop() // reserved function for Arduino
{
	// Tasks
	TASK_DS3231();
	TASK_LCD();
	TASK_BTN();
	TASK_RELAY();
	TASK_LCD_BL();
}
// --------------------------------------------------------------
void TASK_INIT()
{
	digitalWrite(RELAY_PIN, LOW); // High enable

	// Sprinkle Time
	SprinkleTime = 30; // unit: sec
	SprinkleVal = 0;	// 0: hundred, 1: decade, 2: digits
	
	// Button
	BTN_No = 0;

	// RTC
	TASK_DS3231_STEP = 0;
	TASK_DS3231_TIMER_START = 0;
	Adjust_MIN = false;

	// LCD
	TASK_LCD_STEP = 0;
	TASK_LCD_TIMER_START = 0;
	TASK_LCD_BL_STEP = 0;
	TASK_LCD_BL_TIMER_START = 0;
	digitalWrite(LCD_BackLight, HIGH); // High: enable, LOW: disable

	// BTN
	TASK_BTN_STEP = 0; // start from Normal mode
	TASK_BTN_TIMER_START = 0;

	// RELAY
	TASK_RELAY_SHOW_GO = false;
	TASK_RELAY_SHOW_STEP = 0;
	TASK_RELAY_SHOW_TIMER_START = 0;

	// default Time T1, T2
	T1Hour = 6;
	T1Min = 50;
	T2Hour = 17;
	T2Min = 30;

}
// --------------------------------------------------------------
void TASK_DS3231()
{
	switch (TASK_DS3231_STEP)
	{
	case 0:
		TASK_DS3231_TIMER_START = millis();
		TASK_DS3231_STEP = 2;
		break;

	case 2: // Normal
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
#ifdef RUN
			// Send Day-of-Week
			Serial.print(rtc.getDOWStr());
			Serial.print(" ");

			// Send date
			Serial.print(rtc.getDateStr());
			Serial.print(" -- ");

			// Send time
			Serial.print(rtc.getTimeStr());
			Serial.print(" -- ");

			Serial.print("Temperature: ");
			Serial.print(rtc.getTemp());
			Serial.println(" C");
#endif

			// LCD Display
			TASK_DS3231_NORMAL();

			// TASK_DS3231_STEP = 4;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 4: // adjust hour - 1
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			TASK_DS3231_NORMAL();
			TASK_DS3231_STEP = 6;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 6: // adjust hour - 2
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			lcd.cursorTo(1, 0); //line=1, x=0
			pTmpArray = rtc.getTimeStr();
			*(pTmpArray + 0) = ' ';
			*(pTmpArray + 1) = ' ';
			lcd.printIn(&pTmpArray[0]);
			lcd.cursorTo(1, 10);				   //line=1, x=10
			dtostrf(rtc.getTemp(), 5, 2, TempStr); // float to string
			lcd.printIn(TempStr);
			lcd.print('C');

			// Get int-type data
			// t = rtc.getTime();

			TASK_DS3231_STEP = 4;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 8: // adjust min - 1
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			TASK_DS3231_NORMAL();
			TASK_DS3231_STEP = 10;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 10: // adjust min - 2
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			lcd.cursorTo(1, 0); //line=1, x=0
			pTmpArray = rtc.getTimeStr();
			*(pTmpArray + 3) = ' ';
			*(pTmpArray + 4) = ' ';
			lcd.printIn(&pTmpArray[0]);
			lcd.cursorTo(1, 10);				   //line=1, x=10
			dtostrf(rtc.getTemp(), 5, 2, TempStr); // float to string
			lcd.printIn(TempStr);
			lcd.print('C');

			TASK_DS3231_STEP = 8;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 12: // adjust T1 hour - 1
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, blink, 16); // Clear buffer
			TimeIntToStr(T1Hour, &TmpArray[0]);
			TmpArray[2] = ':';
			TimeIntToStr(T1Min, &TmpArray[3]);
			TmpArray[5] = ':';
			TmpArray[6] = '0';
			TmpArray[7] = '0';
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 14;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 14: // adjust T1 hour - 2
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, blink, 16); // Clear buffer
			// TimeIntToStr(T1Hour, &TmpArray[0]);
			TmpArray[2] = ':';
			TimeIntToStr(T1Min, &TmpArray[3]);
			TmpArray[5] = ':';
			TmpArray[6] = '0';
			TmpArray[7] = '0';
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 12;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 16: // adjust T1 min - 1
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, blink, 16); // Clear buffer
			TimeIntToStr(T1Hour, &TmpArray[0]);
			TmpArray[2] = ':';
			TimeIntToStr(T1Min, &TmpArray[3]);
			TmpArray[5] = ':';
			TmpArray[6] = '0';
			TmpArray[7] = '0';
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 18;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 18: // adjust T1 min - 2
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, blink, 16); // Clear buffer
			TimeIntToStr(T1Hour, &TmpArray[0]);
			TmpArray[2] = ':';
			// TimeIntToStr(T1Min, &TmpArray[3]);
			TmpArray[5] = ':';
			TmpArray[6] = '0';
			TmpArray[7] = '0';
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 16;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 20: // adjust T2 hour - 1
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, blink, 16); // Clear buffer
			TimeIntToStr(T2Hour, &TmpArray[0]);
			TmpArray[2] = ':';
			TimeIntToStr(T2Min, &TmpArray[3]);
			TmpArray[5] = ':';
			TmpArray[6] = '0';
			TmpArray[7] = '0';
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 22;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 22: // adjust T2 hour - 2
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, blink, 16); // Clear buffer
			// TimeIntToStr(T2Hour, &TmpArray[0]);
			TmpArray[2] = ':';
			TimeIntToStr(T2Min, &TmpArray[3]);
			TmpArray[5] = ':';
			TmpArray[6] = '0';
			TmpArray[7] = '0';
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 20;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 24: // adjust T2 min - 1
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, blink, 16); // Clear buffer
			TimeIntToStr(T2Hour, &TmpArray[0]);
			TmpArray[2] = ':';
			TimeIntToStr(T2Min, &TmpArray[3]);
			TmpArray[5] = ':';
			TmpArray[6] = '0';
			TmpArray[7] = '0';
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 26;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 26: // adjust T2 min - 2
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, blink, 16); // Clear buffer
			TimeIntToStr(T2Hour, &TmpArray[0]);
			TmpArray[2] = ':';
			// TimeIntToStr(T2Min, &TmpArray[3]);
			TmpArray[5] = ':';
			TmpArray[6] = '0';
			TmpArray[7] = '0';
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 24;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

		// ------------------------------------------------------
	case 30: // Sprinkle Time - hundred
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, SEC, 16); // Clear buffer
			TmpArray[5] = IntToStr(SprinkleTime / 100);
			TmpArray[6] = IntToStr((SprinkleTime / 10) % 10);
			TmpArray[7] = IntToStr(SprinkleTime % 10);
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 32;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 32: // Sprinkle Time - hundred
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, SEC, 16); // Clear buffer
			TmpArray[5] = ' ';
			TmpArray[6] = IntToStr((SprinkleTime / 10) % 10);
			TmpArray[7] = IntToStr(SprinkleTime % 10);
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 30;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 34: // Sprinkle Time - decade
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, SEC, 16); // Clear buffer
			TmpArray[5] = IntToStr(SprinkleTime / 100);
			TmpArray[6] = IntToStr((SprinkleTime / 10) % 10);
			TmpArray[7] = IntToStr(SprinkleTime % 10);
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 36;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 36: // Sprinkle Time - decade
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, SEC, 16); // Clear buffer
			TmpArray[5] = IntToStr(SprinkleTime / 100);
			TmpArray[6] = ' ';
			TmpArray[7] = IntToStr(SprinkleTime % 10);
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 34;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 38: // Sprinkle Time - digits
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, SEC, 16); // Clear buffer
			TmpArray[5] = IntToStr(SprinkleTime / 100);
			TmpArray[6] = IntToStr((SprinkleTime / 10) % 10);
			TmpArray[7] = IntToStr(SprinkleTime % 10);
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 40;
			TASK_DS3231_TIMER_START = millis();
		}
		break;

	case 40: // Sprinkle Time - digits
		if ((millis() - TASK_DS3231_TIMER_START) >= 100)
		{
			// LCD Display
			memcpy(TmpArray, SEC, 16); // Clear buffer
			TmpArray[5] = IntToStr(SprinkleTime / 100);
			TmpArray[6] = IntToStr((SprinkleTime / 10) % 10);
			TmpArray[7] = ' ';
			lcd.cursorTo(1, 0); //line=1, x=0
			lcd.printIn(TmpArray);

			TASK_DS3231_STEP = 38;
			TASK_DS3231_TIMER_START = millis();
		}
		break;
	}
}
// --------------------------------------------------------------
void TASK_DS3231_NORMAL()
{
	// LCD Display
	lcd.cursorTo(1, 0); //line=1, x=0
	lcd.printIn(rtc.getTimeStr());
	lcd.cursorTo(1, 10);				   //line=1, x=10
	dtostrf(rtc.getTemp(), 5, 2, TempStr); // float to string
	lcd.printIn(TempStr);
	lcd.print('C');
}
// --------------------------------------------------------------
void TASK_LCD()
{
	switch (TASK_LCD_STEP)
	{
	case 0:
		TASK_LCD_TIMER_START = millis();

		adc_key_in = analogRead(0); // read the value from the sensor
		// digitalWrite(13, HIGH);
		key = get_key(adc_key_in); // convert into key press

		if (key != oldkey) // if keypress is detected
		{
			TASK_LCD_TIMER_START = millis();
			TASK_LCD_STEP = 2;
		}
		break;

	case 2:
		if ((millis() - TASK_LCD_TIMER_START) > 50) // 50ms
		{
			adc_key_in = analogRead(0); // read the value from the sensor
			key = get_key(adc_key_in);  // convert into key press
			if ((key != oldkey) && (BTN_No == 0))
			{
				oldkey = key;
				if (key >= 0)
				{
					// lcd.cursorTo(2, 0); //line=2, x=0
					// lcd.printIn(msgs[key]);
					BTN_No = key + 1;
					LCD_BL_ON();
				}
			}

			// digitalWrite(13, LOW);
			TASK_LCD_STEP = 0;
		}
		break;
	}
}
// --------------------------------------------------------------
// LCD Back Light Control
void LCD_BL_ON()
{
	digitalWrite(LCD_BackLight, HIGH); // High: enable, LOW: disable
	TASK_LCD_BL_STEP = 0;
}
// --------------------------------------------------------------
void TASK_LCD_BL()
{
	switch (TASK_LCD_BL_STEP)
	{
	case 0:
		TASK_LCD_BL_TIMER_START = millis();
		TASK_LCD_BL_STEP = 2;
		break;

	case 2:
		if ((millis() - TASK_LCD_BL_TIMER_START) > LCD_BL_DURATION)
		{
			digitalWrite(LCD_BackLight, LOW); // High: enable, LOW: disable
			// TASK_LCD_BL_TIMER_START = millis();
			TASK_LCD_BL_STEP = 99;
		}
		break;

	// case 4:
	// 	if ((millis() - TASK_LCD_BL_TIMER_START) > LCD_BL_DURATION)
	// 	{
	// 		digitalWrite(LCD_BackLight, HIGH); // High: enable, LOW: disable
	// 		TASK_LCD_BL_TIMER_START = millis();
	// 		TASK_LCD_BL_STEP = 2;
	// 	}
	// 	break;
	}
}
// --------------------------------------------------------------
// Convert ADC value to key number
int get_key(unsigned int input)
{
	int k;

	for (k = 0; k < NUM_KEYS; k++)
	{
		if (input < adc_key_val[k])
		{
			return k;
		}
	}

	if (k >= NUM_KEYS)
		k = -1; // No valid key pressed

	return k;
}
// --------------------------------------------------------------
void TASK_BTN()
{
	if (BTN_No)
	{
		switch (TASK_BTN_STEP)
		{

		case 0: // Normal Mode
			switch (BTN_No)
			{
			case 1: // Right
				if (TASK_RELAY_ACTION_GO)
				{
					TASK_RELAY_ACTION_STEP = 4;
				}
				else
				{
					TASK_RELAY_ACTION_GO = true;
					TASK_RELAY_ACTION_STEP = 0;
				}
				break;
			case 2: // Up
				break;
			case 3: // Down
				break;
			case 4: // Left
				break;
			case 5:					// Select
				lcd.cursorTo(2, 0); //line=2, x=0
				lcd.printIn("Time Set        ");
				TASK_BTN_STEP = 2;
				// Display Time
				Adjust_MIN = false;   // default: tune hour first
				TASK_DS3231_STEP = 4; // change LCD display mode
				break;
			}
			break;

		case 2: // Time Set:
			switch (BTN_No)
			{
			case 1: // Right

				break;

			case 2: // Up
				if (Adjust_MIN)
				{
					t.min++;
					if (t.min > 59)
					{
						t.min = 0;
					}
				}
				else
				{
					t.hour++;
					if (t.hour > 23)
					{
						t.hour = 0;
					}
				}
				rtc.setTime(t.hour, t.min, t.sec);
				break;

			case 3: // Down
				if (Adjust_MIN)
				{

					if (t.min == 0)
					{
						t.min = 60;
					}
					t.min--;
				}
				else
				{

					if (t.hour == 0)
					{
						t.hour = 24;
					}
					t.hour--;
				}
				rtc.setTime(t.hour, t.min, t.sec);
				break;
			case 4: // Left
				if (Adjust_MIN)
				{
					TASK_DS3231_STEP = 4; // change LCD display mode
					Adjust_MIN = false;   // jump to next stage
				}
				else
				{
					TASK_DS3231_STEP = 8; // change LCD display mode
					Adjust_MIN = true;
				}
				break;

			case 5:					// Select
				lcd.cursorTo(2, 0); //line=2, x=0
				lcd.printIn("Set T1 Time     ");
				TASK_BTN_STEP = 4;
				// Display Time
				Adjust_MIN = false;	// default: tune hour first
				TASK_DS3231_STEP = 12; // change LCD display mode
				break;
			}
			break;

		case 4: // Target 1 Time:
			switch (BTN_No)
			{
			case 1: // Right

				break;
			case 2: // Up
				if (Adjust_MIN)
				{
					T1Min++;
					if (T1Min > 59)
					{
						T1Min = 0;
					}
				}
				else
				{
					T1Hour++;
					if (T1Hour > 23)
					{
						T1Hour = 0;
					}
				}
				break;
			case 3: // Down
				if (Adjust_MIN)
				{

					if (T1Min == 0)
					{
						T1Min = 60;
					}
					T1Min--;
				}
				else
				{

					if (T1Hour == 0)
					{
						T1Hour = 24;
					}
					T1Hour--;
				}
				break;
			case 4: // Left
				if (Adjust_MIN)
				{
					TASK_DS3231_STEP = 12; // change LCD display mode
					Adjust_MIN = false;	// jump to next stage
				}
				else
				{
					TASK_DS3231_STEP = 16; // change LCD display mode
					Adjust_MIN = true;
				}
				break;
			case 5:					// Select
				lcd.cursorTo(2, 0); //line=2, x=0
				lcd.printIn("Set T2 Time     ");
				TASK_BTN_STEP = 6;
				// Display Time
				Adjust_MIN = false;	// default: tune hour first
				TASK_DS3231_STEP = 20; // change LCD display mode
				break;
			}
			break;

		case 6: // Target 1 Time:
			switch (BTN_No)
			{
			case 1: // Right

				break;
			case 2: // Up
				if (Adjust_MIN)
				{
					T2Min++;
					if (T2Min > 59)
					{
						T2Min = 0;
					}
				}
				else
				{
					T2Hour++;
					if (T2Hour > 23)
					{
						T2Hour = 0;
					}
				}
				break;
			case 3: // Down
				if (Adjust_MIN)
				{

					if (T2Min == 0)
					{
						T2Min = 60;
					}
					T2Min--;
				}
				else
				{

					if (T2Hour == 0)
					{
						T2Hour = 24;
					}
					T2Hour--;
				}
				break;
			case 4: // Left
				if (Adjust_MIN)
				{
					TASK_DS3231_STEP = 20; // change LCD display mode
					Adjust_MIN = false;	// jump to next stage
				}
				else
				{
					TASK_DS3231_STEP = 24; // change LCD display mode
					Adjust_MIN = true;
				}
				break;
			case 5:					// Select
				lcd.cursorTo(2, 0); //line=2, x=0
				lcd.printIn("Sprinkle Time   ");

				TASK_DS3231_STEP = 30; // change LCD display mode
				SprinkleVal = 0;	   // default

				TASK_BTN_STEP = 8;

				break;
			}
			break;

		case 8: // Set sprintkel time
			switch (BTN_No)
			{
			case 1: // Right

				break;
			case 2: // Up
				switch (SprinkleVal)
				{
				case 0:
					SprinkleTmp = SprinkleTime / 100;
					SprinkleTime = SprinkleTime - (SprinkleTmp * 100);
					if (SprinkleTmp == 9)
					{
						SprinkleTmp = 0;
					}else{
						SprinkleTmp++;
					}
					SprinkleTime = (SprinkleTime % 100) + SprinkleTmp * 100;
					break;

				case 1:
					SprinkleTmp = (SprinkleTime / 10) % 10;
					SprinkleTime = SprinkleTime - (SprinkleTmp * 10);
					if (SprinkleTmp == 9)
					{
						SprinkleTmp = 0;
					}
					else
					{
						SprinkleTmp++;
					}
					SprinkleTime = SprinkleTime + SprinkleTmp * 10;
					break;

				case 2:
					SprinkleTmp = SprinkleTime % 10;
					SprinkleTime = SprinkleTime - SprinkleTmp;
					if (SprinkleTmp == 9)
					{
						SprinkleTmp = 0;
					}
					else
					{
						SprinkleTmp++;
					}
					SprinkleTime = SprinkleTime + SprinkleTmp;
					break;
				}
				break;

			case 3: // Down
				switch (SprinkleVal)
				{
				case 0:
					SprinkleTmp = SprinkleTime / 100;
					SprinkleTime = SprinkleTime - (SprinkleTmp * 100);
					if (SprinkleTmp == 0)
					{
						SprinkleTmp = 9;
					}else{
						SprinkleTmp--;
					}
					SprinkleTime = (SprinkleTime % 100) + SprinkleTmp * 100;
					break;

				case 1:
					SprinkleTmp = (SprinkleTime / 10) % 10;
					SprinkleTime = SprinkleTime - (SprinkleTmp * 10);
					if (SprinkleTmp == 0)
					{
						SprinkleTmp = 9;
					}
					else
					{
						SprinkleTmp--;
					}
					SprinkleTime = SprinkleTime + SprinkleTmp * 10;
					break;

				case 2:
					SprinkleTmp = SprinkleTime % 10;
					SprinkleTime = SprinkleTime - SprinkleTmp;
					if (SprinkleTmp == 0)
					{
						SprinkleTmp = 9;
					}
					else
					{
						SprinkleTmp--;
					}
					SprinkleTime = SprinkleTime + SprinkleTmp;
					break;
				}
				break;

			case 4: // Left

				SprinkleVal++;
				if (SprinkleVal > 2)
				{
					SprinkleVal = 0;
				}

				switch (SprinkleVal)
				{
				case 0:
					TASK_DS3231_STEP = 30;
					break;

				case 1:
					TASK_DS3231_STEP = 34;
					break;

				case 2:
					TASK_DS3231_STEP = 38;
					break;
				}
				break;

			case 5:					// Select
				lcd.cursorTo(2, 0); //line=2, x=0
				lcd.printIn("Normal          ");
				TASK_DS3231_STEP = 2; // change LCD display mode
				TASK_BTN_STEP = 0;
				break;
			}
			break;

		} // switch (TASK_BTN_STEP)

		BTN_No = 0; // Reset value
	}
}
// --------------------------------------------------------------
void TASK_RELAY()
{
	// ----------------------------------------------------------
	// Time check for Relay action
	if (TASK_BTN_STEP == 0) // Normal mode
	{
		t = rtc.getTime();

		if ((T1Hour == t.hour) && (T1Min == t.min) && (t.sec < 5))
		{
			TASK_RELAY_ACTION_GO = 1;
		}

		if ((T2Hour == t.hour) && (T2Min == t.min) && (t.sec < 5))
		{
			TASK_RELAY_ACTION_GO = 1;
		}
	}

	// ----------------------------------------------------------
	if (TASK_RELAY_ACTION_GO)
	{
		switch (TASK_RELAY_ACTION_STEP)
		{
		case 0:
			TASK_RELAY_ACTION_TIMER_START = millis();
			TASK_RELAY_SHOW_GO = true;

			digitalWrite(RELAY_PIN, HIGH);
			digitalWrite(LCD_BackLight, HIGH); // High: enable, LOW: disable
			// Set Sprinkle value
			LongSprinkleTime = (unsigned long)SprinkleTime * (unsigned long)1000;

			TASK_RELAY_ACTION_STEP = 2;
			break;

		case 2:
			if ((millis() - TASK_RELAY_ACTION_TIMER_START) >= LongSprinkleTime) // run 10s
			{
				lcd.cursorTo(2, 14); //line=2, x=14
				lcd.printIn("  ");   // erase "GO"

				digitalWrite(RELAY_PIN, LOW);
				digitalWrite(LCD_BackLight, LOW); // High: enable, LOW: disable

				TASK_RELAY_SHOW_STEP = 0;
				TASK_RELAY_SHOW_GO = false;

				TASK_RELAY_ACTION_STEP = 0;
				TASK_RELAY_ACTION_GO = false;
			}
			break;

		}
	}
	// ----------------------------------------------------------
	if (TASK_RELAY_SHOW_GO)
	{
		switch (TASK_RELAY_SHOW_STEP)
		{
		case 0:
			TASK_RELAY_SHOW_TIMER_START = millis();
			TASK_RELAY_SHOW_STEP = 2;
			break;

		case 2:
			if ((millis() - TASK_RELAY_SHOW_TIMER_START) >= 250)
			{
				lcd.cursorTo(2, 14); //line=2, x=14
				lcd.printIn("Go");
				TASK_RELAY_SHOW_TIMER_START = millis();
				TASK_RELAY_SHOW_STEP = 4;
			}
			break;

		case 4:
			if ((millis() - TASK_RELAY_SHOW_TIMER_START) >= 250)
			{
				lcd.cursorTo(2, 14); //line=2, x=14
				lcd.printIn("  ");
				TASK_RELAY_SHOW_TIMER_START = millis();
				TASK_RELAY_SHOW_STEP = 2;
			}
			break;
		}
	}
}
// --------------------------------------------------------------
// Translate byte-type to char string
void TimeIntToStr(byte data, char *output)
{
	// static char output[] = "xx";
	if (data < 10)
	{
		*output = 48;
	}
	else
	{
		*output = char((data / 10) + 48);
	}
	*(output + 1) = char((data % 10) + 48);
}
// --------------------------------------------------------------
char IntToStr(int data)
{
	return (data + '0');
}
// --------------------------------------------------------------
// --------------------------------------------------------------
