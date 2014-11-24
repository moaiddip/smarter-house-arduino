// Device Group proudly presents its part of the work on the
// ............SMARTer HOUSE 2014............
// |--------------------------------------------------------------------------------|
// |@authors: Michal Sitarczuk, Ke Jia, Kevin Hildebrand, Martin Vilhemsson, Shen Wang, Ariful Islam
// |--------------------------------------------------------------------------------|
// | VERSION CONTROL HISTORY
// |--------------------------------------------------------------------------------|
// | VERSION | AUTHORs | DATE |MAJOR CHANGE/UPDATE
// |--------------------------------------------------------------------------------|
// | 0.1 Michal Sitarczuk 03/10/2014 initialization
// |--------------------------------------------------------------------------------|
// | 0.2 Martin Wilhelmson 10/10/2014 MUXwrite edited
// |--------------------------------------------------------------------------------|
// | 0.3 Michal Sitarczuk, Ke Jia 11/10/2014 Check methods, Arduino board change
// |--------------------------------------------------------------------------------|
// | 0.4 Michal Sitarczuk 02/11/2014 Coded added to Subversion repository
// |--------------------------------------------------------------------------------|
// | FROM NOW ON CODE REVISION IN REPOSITORY https://smarter-house-arduino.googlecode.com/svn/trunk/smarter-house-arduino
// |________________________________________________________________________________|
//#include <MemoryFree.h>;
//#include <pgmStrToRAM.h>;
#include <avr\pgmspace.h>;
// DIGITAL INPUT PINS
//#define size1 7
byte PROGMEM FireAlarmPin = 2;
byte PROGMEM DoorIsOpenedPin = 3;
byte PROGMEM WaterLeakagePin = 4;
byte PROGMEM StoveOnPin = 5;
byte PROGMEM WindowIsOpenedPin = 6;
byte PROGMEM ElectricityCutPin = 7;
byte PROGMEM TempOutPin = 9;
//int digitalInputPins[size1] = { FireAlarmPin, DoorIsOpenedPin, WaterLeakagePin, StoveOnPin, WindowIsOpenedPin, ElectricityCutPin, TempOutPin };
// ANALOG INPUT PINS
//#define size2 4
byte PROGMEM ElConsumptionPin = 14; //=A0
byte PROGMEM TempInsidePin = 15; //=A1
byte PROGMEM TmpRoofPin = 16; //=A2
byte PROGMEM LDRpin = 17; //=A3
//int analogInputPins[size2] = { ElConsumptionPin, TempInsidePin, TmpRoofPin, LDRpin };
// DIGITAL PWM OUTPUT PINS
byte PROGMEM FanPin = 10;
// DIGITAL OUTPUT PINS (MUX)
//#define size3 4
byte PROGMEM MUX12 = 12;
byte PROGMEM MUX13 = 13;
byte PROGMEM MUX11 = 11;
byte PROGMEM MUX8 = 8;
//int MUXpins[size3] = { MUX12, MUX13, MUX11, MUX8 };
// ALL HOUSE FUNCTIONS
#define size4 19 //24
static const String allFunctions[size4] = { "fa", "do", "wl", "st", "wo", "ec", "tmpout", "elcon", "tmpin", "tmproof", "li", "heatroof", "heatin", "lo", "autoac", "autolo", "sa", "autoli", "fan" }; //, "buzz", "t2", "aled", "t1", "ldr"};//needless
// String input
String inMsg = ""; // saves messages from local server until gets "!" which means end of message
char inChar; // incoming character that is saved in inMsg
// booleans // moslty to help in logic or to keep track of virtual functions (not readeble from hardware); setting
// initial conditions
boolean SecurityAlarm = false;
boolean saTrig, faTrig, wlTrig = false;//keep track if corresponding alarm is triggered
boolean autoli, liON, autolo, loON, autoAC, heatinON, heatroofON = false; //keeps an eye on devices/functions status
// integers // (holding values from pins)
boolean WindowIsOpened, WindowCurrentState = 0;
boolean FireAlarm;
boolean DoorIsOpen, DoorCurrentState = 0;
boolean WaterLeakage;
boolean StoveOn, StoveCurrentState = 0;
boolean ElectricityCut, elcutTrig = 0;
byte LDRsensor;
byte FanSpeed = 0;
byte autoACtmp = 0;
unsigned int alarmReportTimer;
unsigned int tempReportTimer;
//Outside temperature digital sensor/////////////////////////////////////////////////////
float  DutyCycle = 0;                               //DutyCycle that need to calculate  /
unsigned long  SquareWaveHighTime = 0;				//High time for the square wave     /
unsigned long  SquareWaveLowTime = 0;				//Low time for the square wave      /
unsigned long TempOut = 0;						//Calculated temperature using dutycycle/
String TempOutS = "";//																	/
/////////////////////////////////////////////////////////////////////////////////////////
//Inside temperature analog sensor////////////////////////////////////////////////////////
byte TmpIn1, TmpIn2, TmpIn3, TmpIn = 0;//holding readings to finally get avarage in TmpIn/
//////////////////////////////////////////////////////////////////////////////////////////
//Roof temperature analog sensor//////////////////////////////////////////////////////////////////
byte TmpRoof1, TmpRoof2, TmpRoof3, TmpRoof = 0;//holding readings to finally get avarage in TmpIn/
//////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
	Serial.begin(38400);
	inMsg.reserve(10);
	TempOutS.reserve(2);
	//Settings pinMode
	pinMode(FanPin, OUTPUT);
	/*for (int i = 0; i < size1; i++){
		pinMode(digitalInputPins[i], INPUT);
		}*/
	pinMode(FireAlarmPin, INPUT);
	pinMode(DoorIsOpenedPin, INPUT);
	pinMode(WaterLeakagePin, INPUT);
	pinMode(StoveOnPin, INPUT);
	pinMode(WindowIsOpenedPin, INPUT);
	pinMode(ElectricityCutPin, INPUT);
	pinMode(TempOutPin, INPUT);
	/*for (int i = 0; i < size2; i++){
		pinMode(analogInputPins[i], INPUT);
		}*/
	pinMode(ElConsumptionPin, INPUT);
	pinMode(TempInsidePin, INPUT);
	pinMode(TmpRoofPin, INPUT);
	pinMode(LDRpin, INPUT);
	/*for (int i = 0; i < size3; i++){
		pinMode(MUXpins[i], OUTPUT);
		}*/
	pinMode(MUX12, OUTPUT);
	pinMode(MUX13, OUTPUT);
	pinMode(MUX11, OUTPUT);
	pinMode(MUX8, OUTPUT);
	//FIRST RUN INITIALIZATION OF DEVICES and VARIABLES
	MUXwrite(1, 1, 0, 1);//Turns OFF heating element
	MUXwrite(1, 1, 0, 0);//Turns OFF heating element wind
	MUXwrite(1, 0, 1, 0);//Turns OFF inside light
	MUXwrite(1, 1, 1, 1);//Turns OFF outside light
	MUXwrite(0, 0, 0, 0);//Turns buzzer off
	MUXwrite(1, 1, 1, 0); //Timer 1 is set to off
	MUXwrite(1, 0, 0, 1);//Timer2 is used in a loop to prevent random MUX pin values
	MUXwrite(1, 0, 1, 1); //Alarm LED turned off
	calcTempOut(); //initialization of tmpout
	calcTempIn(); //initialization of tmpin
	calcTempRoof();
	CheckAll();
	alarmReportTimer = millis();
	tempReportTimer = millis();
}

//*Changed the order of the MUXwrite-method so it corresponds to the MUX-table in lab4-PDF.
void MUXwrite(int first, int second, int third, int fourth){
	//It works nice the way it is now; if you have troubles with it, change 20 to 50 inside delay,
	//but do not add more delay(s) pls -mms
	delay(100);
	digitalWrite(MUX12, first);
	digitalWrite(MUX13, second);
	digitalWrite(MUX11, third);
	digitalWrite(MUX8, fourth);
	//Serial.println(F((String)first + " " + (String)second + " " + (String)third + " " + (String)fourth);
	delay(100);
}

void loop()
{
	//Keeping MUX values stable by setting T2 to off every loop iteration
	//digitalWrite(MUX12, 1);
	//digitalWrite(MUX13, 0);
	//digitalWrite(MUX11, 0);
	//digitalWrite(MUX8, 1);
	//-------------------------------------------------------------------
	SerialEvent();
	UpdateDevicesStatus();
	if (millis() - alarmReportTimer > 5000){
		alarmReportTimer = millis();
		if (wlTrig){
			Serial.println(F("wl_alarm!"));
		}
		if (faTrig){
			Serial.println(F("fa_alarm!"));
		}
		if (saTrig){
			Serial.println(F("sa_alarm!"));
		}
		if (elcutTrig){
			Serial.println(F("ec_alarm!"));
		}
	}
	if (millis() - tempReportTimer > 12000){
		tempReportTimer = millis();
		calcTempOut();
		calcTempIn();
		calcTempRoof();
		Serial.println(CheckStatus("tmpout"));
		Serial.println(CheckStatus("tmpin"));
		Serial.println(CheckStatus("tmproof"));
		if (autoAC){
			autoAChandler(autoACtmp);
		}
		//Serial.println(freeMemory(), DEC);//debug msg
	}
	if (SecurityAlarm){
		if (digitalRead(DoorIsOpenedPin) == LOW || digitalRead(WindowIsOpenedPin) == HIGH){
			MUXwrite(1, 0, 0, 0);
			saTrig = true;
		}
	}
	if (!faTrig && !wlTrig && !saTrig){
		MUXwrite(0, 0, 0, 0);
	}
	StoveCurrentState = digitalRead(StoveOnPin);
	if (StoveOn != StoveCurrentState){
		if (StoveCurrentState == HIGH){
			Serial.println(F("st_on!"));
		}
		else if (StoveCurrentState == LOW){
			Serial.println(F("st_off!"));
		}
	}
	WindowCurrentState = digitalRead(WindowIsOpenedPin);
	if (WindowIsOpened != WindowCurrentState){
		if (WindowCurrentState == HIGH){
			Serial.println(F("wo_on!"));
		}
		else if (WindowCurrentState == LOW){
			Serial.println(F("wo_off!"));
		}
	}
	DoorCurrentState = digitalRead(DoorIsOpenedPin);
	if (DoorIsOpen != DoorCurrentState){
		if (DoorCurrentState == HIGH){
			Serial.println(F("do_on!"));
		}
		else if (DoorCurrentState == LOW){
			Serial.println(F("do_off!"));
		}
	}
	if (autolo == true){
		LDRsensor = analogRead(LDRpin);
		if (LDRsensor <= 300){
			MUXwrite(0, 1, 1, 1);
			loON = true;
		}
		else if (LDRsensor > 300){
			MUXwrite(1, 1, 1, 1);
			loON = false;
		}
	}
	if (autoli == true){
		LDRsensor = analogRead(LDRpin);
		if (LDRsensor <= 300){
			MUXwrite(0, 0, 1, 0);
			liON = true;
		}
		else if (LDRsensor > 300){
			MUXwrite(1, 0, 1, 0);
			liON = false;
		}
	}
}
void SerialEvent(){
	while (Serial.available()){
		inChar = (char)Serial.read();
		if (inChar != '!'){
			inMsg += inChar;
		}
		else if (inChar == '!'){
			if (inMsg.endsWith("chk")){
				CheckRequest(inMsg);
			}
			else{
				MsgHandler(inMsg);
			}
			if (!inMsg.endsWith("chk")){
				CheckRequest(inMsg);
			}
			inMsg = "";
			break;
		}
	}
}

void CheckRequest(String command){
	//Serial.println(F("Check requested"); //debug msg
	if (command.startsWith("all")){
		CheckAll();
	}
	else if (command.startsWith("fan")){
		Serial.println(CheckStatus("fan"));
	}
	else if (command.startsWith("do")){
		//Serial.println(F(command);//debug msg
		Serial.println(CheckStatus("do"));
	}
	else if (command.startsWith("wl")){
		Serial.println(CheckStatus("wl"));
	}
	else if (command.startsWith("st")){
		Serial.println(CheckStatus("st"));
	}
	else if (command.startsWith("wo")){
		Serial.println(CheckStatus("wo"));
	}
	else if (command.startsWith("ec")){
		Serial.println(CheckStatus("ec"));
	}
	else if (command.startsWith("tmpout")){
		Serial.println(CheckStatus("tmpout"));
	}
	else if (command.startsWith("elcon")){
		Serial.println(CheckStatus("elcon"));
	}
	else if (command.startsWith("tmpin")){
		Serial.println(CheckStatus("tmpin"));
	}
	else if (command.startsWith("tmproof")){
		Serial.println(CheckStatus("tmproof"));
	}
	else if (command.startsWith("li")){
		Serial.println(CheckStatus("li"));
	}
	else if (command.startsWith("heatroof")){
		Serial.println(CheckStatus("heatroof"));
	}
	else if (command.startsWith("heatin")){
		Serial.println(CheckStatus("heatin"));
	}
	else if (command.startsWith("lo")){
		Serial.println(CheckStatus("lo"));
	}
	else if (command.startsWith("autoac")){
		Serial.println(CheckStatus("autoac"));
	}
	else if (command.startsWith("autolo")){
		Serial.println(CheckStatus("autolo"));
	}
	else if (command.startsWith("sa")){
		Serial.println(CheckStatus("sa"));
	}
	else if (command.startsWith("autoli")){
		Serial.println(CheckStatus("autoli"));
	}
	else if (command.startsWith("fa")){
		Serial.println(CheckStatus("fa"));
	}
	else if (command.startsWith("alarms")){
		Serial.println(CheckStatus("wl"));
		Serial.println(CheckStatus("fa"));
		Serial.println(CheckStatus("sa"));
	}
	/*else if (command.startsWith("ldr")){
		Serial.println(CheckStatus("ldr"));
		}
		else if (command.startsWith("buzz")){
		Serial.println(CheckStatus("buzz"));
		}
		else if (command.startsWith("t2")){
		Serial.println(CheckStatus("t2"));
		}
		else if (command.startsWith("aled")){
		Serial.println(CheckStatus("aled"));
		}
		else if (command.startsWith("t1")){
		Serial.println(CheckStatus("t1"));
		}*/
	else{
		Serial.println(F("error_Invalid command; in CheckRequest()!"));
	}
}
void MsgHandler(String command){
	if (command.equals("sa_on")){
		if (digitalRead(DoorIsOpenedPin) == LOW || digitalRead(WindowIsOpenedPin) == HIGH || digitalRead(StoveOnPin) == HIGH) {
			Serial.println(F("error_Door and windows must be closed to activate!"));
		}
		else {
			SecurityAlarm = true;
			MUXwrite(0, 1, 1, 0);
		}
	}
	else if (command.equals("sa_off")){
		SecurityAlarm = false;
		MUXwrite(1, 1, 1, 0);
		saTrig = false;
	}
	else if (command.equals("wl_off")){
		wlTrig = false;
	}
	else if (command.equals("fa_off")){
		faTrig = false;
	}
	else if (command.equals("alarms_off")){
		SecurityAlarm = false;
		MUXwrite(1, 1, 1, 0);
		saTrig = false;
		wlTrig = false;
		faTrig = false;
	}
	else if (command.equals("li_on")){
		if (autoli){
			Serial.println(F("error_Auto Light In option is ON!"));
		}
		else{
			MUXwrite(0, 0, 1, 0);
			liON = true;
		}
	}
	else if (command.equals("li_off")){
		if (autoli){
			Serial.println(F("error_Auto Light In option is ON!"));
		}
		else{
			MUXwrite(1, 0, 1, 0);
			liON = false;
		}
	}
	else if (command.equals("lo_on")){
		if (autolo){
			Serial.println(F("error_Auto Light Out option is ON!"));
		}
		else{
			MUXwrite(0, 1, 1, 1);
			loON = true;
		}
	}
	else if (command.equals("lo_off")){
		if (autolo){
			Serial.println(F("error_Auto Light Out option is ON!"));
		}
		else{
			MUXwrite(1, 1, 1, 1);
			loON = false;
		}
	}
	else if (command.equals("autolo_on")){
		autolo = true;
	}
	else if (command.equals("autolo_off"))
	{
		autolo = false;
		if (loON){
			MUXwrite(0, 1, 1, 1);
		}
		else if (!loON){
			MUXwrite(1, 1, 1, 1);
		}
	}
	else if (command.equals("autoli_on")){
		autoli = true;
	}
	else if (command.equals("autoli_off")){
		autoli = false;
		if (liON){
			MUXwrite(0, 0, 1, 0);
		}
		else if (!liON){
			MUXwrite(1, 0, 1, 0);
		}
	}
	else if (command.startsWith("fan")){
		if (!autoAC){
			if (command.endsWith("1")){
				analogWrite(FanPin, 1023);
				delay(100);
				analogWrite(FanPin, 341);
				FanSpeed = 1;
			}
			else if (command.endsWith("2")){
				analogWrite(FanPin, 1023);
				delay(100);
				analogWrite(FanPin, 682);
				FanSpeed = 2;
			}
			else if (command.endsWith("3")){
				analogWrite(FanPin, 1023);
				FanSpeed = 3;
			}
			else if (command.endsWith("off")){
				analogWrite(FanPin, 0);
				FanSpeed = 0;
			}
			else{
				Serial.println(F("error_Unknown command or syntax error. To set fan speed use i.e. fan_1! (1 min - 3 max)!"));
			}
		}
		else{
			Serial.println(F("error_To manually controll the fan, turn OFF AutoAC first!"));
		}
	}
	else if (command.equals("heatin_on")){
		if (autoAC){
			Serial.println(F("error_Auto AC option is ON!"));
		}
		else{
			MUXwrite(0, 1, 0, 1);
			heatinON = true;
		}
	}
	else if (command.equals("heatin_off")){
		if (autoAC){
			Serial.println(F("error_Auto AC option is ON!"));
		}
		else{
			MUXwrite(1, 1, 0, 1);
			heatinON = false;
		}
	}
	else if (command.equals("heatroof_on")){
		if (autoAC){
			Serial.println(F("error_Auto AC option is ON!"));
		}
		else{
			MUXwrite(0, 1, 0, 0);
			heatroofON = true;
		}
	}
	else if (command.equals("heatroof_off")){
		if (autoAC){
			Serial.println(F("error_Auto AC option is ON!"));
		}
		else{
			MUXwrite(1, 1, 0, 0);
			heatroofON = false;
		}
	}
	else if (command.startsWith("autoac")){
		if (command.endsWith("off")){
			MUXwrite(1, 1, 0, 0); //heatroof off
			heatroofON = false;
			MUXwrite(1, 1, 0, 1); //heatin off
			heatinON = false;
			analogWrite(FanPin, 0); //turns off fan
			FanSpeed = 0;
			autoACtmp = 0;
			autoAC = false;
		}
		else{
			byte tempcommandlength = 0;//command.length;
			if (tempcommandlength < 10){
				char one = command.charAt(7);
				char two = command.charAt(8);
				String oneS, twoS;
				oneS.reserve(3);
				twoS.reserve(3);
				oneS = (String)one;
				twoS = (String)two;
				byte oneI = oneS.toInt() * 10;
				byte twoI = twoS.toInt();
				autoACtmp = oneI + twoI;
				//Serial.println(autoAC);//debug msg
				if (autoACtmp < 10 || autoACtmp>40){
					autoACtmp = 0;
					Serial.println(F("error_Use \"autoac_XX\" where XX is valid integer for target temperature!"));
				}
				else if (autoACtmp > 9 && autoACtmp < 41){
					//Serial.println("error_Success. AutoAC turned on. Desired temperature: " + (String)autoACtmp + "!");//debug msg
					autoAC = true;
				}
				else{
					Serial.println(F("error_Error parsing to integer!"));
				}
			}
			else
				Serial.println(F("error_String too long. Temperature values range: 10 - 40!"));
		}
	}
	else {
		Serial.println(F("error_Syntax error. Unknown command (in MsgHandler())!"));
	}
}
void CheckAll(){
	Serial.println(F("autochkstart!"));
	for (byte i = 0; i < size4; i++){
		Serial.println(CheckStatus(allFunctions[i]));
	}
	Serial.println(F("eol!"));
}
void UpdateDevicesStatus(){
	for (byte i = 0; i < size4; i++){
		CheckStatus(allFunctions[i]);
	}
}


String CheckStatus(String what){
	//Serial.println(F(what); //debug msg
	if (what.equals("fa")){
		FireAlarm = digitalRead(FireAlarmPin);
		if (FireAlarm == HIGH){
			//Serial.println(F("fa_on!"); //debug msg
			MUXwrite(1, 0, 0, 0);
			faTrig = true;
			return "fa_alarm!";
		}
		else if (FireAlarm == LOW){
			//Serial.println(F("fa_off!");
			return "fa_off!";
		}
	}
	else if (what.equals("do")){
		DoorIsOpen = digitalRead(DoorIsOpenedPin);
		if (DoorIsOpen == LOW){
			//Serial.println(F("do_on!");
			return "do_on!";
		}
		else if (DoorIsOpen == HIGH){
			//Serial.println(F("do_off!");
			return "do_off!";
		}
	}
	else if (what.equals("wl")){
		WaterLeakage = digitalRead(WaterLeakagePin);
		if (WaterLeakage == HIGH){
			//Serial.println(F("wl_on!");
			MUXwrite(1, 0, 0, 0);
			wlTrig = true;
			return "wl_on!";
		}
		else if (WaterLeakage == LOW){
			//Serial.println(F("wl_off!");
			return "wl_off!";
		}

	}
	else if (what.equals("st")){
		StoveOn = digitalRead(StoveOnPin);
		if (StoveOn == HIGH){
			//Serial.println(F("st_on!");
			return "st_on!";
		}
		else if (StoveOn == LOW){
			//Serial.println(F("st_off!");
			return "st_off!";
		}
	}
	else if (what.equals("wo")){
		WindowIsOpened = digitalRead(WindowIsOpenedPin);
		if (WindowIsOpened == HIGH){
			//Serial.println(F("wo_on!");
			return "wo_on!";
		}
		else if (WindowIsOpened == LOW){
			//Serial.println(F("wo_off!");
			return "wo_off!";
		}
	}
	else if (what.equals("ec")){
		ElectricityCut = digitalRead(ElectricityCutPin);
		if (ElectricityCut == HIGH){
			//Serial.println(F("ec_on!");
			elcutTrig = false;
			return "ec_off!";
		}
		else if (ElectricityCut == LOW){
			elcutTrig = true;
			return "ec_on!";
			//Serial.println(F("ec_off!");
		}
	}
	else if (what.equals("li")){
		if (liON){
			return "li_on!";
		}
		else if (!liON){
			return "li_off!";
		}
	}
	else if (what.equals("lo")){
		if (loON){
			return "lo_on!";
		}
		else if (!loON){
			return "lo_off!";
		}
	}
	else if (what.equals("sa")){
		if (SecurityAlarm){
			return "sa_on!";
		}
		else if (!SecurityAlarm){
			return "sa_off!";
		}
	}
	else if (what.equals("tmpout")){
		//return "tmpout_" + (String)TempOut + "!";
		return "tmpout_" + TempOutS + "!";
	}
	else if (what.equals("autolo")){
		if (autolo){
			return"autolo_on!";
		}
		else if (!autolo){
			return"autolo_off!";
		}
	}
	else if (what.equals("autoli")){
		if (autoli){
			return "autoli_on!";
		}
		else if (!autoli){
			return "autoli_off!";
		}
	}
	else if (what.equals("fan")){
		if (FanSpeed != 0){
			return "fan_" + (String)FanSpeed + "!";
		}
		else if (FanSpeed == 0){
			return "fan_off!";
		}
	}
	else if (what.equals("elcon")){
		return "THIS CHECK HAS TO BE DEVELOPED elcon";
	}
	else if (what.equals("tmpin")){
		return "tmpin_" + (String)TmpIn + "!";
	}
	else if (what.equals("tmproof")){
		return "tmproof_" + (String)TmpRoof + "!";
	}
	else if (what.equals("heatin")){
		if (heatinON){
			return "heatin_on!";
		}
		else if (!heatinON){
			return "heatin_off!";
		}
	}
	else if (what.equals("heatroof")){
		if (heatroofON){
			return "heatroof_on!";
		}
		else if (!heatroofON){
			return "heatroof_off!";
		}
	}
	else if (what.equals("autoac")){
		if (autoACtmp == 0){
			return "autoac_off!";
		}
		else{
			return "autoac_" + (String)autoACtmp + "!";
		}
	}
	else{
		//Serial.println(F("Unknown or empty command");
		return "error_Unknown or empty command!";
	}
}

void calcTempOut() {
	DutyCycle = 0;                                      //Initialize to zero to avoid wrong reading 
	TempOut = 0;                                    //due to incorrect sampling etc  
	SquareWaveHighTime = pulseIn(TempOutPin, HIGH);
	SquareWaveLowTime = pulseIn(TempOutPin, LOW);
	DutyCycle = SquareWaveHighTime;						//Calculate Duty Cycle for the square wave 
	DutyCycle /= (SquareWaveHighTime + SquareWaveLowTime);
	TempOut = (DutyCycle - 0.320) / 0.00470;
	TempOutS = (String)TempOut;
}

void calcTempIn(){
	for (byte i = 0; i < 3; i++){
		if (i == 0){
			TmpIn1 = (5.0 * analogRead(TempInsidePin) * 100.0) / 1024;
		}
		if (i == 1){
			TmpIn2 = (5.0 * analogRead(TempInsidePin) * 100.0) / 1024;
		}
		if (i == 2){
			TmpIn3 = (5.0 * analogRead(TempInsidePin) * 100.0) / 1024;
		}
	}
	TmpIn = (TmpIn1 + TmpIn2 + TmpIn3) / 3;
}
void calcTempRoof(){
	for (byte i = 0; i < 3; i++){
		if (i == 0){
			TmpRoof1 = (5.0 * analogRead(TmpRoofPin) * 100.0) / 1024;
		}
		if (i == 1){
			TmpRoof2 = (5.0 * analogRead(TmpRoofPin) * 100.0) / 1024;
		}
		if (i == 2){
			TmpRoof3 = (5.0 * analogRead(TmpRoofPin) * 100.0) / 1024;
		}
	}
	TmpRoof = (TmpRoof1 + TmpRoof2 + TmpRoof3) / 3;
}

void autoAChandler(int temperature){
	if (temperature > TmpIn){
		if (temperature > TempOut){
			analogWrite(FanPin, 1023);
			MUXwrite(0, 1, 0, 0); //turns on heatroof
			heatroofON = true;
			MUXwrite(0, 1, 0, 1); //turns on heatin
			heatinON = true;
			analogWrite(FanPin, 341);
			FanSpeed = 1;
		}
		else if (temperature < TempOut){
			analogWrite(FanPin, 1023);
			MUXwrite(1, 1, 0, 0); //turns off heatroof
			heatroofON = false;
			MUXwrite(0, 1, 0, 1); //turns on heatin
			heatinON = true;
			analogWrite(FanPin, 682);
			FanSpeed = 2;
		}
	}
	else if (temperature < TmpIn){
		if (temperature > TempOut){
			analogWrite(FanPin, 1023);
			FanSpeed = 3;
			MUXwrite(1, 1, 0, 0); //turns off heatroof
			heatroofON = false;
			MUXwrite(1, 1, 0, 1); //turns off heatin
			heatinON = false;
		}
		else if (temperature < TempOut){
			analogWrite(FanPin, 341);
			FanSpeed = 1;
			MUXwrite(1, 1, 0, 0); //turns off heatroof
			heatroofON = false;
			MUXwrite(1, 1, 0, 1); //turns off heatin
			heatinON = false;
		}
	}
	else if (temperature == TmpIn){
		analogWrite(FanPin, 0);
		FanSpeed = 0;
		MUXwrite(1, 1, 0, 0); //turns off heatroof
		heatroofON = false;
		MUXwrite(1, 1, 0, 1); //turns off heatin
		heatinON = false;
	}
}