// Device Group proudly presents its part of the work on the
// ............SMARTer HOUSE 2014............
// |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
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
#define size4 24
String allFunctions[size4] = { "fa", "do", "wl", "st", "wo", "ec", "tmpout", "elcon", "tmpin", "tmproof", "ldr", "buzz", "t2", "li", "aled", "heatroof", "heatin", "t1", "lo", "autoac", "autolo", "sa", "autoli", "fan" };
// String input
String inMsg = ""; // saves messages from local server until gets "!" which means end of message
char inChar; // incoming character that is saved in inMsg
// booleans // moslty to help in logic or to keep track of virtual functions (not readeble from hardware); setting
// initial conditions
boolean SecurityAlarm = false;
boolean FirstRun = true;
boolean saTrig, faTrig, wlTrig = false;//keep track if corresponding alarm is triggered
boolean autoli, liON, autolo, loON, autoAC = false;
// integers // (holding values from pins)
int WindowIsOpened, WindowCurrentState = 0;
int FireAlarm;
int DoorIsOpen;
int WaterLeakage;
int StoveOn, StoveCurrentState = 0;
int ElectricityCut;
int LDRsensor;
int FanSpeed = 0;
double TempOut;
long alarmReportTimer;
long tempReportTimer;

float  DutyCycle = 0;                               //DutyCycle that need to calculate
unsigned long  SquareWaveHighTime = 0;				//High time for the square wave
unsigned long  SquareWaveLowTime = 0;				//Low time for the square wave
unsigned long  Temperature = 0;						//Calculated temperature using dutycycle  
String tmpoutString;

void setup()
{
	Serial.begin(38400);
	inMsg.reserve(10);
	//Settings pinMode
	pinMode(FanPin, OUTPUT);
	for (int i = 0; i < size1; i++){
		pinMode(digitalInputPins[i], INPUT);
	}
	for (int i = 0; i < size2; i++){
		pinMode(analogInputPins[i], INPUT);
	}
	for (int i = 0; i < size3; i++){
		pinMode(MUXpins[i], OUTPUT);
	}
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
	//Serial.println((String)first + " " + (String)second + " " + (String)third + " " + (String)fourth);
	delay(100);
}

void loop()
{
	if (FirstRun){
		MUXwrite(1, 1, 0, 1);//Turns OFF heating element
		MUXwrite(1, 1, 0, 0);//Turns OFF heating element wind
		MUXwrite(1, 0, 1, 0);//Turns OFF inside light
		MUXwrite(1, 1, 1, 1);//Turns OFF outside light
		MUXwrite(0, 0, 0, 0);//Turns buzzer off
		MUXwrite(1, 1, 1, 0); //Timer 1 is set to off
		MUXwrite(1, 0, 0, 1);//Timer2 is used in a loop to prevent random MUX pin values
		MUXwrite(1, 0, 1, 1); //Alarm LED turned off
		calcTemp(); //initialization of tmpout (stored as string in temperature2)
		Serial.println("autochkstart!");
		CheckAll();
		Serial.println("eol!");
		alarmReportTimer = millis();
		tempReportTimer = millis();
		FirstRun = false;
	}
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
			Serial.println("wl_alarm!");
		}
		if (faTrig){
			Serial.println("fa_alarm!");
		}
		if (saTrig){
			Serial.println("sa_alarm!");
		}
	}
	if (millis() - tempReportTimer > 12000){
		tempReportTimer = millis();
		calcTemp();
		Serial.println("tmpout_" + tmpoutString + "!");
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
			Serial.println("st_on!");
		}
		else if (StoveCurrentState == LOW){
			Serial.println("st_off!");
		}
	}
	WindowCurrentState = digitalRead(WindowIsOpenedPin);
	if (WindowIsOpened != WindowCurrentState){
		if (WindowCurrentState == HIGH){
			Serial.println("wo_on!");
		}
		else if (WindowCurrentState == LOW){
			Serial.println("wo_off!");
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
			if (inMsg.endsWith("test")){
				TestRequest(inMsg);
			}
			else if (inMsg.endsWith("chk")){
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

void TestRequest(String command){
	Serial.println("Test requested");
}
void CheckRequest(String command){
	//Serial.println("Check requested"); //debug msg
	if (command.startsWith("all")){
		CheckAll();
	}
	else if (command.startsWith("fan")){
		Serial.println(CheckStatus("fan"));
	}
	else if (command.startsWith("do")){
		//Serial.println(command);//debug msg
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
	else if (command.startsWith("ldr")){
		Serial.println(CheckStatus("ldr"));
	}
	else if (command.startsWith("buzz")){
		Serial.println(CheckStatus("buzz"));
	}
	else if (command.startsWith("t2")){
		Serial.println(CheckStatus("t2"));
	}
	else if (command.startsWith("li")){
		Serial.println(CheckStatus("li"));
	}
	else if (command.startsWith("aled")){
		Serial.println(CheckStatus("aled"));
	}
	else if (command.startsWith("heatroof")){
		Serial.println(CheckStatus("heatroof"));
	}
	else if (command.startsWith("heatin")){
		Serial.println(CheckStatus("heatin"));
	}
	else if (command.startsWith("t1")){
		Serial.println(CheckStatus("t1"));
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
	else{
		Serial.println(CheckStatus("error_Invalid command!"));
	}
}
void MsgHandler(String command){
	if (command.equals("sa_on")){
		if (digitalRead(DoorIsOpenedPin) == LOW || digitalRead(WindowIsOpenedPin) == HIGH || digitalRead(StoveOnPin) == HIGH) {
			Serial.println("error_Door and windows must be closed to activate!");
		}
		else {
			SecurityAlarm = true;
		}
	}
	else if (command.equals("sa_off")){
		SecurityAlarm = false;
		saTrig = false;
		wlTrig = false;
		faTrig = false;
	}
	else if (command.equals("li_on")){
		if (autoli){
			Serial.println("error_Auto Light option is ON.");
		}
		else{
			MUXwrite(0, 0, 1, 0);
			liON = true;
		}
	}
	else if (command.equals("li_off")){
		MUXwrite(1, 0, 1, 0);
		liON = false;
	}
	else if (command.equals("lo_on")){
		if (autolo){
			Serial.println("error_Auto Light option is ON.");
		}
		else{
			MUXwrite(0, 1, 1, 1);
			loON = true;
		}
	}
	else if (command.equals("lo_off")){
		MUXwrite(1, 1, 1, 1);
		loON = false;
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
	if (command.startsWith("fan")){
		if (!autoAC){
			if (command.endsWith("1")){
				analogWrite(FanPin, 1023);
				delay(100);
				analogWrite(FanPin, 341);
				FanSpeed = 1;
			}
			if (command.endsWith("2")){
				analogWrite(FanPin, 1023);
				delay(100);
				analogWrite(FanPin, 682);
				FanSpeed = 2;
			}
			if (command.endsWith("3")){
				analogWrite(FanPin, 1023);
				FanSpeed = 3;
			}
			if (command.endsWith("off")){
				analogWrite(FanPin, 0);
				FanSpeed = 0;
			}
			else{
				Serial.println("error_Unknown command or syntax error. To set fan speed use i.e. fan_1! (1 min - 3 max)!");
			}
		}
		else{
			Serial.println("error_To manually controll the fan, turn OFF AutoAC first");
		}
	}
	else {
		//Serial.println("error_Syntax error. \t\"" + command + "\"\t Unknown command!");
		Serial.println("error_Syntax error. Unknown command!");
	}
}
void CheckAll(){
	for (int i = 0; i < size4; i++){
		Serial.println(CheckStatus(allFunctions[i]));
	}
}
void UpdateDevicesStatus(){
	for (int i = 0; i < size4; i++){
		CheckStatus(allFunctions[i]);
	}
}


String CheckStatus(String what){
	//Serial.println(what); //debug msg
	if (what.equals("fa")){
		FireAlarm = digitalRead(FireAlarmPin);
		if (FireAlarm == HIGH){
			//Serial.println("fa_on!"); //debug msg
			MUXwrite(1, 0, 0, 0);
			faTrig = true;
			return "fa_alarm!";
		}
		else if (FireAlarm == LOW){
			//Serial.println("fa_off!");
			return "fa_off!";
		}
	}
	else if (what.equals("do")){
		DoorIsOpen = digitalRead(DoorIsOpenedPin);
		if (DoorIsOpen == LOW){
			//Serial.println("do_on!");
			return "do_on!";
		}
		else if (DoorIsOpen == HIGH){
			//Serial.println("do_off!");
			return "do_off!";
		}
	}
	else if (what.equals("wl")){
		WaterLeakage = digitalRead(WaterLeakagePin);
		if (WaterLeakage == HIGH){
			//Serial.println("wl_on!");
			MUXwrite(1, 0, 0, 0);
			wlTrig = true;
			return "wl_on!";
		}
		else if (WaterLeakage == LOW){
			//Serial.println("wl_off!");
			return "wl_off!";
		}

	}
	else if (what.equals("st")){
		StoveOn = digitalRead(StoveOnPin);
		if (StoveOn == HIGH){
			//Serial.println("st_on!");
			return "st_on!";
		}
		else if (StoveOn == LOW){
			//Serial.println("st_off!");
			return "st_off!";
		}
	}
	else if (what.equals("wo")){
		WindowIsOpened = digitalRead(WindowIsOpenedPin);
		if (WindowIsOpened == HIGH){
			//Serial.println("wo_on!");
			return "wo_on!";
		}
		else if (WindowIsOpened == LOW){
			//Serial.println("wo_off!");
			return "wo_off!";
		}
	}
	else if (what.equals("ec")){
		ElectricityCut = digitalRead(ElectricityCutPin);
		if (ElectricityCut == HIGH){
			//Serial.println("ec_on!");
			return ".";
		}
		else if (ElectricityCut == LOW){
			return ".";
			//Serial.println("ec_off!");
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
		return tmpoutString;
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
	else{
		//Serial.println("Unknown or empty command");
		return "error_Unknown or empty command!";
	}
}

void calcTemp() {
	DutyCycle = 0;                                      //Initialize to zero to avoid wrong reading 
	Temperature = 0;                                    //due to incorrect sampling etc  
	SquareWaveHighTime = pulseIn(TempOutPin, HIGH);
	SquareWaveLowTime = pulseIn(TempOutPin, LOW);
	DutyCycle = SquareWaveHighTime;						//Calculate Duty Cycle for the square wave 
	DutyCycle /= (SquareWaveHighTime + SquareWaveLowTime);
	Temperature = (DutyCycle - 0.320) / 0.00470;
	tmpoutString = (String)Temperature;
}