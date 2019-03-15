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
		break;

	case 2:
		Serial.println("Connection error: 0");
		break;

	default:
		Serial.println("Connection OK");
	}

	driver.microsteps(256);
	driver.intpol(true);
	digitalWrite(EN_PIN, LOW);    // Enable driver in hardware

	// driver 2
	driver2.beginSerial(115200);
	// Push at the start of setting up the driver resets the register to default
	driver2.push();

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
		break;

	case 2:
		Serial.println("Connection error: 0");
		break;

	default:
		Serial.println("Connection OK");
	}

	driver2.microsteps(256);
	driver2.intpol(true);
	digitalWrite(EN_PIN2, LOW);    // Enable driver in hardware
}

bool homing = false;

void loop() {

	if (Serial.available() > 0) {
		char receivedChar = Serial.read();
		if (receivedChar == 'i') {
			homing = true;
			driver2.shaft(0);
		} else if (receivedChar == 'v') {
			digitalWrite(STEP_PIN2, 1);
			delay(1);
			digitalWrite(STEP_PIN2, 0);
			delay(1);
		} else if (receivedChar == 'h') {
			digitalWrite(STEP_PIN, 1);
			delay(1);
			digitalWrite(STEP_PIN, 0);
			delay(1);
		} else if (receivedChar == 'r') {
			driver.shaft(0);
		} else if (receivedChar == 'l') {
			driver.shaft(1);
		} else if (receivedChar == 'd') {
			driver2.shaft(0);
		} else if (receivedChar == 'u') {
			driver2.shaft(1);
		}
	}

	int buttonState = digitalRead(TASTER_PIN);
	if (buttonState == HIGH) {
		homing = false;
		driver2.shaft(1);

		for (int i = 0; i < 15; i++) {
			digitalWrite(STEP_PIN2, 1);
			delay(1);
			digitalWrite(STEP_PIN2, 0);
			delay(10);
		}

		Serial.println("HOMED");
	} else {
		if (homing) {
			digitalWrite(STEP_PIN2, 1);
			delay(1);
			digitalWrite(STEP_PIN2, 0);
			delay(40);
		}
	}
	delay(10);
}
