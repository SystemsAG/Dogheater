/*
 Name:		Game.ino
 Created:	15.03.2018 16:37:27
 Author:	SystemsAG
*/
/*  Arduino TFT Tutorial
*  Program made by Dejan Nedelkovski,
*  www.HowToMechatronics.com
*/

/*  This program uses the UTFT and URTouch libraries
*  made by Henning Karlsen.
*  You can find and download them at:
*  www.RinkyDinkElectronics.com
*/
#include <EEPROM.h>
#include <UTFT.h> 
#include <URTouch.h>
#include <Adafruit_ILI9341.h>

#include <DallasTemperature.h>	// Temperatursensor lib
#include <OneWire.h>			// Sensor lib

//==== Creating Objects
UTFT    myGLCD(ILI9341_16, 38, 39, 40, 41); //Parameters should be adjusted to your Display/Schield model
URTouch  myTouch(6, 5, 4, 3, 2);

//==== temp sensor
OneWire  oneWire(8);  // Pin 4 mit 4,7 kOhm gegen 5V gelegt für den Sensor
DallasTemperature sensors(&oneWire);	//Auswertungsbibliothek für die Sensordaten
DeviceAddress tempInDeviceAddress; //Sensor Address

//==== Data for the EEPROM
int addrStopTemp = 0;
int addrDeltaStartHeat = 10;
long addrMaxHeatingTime = 0X20;

int PinRelaisHeatOnOff = 9; // Relais for the Heat
bool ButtonHeatState = false; //Button Heat
bool oldButtonHeatState = false; //Button Heat
bool oldVisuHeatState = false; //For the visu button
bool firstTimeStarting = true; //first time starting the heating in the cyclic time
float currentTemp = 0.0; // indoor temp
float stopHeatingTemp = 22.0; // time to stop heating 
float deltaStartHeat = 3.0;
int startHeatingTemp = 0; // this time will be calculate with the stopHeatingTemp - deltaStartHeat in code
int currentHeatTime = 0;

bool changeMaxTemperature = false; //change the parameter for maximal temperature time
bool changeDeltaTime = false; //change the parameter for delta time
bool changeMaxTime = false; //change the parameter for maximal heating time

bool stopHeatingWithMaxTime = false; //redundancy for stopping the heater

const char compile_date[] = __DATE__ " " __TIME__;
int resolution = 12;						//12bit for Delay
											//uint32_t targetTime = 0;                    // for next 1 second timeout
unsigned long lastTempRequest = 0, lastTimerRequest = 0;		//Last Temp and Timer period

int delayInMillis = 0;
int idle = 0;
int startDateMillis = 0;
int DateDelay = 100000;

extern unsigned short stop[1024];
extern unsigned int bird01[0x41A];
extern unsigned short power[1024];
extern unsigned short settings[1024];
extern unsigned short left[1024];
extern unsigned short right[1024];
//extern unsigned short raspberry_pi1600_64b[5248];


volatile unsigned long lastTime = 0,  HeatTimer = 0, maxHeatingStopTimer = 900, WaitTimer = millis(), cyclicTimer = 60 , cyclicVisuTimer = 1000;//Time Interrupt Button and Heater Timer

char currentPage, selectedUnit;

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];
extern uint8_t arial_bold[];

//==== Defining Variables
int x, y;
char stCurrent[20] = "";
int stCurrentLen = 0;
char stLast[20] = "";

// Floppy Bird
int xP = 319;
int yP = 100;
int yB = 30;
int fallRateInt = 0;
float fallRate = 0;
int score = 0;
const int button = 14;
int buttonState = 0;

void setup() {
	// Initial setup
	myGLCD.InitLCD();
	myGLCD.clrScr();
	myTouch.InitTouch();
	myTouch.setPrecision(PREC_HI);

	//===== read old saved values 
	stopHeatingTemp = EEPROM.read(addrStopTemp);
	deltaStartHeat = EEPROM.read(addrDeltaStartHeat);
	addrMaxHeatingTime += sizeof(long);
	EEPROM.get(addrMaxHeatingTime, maxHeatingStopTimer);
	

	startHeatingTemp = stopHeatingTemp - deltaStartHeat; //calculate the start HeatingTemp
		
	//drawDetailedSettingScreen();  // Draws the Home Screen
	drawHomeScreen();
	currentPage = '0'; // Indicates that we are at Home Screen
	selectedUnit = '0'; // Indicates the selected unit for the first example, cms or inches

	//Starten zum auslesen der Sensordaten
	sensors.begin();
	sensors.getAddress(tempInDeviceAddress, 0);
	sensors.setResolution(tempInDeviceAddress, resolution);	
	sensors.setWaitForConversion(false);
	sensors.requestTemperatures();
	//set relais pin to output mode
	pinMode(PinRelaisHeatOnOff, OUTPUT);
	digitalWrite(PinRelaisHeatOnOff, HIGH);
	
	//delayInMillis = 750 / (1 << (12 - resolution));
	delayInMillis = 10000;
	lastTempRequest = millis();
	
	//Serial.begin(2400); // Serielle Ausgabe		
}

void loop() {
/*___________________________________________FUNCTIONAL PART Of TOUCH________________________________________________________________________*/
	// Home Screen
	switch (currentPage)
	{
	case '0':
		
			if (myTouch.dataAvailable()) {
				myTouch.read();
				x = myTouch.getX(); // X coordinate where the screen has been pressed
				y = myTouch.getY(); // Y coordinates where the screen has been pressed
				
				//=====================enter the detailed screen=======================	
				if ((x >= 5) && (x <= 50) && (y >= 0) && (y <= 50)) {
					drawCircleFrame(21, 16, 16); // Custom Function -Highlighs the buttons when it's pressed
					currentPage = '1'; // Go to the detailed site					
					myGLCD.clrScr(); // Clears the screen
					drawDetailedScreen(); // It is called only once, because in the next iteration of the loop, this above if statement will be false so this funtion won't be called. This function will draw the graphics of the first example.
				}
				
				//=====================enter the setting screen=======================	
				if ((x >=250) && (x <= 320) && (y >= 0) && (y <= 50)) {
					drawCircleFrame(301, 16,16); // Custom Function -Highlighs the buttons when it's pressed
					currentPage = '2'; // Go to the setting site					
					myGLCD.clrScr(); // Clears the screen
					drawSettings(); // It is called only once, because in the next iteration of the loop, this above if statement will be false so this funtion won't be called. This function will draw the graphics of the first example.
				}

				//===================== heating button =======================	
				if ((x >= 128) && (x <= 195) && (y >= 140) && (y <= 205)) {
					drawCircleFrame(160, 172, 32); // Custom Function -Highlighs the buttons when it's pressed
					oldButtonHeatState = ButtonHeatState;
					ButtonHeatState = !ButtonHeatState;
				}
			}

			if (millis() - lastTempRequest >= delayInMillis)
			{

				sensors.requestTemperatures();
				currentTemp = sensors.getTempCByIndex(0);

				myGLCD.setFont(BigFont);
				myGLCD.setBrightness(255);
				myGLCD.setColor(ILI9341_ORANGE);
				myGLCD.printNumF(currentTemp, 1, 25, 68);	
								
				lastTempRequest = millis();

			}

			if (ButtonHeatState!=oldVisuHeatState)			
			{
				if (ButtonHeatState)
				{
					myGLCD.setColor(ILI9341_RED);
					myGLCD.fillCircle(85, 120, 10);
					myGLCD.drawCircle(160, 172, 32);
				}
				else
				{
					myGLCD.setColor(ILI9341_GREEN);
					myGLCD.fillCircle(85, 120, 10);
					myGLCD.drawCircle(160, 172, 32);
				}
				oldVisuHeatState = ButtonHeatState;
			}
				
			
			break;

	case '1':
		if (myTouch.dataAvailable()) {
			myTouch.read();
			x = myTouch.getX(); // X coordinate where the screen has been pressed
			y = myTouch.getY(); // Y coordinates where the screen has been pressed
								// If we press the Distance Sensor Button 
			//=====================enter the setting screen=======================	
			if ((x >= 250) && (x <= 320) && (y >= 0) && (y <= 50)) {
				drawCircleFrame(301, 16, 16); // Custom Function -Highlighs the buttons when it's pressed
				currentPage = '0'; // Go to the setting site					
				myGLCD.clrScr(); // Clears the screen
				drawHomeScreen(); // It is called only once, because in the next iteration of the loop, this above if statement will be false so this funtion won't be called. This function will draw the graphics of the first example.
			}			
		}
		//Show current temperature
		if (millis() - lastTempRequest >= delayInMillis)
		{
			sensors.requestTemperatures();
			currentTemp = sensors.getTempCByIndex(0);

			myGLCD.setFont(BigFont);
			myGLCD.setBrightness(255);
			myGLCD.setColor(ILI9341_GREEN);
			myGLCD.printNumF(currentTemp, 1, 25, 68);
			myGLCD.drawCircle(95, 68, 2);
			myGLCD.print("C", 97, 68);

			lastTempRequest = millis();
		}
		//Show current heating time
		if (millis() - lastTimerRequest >= cyclicVisuTimer && ButtonHeatState)
		{
			int HeatingTimeSec = 0;
			HeatingTimeSec = (millis() - HeatTimer) / 1000;
			myGLCD.setFont(BigFont);
			myGLCD.setBrightness(255);
			myGLCD.setColor(ILI9341_RED);
			myGLCD.printNumI(HeatingTimeSec, 25, 188);
			myGLCD.print("sec", 90, 188);

			lastTimerRequest = millis();
						
		}

		break;

	case '2':
		//Touch functions of the setting page 		
			if (myTouch.dataAvailable()) {
				myTouch.read();
				x = myTouch.getX();
				y = myTouch.getY();

				//=====================enter the detailed screen=======================	
				if ((x >= 5) && (x <= 50) && (y >= 0) && (y <= 50)) {
					drawCircleFrame(21, 16, 16); // Custom Function -Highlighs the buttons when it's pressed
					currentPage = '0'; // Go to the detailed site					
					myGLCD.clrScr(); // Clears the screen
					drawHomeScreen(); // It is called only once, because in the next iteration of the loop, this above if statement will be false so this funtion won't be called. This function will draw the graphics of the first example.
				}			
				//Button set maximal temperatur
				if ((x >= 5) && (x <= 155) && (y >= 49) && (y <= 89)) {
					drawFrame(5, 49, 155, 89);
					currentPage = '9';
					myGLCD.clrScr();
					drawButtons();
					changeMaxTemperature = true; //activate the valid flag for changing
					
				}
				//Button set delta for temperatur
				if ((x >= 5) && (x <= 155) && (y >= 107) && (y <= 147)) {
					drawFrame(5, 107, 155, 147);
					currentPage = '9';
					myGLCD.clrScr();
					drawButtons();
					changeDeltaTime = true; //activate the valid flag for changing
					

				}

				//Button set maximal heating time for temperatur
				if ((x >= 5) && (x <= 155) && (y >= 165) && (y <= 205)) {
					drawFrame(5, 165, 155, 205);
					currentPage = '9';
					myGLCD.clrScr();
					drawButtons();
					changeMaxTime = true; //activate the valid flag for changing
					
				}

				// If we press the Birduino Game Button
				if ((x >= 165) && (x <= 315) && (y >= 49) && (y <= 89)) {
					drawFrame(35, 190, 285, 230);
					currentPage = '3';
					myGLCD.clrScr();
					myGLCD.setColor(114, 198, 206);
					myGLCD.fillRect(0, 0, 319, 239);
					drawGround();
					drawPilars(xP, yP);
					drawBird(30);
					delay(1000);
				}

			}
			break;
	
	case '3':
			//==== This section of the code, for the game example, is explained in my next tutorial
	// Birduino Game		
			xP = xP - 3;
			drawPilars(xP, yP);

			yB += fallRateInt;
			fallRate = fallRate + 0.2;//ursprünglich 0,4
			fallRateInt = int(fallRate);
			if (yB >= 220) {
				yB = 220;
			}
			if (yB >= 180 || yB <= 0) {
				restartGame();
			}
			if ((xP <= 85) && (xP >= 30) && (yB <= yP - 2)) {
				restartGame();
			}
			if ((xP <= 85) && (xP >= 30) && (yB >= yP + 60)) {
				restartGame();
			}
			drawBird(yB);

			if (xP <= -51) {
				xP = 319;
				yP = rand() % 100 + 20;
				score++;
			}
			if (myTouch.dataAvailable()) {
				myTouch.read();
				x = myTouch.getX();
				y = myTouch.getY();
				if ((x >= 0) && (x <= 319) && (y >= 50) && (y <= 239)) {
					fallRate = -5;
				}
			}
			/*buttonState = digitalRead(button);
			if (buttonState == HIGH) {
				fallRate = -5;
			}*/

	case '9':	
		//Number panel
			if (myTouch.dataAvailable())
			{
				myTouch.read();
				x = myTouch.getX();
				y = myTouch.getY();

				if ((y >= 10) && (y <= 60))  // Upper row
				{
					if ((x >= 10) && (x <= 60))  // Button: 1
					{
						waitForIt(10, 10, 60, 60);
						updateStr('1');
					}
					if ((x >= 70) && (x <= 120))  // Button: 2
					{
						waitForIt(70, 10, 120, 60);
						updateStr('2');
					}
					if ((x >= 130) && (x <= 180))  // Button: 3
					{
						waitForIt(130, 10, 180, 60);
						updateStr('3');
					}
					if ((x >= 190) && (x <= 240))  // Button: 4
					{
						waitForIt(190, 10, 240, 60);
						updateStr('4');
					}
					if ((x >= 250) && (x <= 300))  // Button: 5
					{
						waitForIt(250, 10, 300, 60);
						updateStr('5');
					}
				}

				if ((y >= 70) && (y <= 120))  // Center row
				{
					if ((x >= 10) && (x <= 60))  // Button: 6
					{
						waitForIt(10, 70, 60, 120);
						updateStr('6');
					}
					if ((x >= 70) && (x <= 120))  // Button: 7
					{
						waitForIt(70, 70, 120, 120);
						updateStr('7');
					}
					if ((x >= 130) && (x <= 180))  // Button: 8
					{
						waitForIt(130, 70, 180, 120);
						updateStr('8');
					}
					if ((x >= 190) && (x <= 240))  // Button: 9
					{
						waitForIt(190, 70, 240, 120);
						updateStr('9');
					}
					if ((x >= 250) && (x <= 300))  // Button: 0
					{
						waitForIt(250, 70, 300, 120);
						updateStr('0');
					}
				}

				if ((y >= 130) && (y <= 180))  // Upper row
				{
					if ((x >= 10) && (x <= 150))  // Button: Clear
					{
						waitForIt(10, 130, 150, 180);
						stCurrent[0] = '\0';
						stCurrentLen = 0;
						myGLCD.setColor(0, 0, 0);
						myGLCD.fillRect(0, 224, 319, 239);
					}
					if ((x >= 160) && (x <= 300))  // Button: Enter
					{
						waitForIt(160, 130, 300, 180);
						if (stCurrentLen > 0)
						{
							for (x = 0; x < stCurrentLen + 1; x++)
							{
								stLast[x] = stCurrent[x];
							}

							if (changeMaxTemperature)
							{
								stopHeatingTemp = atoi(stLast);//Convert array char to int and create new stop heating temperature					
								EEPROM.write(addrStopTemp, stopHeatingTemp);//save new data to the storage								

								changeMaxTemperature = false; //set the activating flag for changing the parameter back
							}
							else if (changeDeltaTime)
							{
								deltaStartHeat = atoi(stLast);//Convert array char to int and create new stop heating temperature				
								EEPROM.write(addrDeltaStartHeat, deltaStartHeat);//save new data to the storage
								startHeatingTemp = stopHeatingTemp - deltaStartHeat;//calculate the start HeatingTemp

								changeDeltaTime = false; //set the activating flag for changing the parameter back
							}
							else if (changeMaxTime)
							{
								maxHeatingStopTimer = atoi(stLast);//Convert array char to int and create new stop heating temperature				
								EEPROM.put(addrMaxHeatingTime, maxHeatingStopTimer);//save new data to the storage
								addrMaxHeatingTime += sizeof(long);

								changeMaxTime = false; //set the activating flag for changing the parameter back 
							}
							
							currentPage = '2'; //go back to the setting page
							myGLCD.clrScr();
							drawSettings();
						}
						else
						{
							myGLCD.setColor(255, 0, 0);
							myGLCD.print("BUFFER EMPTY", CENTER, 192);
							delay(500);
							myGLCD.print("            ", CENTER, 192);
							delay(500);
							myGLCD.print("BUFFER EMPTY", CENTER, 192);
							delay(500);
							myGLCD.print("            ", CENTER, 192);
							myGLCD.setColor(0, 255, 0);
						}
					}
				}
			}
			break;
	default:
		break;
	}
	
	if (!stopHeatingWithMaxTime) //if the function stop with the max heating time, you need to restart the arduino
	{
		//Automatic mode heating
		if (millis() - WaitTimer > (cyclicTimer * 1000))
		{
			if (currentTemp < startHeatingTemp)
			{
				ButtonHeatState = true; //set the button in visu to red 
				oldButtonHeatState = ButtonHeatState;

				digitalWrite(PinRelaisHeatOnOff, LOW);
				//ShowButtonState("HEAT", HIGH);	

				if (firstTimeStarting)
				{
					HeatTimer = millis(); //Timer to shutdown the Heater
					lastTimerRequest = millis(); //For timer at visu
					firstTimeStarting = false;
				}
			}
			else if (currentTemp > stopHeatingTemp)
			{
				ButtonHeatState = false; //set the button in visu to green
				oldButtonHeatState = ButtonHeatState;

				digitalWrite(PinRelaisHeatOnOff, HIGH);
				//ShowButtonState("HEAT", LOW);
				HeatTimer = millis(); //set heat timer to zero
				firstTimeStarting = true;
			}

			WaitTimer = millis(); //set timer to starting position

		}

		//Shutdown the Heater after time
		if (millis() - HeatTimer > (maxHeatingStopTimer * 1000) && ButtonHeatState)
		{
			ButtonHeatState = false;
			oldButtonHeatState = ButtonHeatState;

			//ShowButtonState("HEAT", LOW);
			digitalWrite(PinRelaisHeatOnOff, HIGH);

			HeatTimer = millis(); //set timer to starting position	
			WaitTimer = millis(); //set timer for cyclic time to zero
			firstTimeStarting = true;

			stopHeatingWithMaxTime = true; //stop the whole running process
		}

		//Manuel mode for switching on/off heating
		if (ButtonHeatState != oldButtonHeatState)
		{
			oldButtonHeatState = ButtonHeatState;//show this function once

			if (ButtonHeatState)
			{
				digitalWrite(PinRelaisHeatOnOff, LOW); //Start heating
				//ShowButtonState("HEAT", HIGH);

				if (firstTimeStarting)
				{
					HeatTimer = millis(); //Timer to shutdown the Heater
					lastTimerRequest = millis(); //For timer at visu
					firstTimeStarting = false;
				}
			}
			else
			{
				digitalWrite(PinRelaisHeatOnOff, HIGH);
				HeatTimer = millis(); //set heat timer to zero
				firstTimeStarting = true;
				//ShowButtonState("HEAT", LOW);
			}
		}
	}	
}

// ======*************************************** SUB FUNCTIONS ****************************************************======

void updateStr(int val)
{
	if (stCurrentLen < 20)
	{
		stCurrent[stCurrentLen] = val;
		stCurrent[stCurrentLen + 1] = '\0';
		stCurrentLen++;
		myGLCD.setColor(0, 255, 0);
		myGLCD.print(stCurrent, LEFT, 224);
	}
	else
	{
		myGLCD.setColor(255, 0, 0);
		myGLCD.print("BUFFER FULL!", CENTER, 192);
		delay(500);
		myGLCD.print("            ", CENTER, 192);
		delay(500);
		myGLCD.print("BUFFER FULL!", CENTER, 192);
		delay(500);
		myGLCD.print("            ", CENTER, 192);
		myGLCD.setColor(0, 255, 0);
	}
}

// Draw a red frame while a button is touched
void waitForIt(int x1, int y1, int x2, int y2)
{
	myGLCD.setColor(255, 0, 0);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
	while (myTouch.dataAvailable())
		myTouch.read();
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
}
	
//====================================================

void gameOver() {
	myGLCD.clrScr();
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setFont(BigFont);
	myGLCD.print("GAME OVER", CENTER, 40);
	myGLCD.print("Score:", 100, 80);
	myGLCD.printNumI(score, 200, 80);
	myGLCD.print("Restarting...", CENTER, 120);
	myGLCD.setFont(SevenSegNumFont);
	myGLCD.printNumI(2, CENTER, 150);
	delay(1000);
	myGLCD.printNumI(1, CENTER, 150);
	delay(1000);
	myGLCD.setColor(114, 198, 206);
	myGLCD.fillRect(0, 0, 319, 239);
	drawBird(30);
	drawGround();
	delay(1000);
}

void restartGame() {
	delay(1000);
	gameOver();
	xP = 319;
	yB = 30;
	fallRate = 0;
	score = 0;
}
// ======*************************************** VISUALISATION OF ALL SCREENS ****************************************************======
// drawHomeScreen - Custom Function
void drawHomeScreen() {
	// Title
	myGLCD.setBackColor(0, 0, 0); // Sets the background color of the area where the text will be printed to black
	myGLCD.setColor(255, 255, 255); // Sets color to white
	myGLCD.setFont(BigFont); // Sets font to big
	myGLCD.print("Temp Regler", CENTER, 10); // Prints the string on the screen

	myGLCD.setColor(255, 0, 0); // Sets color to red
	myGLCD.drawLine(0, 40, 319, 40); // Draws 1 horizontal red line	
	myGLCD.drawLine(160, 40, 160, 90); // Draws vertical red line
	myGLCD.drawLine(0, 98, 319, 98); // Draws 2 horizontal red line	

	myGLCD.setColor(0xcccccc); // Sets color to grey80
	myGLCD.setFont(BigFont);
	myGLCD.print("aktTemp", LEFT, 44);	
	myGLCD.print("tempAUS", RIGHT, 44);	

	myGLCD.setColor(ILI9341_ORANGE);
	myGLCD.setFont(BigFont);
	myGLCD.drawCircle(95, 68, 2);
	myGLCD.print("C", 97, 68);
	myGLCD.drawCircle(275, 68, 2);
	myGLCD.print("C", 277, 68);
	
	myGLCD.setFont(BigFont);
	myGLCD.setBrightness(255);
	myGLCD.setColor(ILI9341_ORANGE);
	myGLCD.printNumF(stopHeatingTemp, 1, 205, 68);

	// Button - Settings
	//myGLCD.setColor(16, 167, 103); // Sets green color
	//myGLCD.fillRoundRect(110, 100, 210, 140); // Draws filled rounded rectangle
	myGLCD.setColor(255, 255, 255); // Sets color to white
	//myGLCD.drawRoundRect(110, 100, 210, 140); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
	myGLCD.setFont(BigFont); // Sets the font to big
	//myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
	myGLCD.print("heizen", 112, 110); // Prints the string

	//Power ON/OFF button in the middle of Screen
	myGLCD.drawBitmap(128, 140, 32, 32, power, 2);
	//setting button in the right corner
	myGLCD.drawBitmap(285, 2, 32, 32, settings, 1);
	//button for changing screen to detailed screen
	myGLCD.drawBitmap(5, 2, 32, 32, left, 1);

	//Ground of screen
	myGLCD.setColor(255, 255, 255); // Sets color to white
	myGLCD.setFont(SmallFont); // Sets the font to small
	myGLCD.print("by SystemsAG", RIGHT, 225); // Prints the string	
	myGLCD.print(__DATE__, LEFT, 225); // This uses the standard ADAFruit small font

}

// draw detailed screen for all possible settings - Custom Function
void drawDetailedScreen() {
	// Title
	myGLCD.setBackColor(0, 0, 0); // Sets the background color of the area where the text will be printed to black
	myGLCD.setColor(255, 255, 255); // Sets color to white
	myGLCD.setFont(BigFont); // Sets font to big
	myGLCD.print("Temp Regler", CENTER, 10); // Prints the string on the screen
	//draw the lines
	myGLCD.setColor(255, 0, 0); // Sets color to red
	myGLCD.drawLine(160, 40, 160, 206); // Draws vertical red line
	myGLCD.drawLine(0, 40, 319, 40); // Draws 1 horizontal red line		
	myGLCD.drawLine(0, 98, 319, 98); // Draws 2 horizontal red line	
	myGLCD.drawLine(0, 156, 319, 156); // Draws 3 horizontal red line		
	myGLCD.drawLine(0, 214, 319, 214); // Draws 4 horizontal red line	

	myGLCD.setColor(0xcccccc); // Sets color to grey80
	myGLCD.setFont(arial_bold);
	myGLCD.print("aktTemp", LEFT, 44);
	myGLCD.print("Zyklus", LEFT, 104);
	myGLCD.print("tempAUS", RIGHT, 44);
	myGLCD.print("deltaEIN", RIGHT, 104);
	myGLCD.print("maxZeit", RIGHT, 164);
	myGLCD.print("aktZeit", LEFT, 164);

	myGLCD.setColor(ILI9341_ORANGE);
	myGLCD.setFont(arial_bold);	
	myGLCD.drawCircle(275, 68, 2);
	myGLCD.print("C", 277, 68);
	
	myGLCD.print("sec", 95, 128);
	myGLCD.drawCircle(275, 128, 2);
	myGLCD.print("C", 277, 128);	
	myGLCD.print("sec", 260, 188);

	myGLCD.setFont(arial_bold);
	myGLCD.setBrightness(255);
	myGLCD.setColor(ILI9341_ORANGE);
	myGLCD.printNumF(stopHeatingTemp, 1, 195, 68);
	myGLCD.printNumF(deltaStartHeat, 1, 195, 128);
	myGLCD.printNumI(maxHeatingStopTimer, 195, 188);
	myGLCD.printNumI(cyclicTimer, 25, 128);

	//======================BUTTONS=====================
	//setting button in the right corner
	myGLCD.drawBitmap(285, 2, 32, 32, right, 1);

	//Ground of screen
	myGLCD.setBackColor(ILI9341_BLACK);
	myGLCD.setColor(255, 255, 255); // Sets color to white
	myGLCD.setFont(SmallFont); // Sets the font to small
	myGLCD.print("by SystemsAG", RIGHT, 225); // Prints the string	
	myGLCD.print(__DATE__, LEFT, 225); // This uses the standard ADAFruit small font

}


// draw setting screen
void drawSettings() {

	// Title
	myGLCD.setBackColor(0, 0, 0); // Sets the background color of the area where the text will be printed to black
	myGLCD.setColor(255, 255, 255); // Sets color to white
	myGLCD.setFont(BigFont); // Sets font to big
	myGLCD.print("Einstellungen", CENTER, 10); // Prints the string on the screen

	//draw the lines
	myGLCD.setColor(255, 0, 0); // Sets color to red
	myGLCD.drawLine(160, 40, 160, 206); // Draws vertical red line
	myGLCD.drawLine(0, 40, 319, 40); // Draws 1 horizontal red line		
	myGLCD.drawLine(0, 98, 319, 98); // Draws 2 horizontal red line	
	myGLCD.drawLine(0, 156, 319, 156); // Draws 3 horizontal red line		
	myGLCD.drawLine(0, 214, 319, 214); // Draws 4 horizontal red line		

	////back arrow in the left high corner
	//myGLCD.setColor(100, 155, 203);
	//myGLCD.fillRoundRect(5, 5, 55, 31);
	//myGLCD.setColor(255, 255, 255);
	//myGLCD.drawRoundRect(5, 5, 55, 31);
	//myGLCD.setFont(BigFont);
	//myGLCD.setBackColor(100, 155, 203);
	//myGLCD.print("<-", 13, 10);
	
	//******************************draw all buttons*************************************
	// Button for max Temp
	myGLCD.setColor(16, 167, 103);
	myGLCD.fillRoundRect(5, 49, 155, 89);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(5, 49, 155, 89);
	myGLCD.setBackColor(16, 167, 103);
	myGLCD.setFont(arial_bold);
	myGLCD.print("Eingabe", 10, 53);
	myGLCD.print("maxTemp", 10, 68);
	//Button for delta Temp
	myGLCD.setColor(16, 167, 103);
	myGLCD.fillRoundRect(5, 107, 155, 147);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(5, 107, 155, 147);
	myGLCD.setBackColor(16, 167, 103);
	myGLCD.setFont(arial_bold);
	myGLCD.print("Eingabe", 10, 111);
	myGLCD.print("deltaTemp", 10, 126);
	// Button for max Time
	myGLCD.setColor(16, 167, 103);
	myGLCD.fillRoundRect(5, 156+9, 155, 196+9);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(5, 156+9, 155, 196+9);
	myGLCD.setBackColor(16, 167, 103);
	myGLCD.setFont(arial_bold);
	myGLCD.print("Eingabe", 10, 156+13);
	myGLCD.print("maxZeit", 10, 156+13);

	// Button - Birduino
	myGLCD.setColor(16, 167, 103);
	myGLCD.fillRoundRect(165, 49, 315, 89);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(165, 49, 315, 89);
	myGLCD.setFont(arial_bold);
	myGLCD.setBackColor(16, 167, 103);
	myGLCD.print("BIRDUINO GAME", 170, 53);

	//button for changing screen to detailed screen
	myGLCD.drawBitmap(5, 2, 32, 32, left, 1);
	
	//Ground of screen
	myGLCD.setBackColor(ILI9341_BLACK);
	myGLCD.setColor(255, 255, 255); // Sets color to white
	myGLCD.setFont(SmallFont); // Sets the font to small
	myGLCD.print("by SystemsAG", RIGHT, 225); // Prints the string	
	myGLCD.print(__DATE__, LEFT, 225); // This uses the standard ADAFruit small font
}


//================ Draw BUTTON-SCREEN =======================
void drawButtons()
{
	myGLCD.InitLCD();
	myGLCD.clrScr();

	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);

	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255); // Sets the background color of the area where the text will be printed to black


	// Draw the upper row of buttons
	for (x = 0; x < 5; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(10 + (x * 60), 10, 60 + (x * 60), 60);
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(10 + (x * 60), 10, 60 + (x * 60), 60);
		myGLCD.printNumI(x + 1, 27 + (x * 60), 27);
	}
	// Draw the center row of buttons
	for (x = 0; x < 5; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(10 + (x * 60), 70, 60 + (x * 60), 120);
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(10 + (x * 60), 70, 60 + (x * 60), 120);
		if (x < 4)
			myGLCD.printNumI(x + 6, 27 + (x * 60), 87);
	}
	myGLCD.print("0", 267, 87);
	// Draw the lower row of buttons
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect(10, 130, 150, 180);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(10, 130, 150, 180);
	myGLCD.print("Clear", 40, 147);
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect(160, 130, 300, 180);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(160, 130, 300, 180);
	myGLCD.print("Enter", 190, 147);
	myGLCD.setBackColor(0, 0, 0);
}
// Highlights the button when pressed
void drawFrame(int x1, int y1, int x2, int y2) {
	myGLCD.setColor(255, 0, 0);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
	while (myTouch.dataAvailable())
		myTouch.read();
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
}
// Highlights the button when pressed
void drawCircleFrame(int x1, int y1, int radius) {
	myGLCD.setColor(255, 0, 0);
	myGLCD.drawCircle(x1, y1, radius);
	while (myTouch.dataAvailable())
		myTouch.read();
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawCircle(x1, y1, radius);
}

//====================================================
void drawGround() {
	myGLCD.setColor(221, 216, 148);
	myGLCD.fillRect(0, 215, 319, 239);
	myGLCD.setColor(47, 175, 68);
	myGLCD.fillRect(0, 205, 319, 214);
	myGLCD.setColor(0, 0, 0);
	myGLCD.setBackColor(221, 216, 148);
	myGLCD.setFont(BigFont);
	myGLCD.print("Score:", 5, 220);
	myGLCD.setFont(SmallFont);
	myGLCD.print("SystemsAG", 140, 220);
}
void drawPilars(int x, int y) {

	if (x >= 270) {
		myGLCD.setColor(0, 200, 20);
		myGLCD.fillRect(318, 0, x, y - 1);
		myGLCD.setColor(0, 0, 0);
		myGLCD.drawRect(319, 0, x - 1, y);

		myGLCD.setColor(0, 200, 20);
		myGLCD.fillRect(318, y + 81, x, 203);
		myGLCD.setColor(0, 0, 0);
		myGLCD.drawRect(319, y + 80, x - 1, 204);
	}
	else if (x <= 268) {
		myGLCD.setColor(114, 198, 206);
		myGLCD.fillRect(x + 51, 0, x + 53, y);
		myGLCD.setColor(0, 200, 20);
		myGLCD.fillRect(x + 49, 1, x + 1, y - 1);
		myGLCD.setColor(0, 0, 0);
		myGLCD.drawRect(x + 50, 0, x, y);
		myGLCD.setColor(114, 198, 206);
		myGLCD.fillRect(x - 1, 0, x - 3, y);

		myGLCD.setColor(114, 198, 206);
		myGLCD.fillRect(x + 51, y + 80, x + 53, 204);
		myGLCD.setColor(0, 200, 20);
		myGLCD.fillRect(x + 49, y + 81, x + 1, 203);
		myGLCD.setColor(0, 0, 0);
		myGLCD.drawRect(x + 50, y + 80, x, 204);
		myGLCD.setColor(114, 198, 206);
		myGLCD.fillRect(x - 1, y + 80, x - 3, 204);
	}
	myGLCD.setColor(0, 0, 0);
	myGLCD.setBackColor(221, 216, 148);
	myGLCD.setFont(BigFont);
	myGLCD.printNumI(score, 100, 220);
}
//====================================================
void drawBird(int y) {
	if (y <= 219) {
		myGLCD.drawBitmap(50, y, 35, 30, bird01);
		myGLCD.setColor(114, 198, 206);
		myGLCD.fillRoundRect(50, y, 85, y - 6);
		myGLCD.fillRoundRect(50, y + 30, 85, y + 36);
	}
	else if (y >= 200) {
		myGLCD.drawBitmap(50, 200, 35, 30, bird01);
		myGLCD.setColor(114, 198, 206);
		myGLCD.fillRoundRect(50, 200, 85, 200 - 6);
		myGLCD.fillRoundRect(50, 200 + 30, 85, 200 + 36);
	}
}
//====================================================
//===== getDistance - Custom Function


