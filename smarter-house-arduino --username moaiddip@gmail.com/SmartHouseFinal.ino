﻿// Device Group proudly presents its part of the work on the
// ............SMARTer HOUSE 2014............
// |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
// |@authors: Michal Sitarczuk, Ke Jia, Kevin Hildebrand, Martin Vilhemsson, Shen Wang, Ariful Islam
// |--------------------------------------------------------------------------------|
// |                            VERSION CONTROL HISTORY
// |--------------------------------------------------------------------------------|
// |  VERSION    |              AUTHORs            |    DATE    |MAJOR CHANGE/UPDATE
// |--------------------------------------------------------------------------------|
// |  0.1                Michal Sitarczuk            03/10/2014    initialization
// |--------------------------------------------------------------------------------|
// |  0.2                Martin Wilhelmson           10/10/2014    MUXwrite edited
// |--------------------------------------------------------------------------------|
// |  0.3                Michal Sitarczuk            11/10/2014    Check methods, Arduino board change
// |--------------------------------------------------------------------------------|
// |  0.4				Michal Sitarczuk			02/11/2014	   Code added to Subversion repository
// |--------------------------------------------------------------------------------|
// |  0.5				Michal Sitarczuk			02/11/2014		Versioning from now on at https://smarter-house-arduino.googlecode.com/svn/trunk/smarter-house-arduino
// |--------------------------------------------------------------------------------|
// |
// |________________________________________________________________________________|

// DIGITAL INPUT PINS
#define size1 7
int FireAlarmPin = 2;
int DoorIsOpenedPin = 3;
int WaterLeakagePin = 4;
int StoveOnPin = 5;
int WindowIsOpenedPin = 6;
int ElectricityCutPin = 7;
int TempOutPin = 9;
int digitalInputPins[size1] = { FireAlarmPin, DoorIsOpenedPin, WaterLeakagePin, StoveOnPin, WindowIsOpenedPin, ElectricityCutPin, TempOutPin };
// ANALOG INPUT PINS
#define size2 4
int ElConsumptionPin = 14; //=A0
int TempInsidePin = 15; //=A1
int TempInsideWindPin = 16; //=A2
int LDRpin = 17; //=A3
int analogInputPins[size2] = { ElConsumptionPin, TempInsidePin, TempInsideWindPin, LDRpin };
// DIGITAL PWM OUTPUT PINS
int FanPin = 10;
// DIGITAL OUTPUT PINS (MUX)
#define size3 4
int MUX12 = 12;
int MUX13 = 13;
int MUX11 = 11;
int MUX8 = 8;
int MUXpins[size3] = { MUX12, MUX13, MUX11, MUX8 };
// ALL HOUSE FUNCTIONS
#define size4 23
String allFunctions[size4] = { "fa","do","wl","st","wo","ec","tmpout","elcon","tmpin","tmproof","ldr","buzz","t2","lghtin","aled","heatroof","heatin","t1","lghout","autoac","autolo","sa","autoli"};
// String input
String inMsg = "";	// saves messages from local server until gets "!" which means end of message
char inChar;		// incoming character that is saved in inMsg
// booleans			// moslty to help in logic or to keep track of virtual functions (not readeble from hardware); setting
					// initial conditions
boolean SecurityAlarm = false;
boolean FirstRun = true;
// integers			// (holding values from pins)
int WindowIsOpened;
int FireAlarm;
int DoorIsOpen;
int WaterLeakage;
int StoveOn;
int ElectricityCut;
double TempOut;

void setup()
{
	Serial.begin(115200);
	inMsg.reserve(100);
	//Settings pinMode
	for (int i = 0; i<size1; i++){
		pinMode(digitalInputPins[i], INPUT);
	}
	for (int i = 0; i<size2; i++){
		pinMode(analogInputPins[i], INPUT);
	}
	for (int i = 0; i<size3; i++){
		pinMode(MUXpins[i], OUTPUT);
	}
}

//*Changed the order of the MUXwrite-method so it corresponds to the MUX-table in lab4-PDF.
void MUXwrite(int first, int second, int third, int fourth){
	//*Added delays in the MUXwrite-method; why? 
	//I get these very strange errors where things 
	//simply stops working. if more than one property is turned on. 
	//Talked to Haydar who advised me to do so, and it worked. 
	//If you dont get this;great, remove it.
	//Removed delays as slow down the loop (which we base on) -mms
	delay(20);
	digitalWrite(MUX12, first);
	digitalWrite(MUX13, second);
	digitalWrite(MUX11, third);
	digitalWrite(MUX8, fourth);
	delay(20);
}

void loop()
{
	if (FirstRun){\
		MUXwrite(1, 1, 0, 1);//TURNS OFF heating element
		MUXwrite(1, 1, 0, 0);//TURNS OFF heating element wind
		MUXwrite(0, 0, 1, 0);
		MUXwrite(0, 1, 1, 1);
		CheckAll();
		FirstRun = false;
	}
	SerialEvent();
	if (SecurityAlarm){
		if (digitalRead(DoorIsOpenedPin) == LOW || digitalRead(WindowIsOpenedPin) == HIGH){
			MUXwrite(1, 0, 0, 0);
		}
	}
	if (!SecurityAlarm){
		MUXwrite(0, 0, 0, 0);
	}
}

void SerialEvent(){
	while (Serial.available()){
		inChar = (char)Serial.read();
		if (inChar != '!'){
			inMsg += inChar;
		}
		else if (inChar == '!'){
			if (inMsg.endsWith("test")){
				TestRequest(inMsg);
			}
			else if (inMsg.endsWith("chk")){
				CheckRequest(inMsg);
			}
			else{
				MsgHandler(inMsg);
			}
			inMsg = "";
			break;
		}
	}
}

void TestRequest(String command){
	Serial.println("Test requested");
}
void CheckRequest(String command){
	Serial.println("Check requested");
	CheckAll();
}
void MsgHandler(String command){
	if (command.equals("sa_on")){
		SecurityAlarm = true;
	}
	else if (command.equals("sa_off")){
		SecurityAlarm = false;
	}
	else {
		Serial.println("Syntax error. \t\"" + command + "\"\t Unknown command");
	}
}
void CheckAll(){
	for (int i = 0; i < size4; i++){
		CheckStatus(allFunctions[i]);
	}
}

void CheckStatus(String what){
	Serial.println(what);
	if (what.equals("fa")){
		FireAlarm = digitalRead(FireAlarmPin);
		if (FireAlarm == HIGH){
			Serial.println("fa_on!");
		}
		else
			Serial.println("fa_off!");
	}
	else if (what.equals("do")){
		DoorIsOpen = digitalRead(DoorIsOpenedPin);
		if (DoorIsOpen == LOW){
			Serial.println("do_on!");
		}
		else
			Serial.println("do_off!");
	}
	else if (what.equals("wl")){
		WaterLeakage = digitalRead(WaterLeakagePin);
		if (WaterLeakage == HIGH){
			Serial.println("wl_on!");
		}
		else
			Serial.println("wl_off!");

	}
	else if (what.equals("st")){
		StoveOn = digitalRead(StoveOnPin);
		if (StoveOn == HIGH){
			Serial.println("st_on!");
		}
		else
			Serial.println("st_off!");
	}
	else if (what.equals("wo")){
		WindowIsOpened = digitalRead(WindowIsOpenedPin);
		if (WindowIsOpened == HIGH){
			Serial.println("wo_on!");
		}
		else
			Serial.println("wo_off!");
	}
	else if (what.equals("ec")){
		ElectricityCut = digitalRead(ElectricityCutPin);
		if (ElectricityCut == HIGH){
			Serial.println("ec_on!");
		}
		else
			Serial.println("ec_off!");
	}
	else if (what.equals("tmpout")){
		TempOut = analogRead(TempOutPin) * 5 / 1024.0; TempOut = TempOut - 0.5; TempOut = TempOut / 0.01;
		Serial.println("tmpout_"+(String)TempOut);
	}
	else
		Serial.println("Unknown or empty command");
}