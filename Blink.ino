#define TASTER_PIN    4

#define RX_PIN    5   // To UARTs
#define TX_PIN    6   // To 1k Ohm to UART

#define STEP_PIN  7  // Step on rising edge
#define EN_PIN    8  // LOW: Driver enabled. HIGH: Driver disabled

#define RX_PIN2    9   // To UARTs
#define TX_PIN2    10   // To 1k Ohm to UART

#define STEP_PIN2  11
#define EN_PIN2    12


#include <TMC2208Stepper.h>
TMC2208Stepper driver = TMC2208Stepper(RX_PIN, TX_PIN, true); // rotate
TMC2208Stepper driver2 = TMC2208Stepper(RX_PIN2, TX_PIN2, true); // up/down

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	while (!Serial)
		;

	Serial.println("Start...");
	pinMode(TASTER_PIN, INPUT);


	// driver 1
	driver.beginSerial(115200);
	// Push at the start of setting up the driver resets the register to default
	driver.push();
	// Prepare pins

	pinMode(EN_PIN, OUTPUT);
	pinMode(STEP_PIN, OUTPUT);

	driver.pdn_disable(true);     // Use PDN/UART pin for communication
	driver.I_scale_analog(false); // Use internal voltage reference
	driver.rms_current(200); // Set driver current = 500mA, 0.5 multiplier for hold current and RSENSE = 0.11.
	driver.toff(2);               // Enable driver in software

	uint32_t drv_status;
	driver.DRV_STATUS(&drv_status);
	Serial.print("drv_status=");
	Serial.println(drv_status, HEX);

	switch (driver.test_connection()) {
	case 1:
		Serial.println("Connection error: F");
	case 2:
		Serial.println("Connection error: 0");
	default:
		Serial.println("Connection OK");
	}

	//digitalWrite(EN_PIN, LOW);    // Enable driver in hardware

	driver.microsteps(256);
	driver.intpol(true);
	digitalWrite(EN_PIN, LOW);    // Enable driver in hardware



	// driver 2
	driver2.beginSerial(115200);
	// Push at the start of setting up the driver resets the register to default
	driver2.push();
	// Prepare pins
	//pinMode(EN_PIN, OUTPUT);
	//pinMode(STEP_PIN, OUTPUT);

	pinMode(EN_PIN2, OUTPUT);
	pinMode(STEP_PIN2, OUTPUT);

	driver2.pdn_disable(true);     // Use PDN/UART pin for communication
	driver2.I_scale_analog(false); // Use internal voltage reference
	driver2.rms_current(200); // Set driver current = 500mA, 0.5 multiplier for hold current and RSENSE = 0.11.
	driver2.toff(2);               // Enable driver in software

	uint32_t drv_status2;
	driver2.DRV_STATUS(&drv_status2);
	Serial.print("drv_status=");
	Serial.println(drv_status2, HEX);

	switch (driver2.test_connection()) {
	case 1:
		Serial.println("Connection error: F");
	case 2:
		Serial.println("Connection error: 0");
	default:
		Serial.println("Connection OK");
	}


	driver2.microsteps(256);
	driver2.intpol(true);
	digitalWrite(EN_PIN2, LOW);    // Enable driver in hardware
}

//int i = 0;
//bool dir = true;

bool homing = false;

void loop() {

	if (Serial.available() > 0) {
		char receivedChar = Serial.read();
		if (receivedChar == 'h')
		{
			homing = true;
		}
	}

	int buttonState = digitalRead(TASTER_PIN);
	if (buttonState == HIGH)
	{
		 homing = false;
		 driver2.shaft(1);

		 //digitalWrite(STEP_PIN, !digitalRead(STEP_PIN));
		 for (int i = 0; i < 30; i++)
		 {
			 digitalWrite(STEP_PIN2, !digitalRead(STEP_PIN2));
		 	 delay(10);
		 }

		 Serial.println("HOMED");
	}
	else
	{
		if (homing)
		{
			digitalWrite(STEP_PIN2, !digitalRead(STEP_PIN2));
			delay(50);
		}
	}

	//i++;

	/*int o = (i % 300) - 5;
	if (o < 0)
		o = -o;*/

	//driver.VACTUAL(10 * (o));
	//driver.VACTUAL(10);
	//delay(10);
	//driver2.VACTUAL(100);


	 //digitalWrite(STEP_PIN, !digitalRead(STEP_PIN));
	 //delay(10);

	 //if (i % 100 == 0) {
	 //driver.shaft();

	/* uint32_t g;
	 driver.GCONF(&g);
	 Serial.println(g);

	 uint32_t data;
	 driver.DRV_STATUS(&data);
	 Serial.println(data, BIN);*/

	 //}

	//driver.step();
	//digitalWrite(STEP_PIN, HIGH);
	//delay(1);
	//digitalWrite(STEP_PIN, LOW);
	delay(10);

	/*uint32_t data;
	driver.MSCNT(&data);
	Serial.println(data, DEC);*/

	/*uint32_t ms = millis();
	 static uint32_t last_time = 0;
	 if ((ms - last_time) > 2000) {
	 if (dir) {
	 Serial.println("Dir -> 0");
	 driver.shaft(0);
	 } else {
	 Serial.println("Dir -> 1");
	 driver.shaft(1);
	 }
	 dir = !dir;
	 last_time = ms;
	 }*/
}
