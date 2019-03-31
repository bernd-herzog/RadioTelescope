#define TASTER_PIN    4

// rotate
#define RX_PIN    5   // To UARTs
#define TX_PIN    6   // To 1k Ohm to UART

#define STEP_PIN  7  // Step on rising edge
#define EN_PIN    8  // LOW: Driver enabled. HIGH: Driver disabled

//tilt
#define RX_PIN2    9   // To UARTs
#define TX_PIN2    10   // To 1k Ohm to UART

#define STEP_PIN2  11
#define EN_PIN2    12

#include <TMC2208Stepper.h>
TMC2208Stepper rotateDriver = TMC2208Stepper(RX_PIN, TX_PIN, true); // rotate
TMC2208Stepper tiltDriver = TMC2208Stepper(RX_PIN2, TX_PIN2, true); // up/down

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	while (!Serial)
		;

	Serial.println("Start...");
	pinMode(TASTER_PIN, INPUT);

	// driver 1
	rotateDriver.beginSerial(115200);
	// Push at the start of setting up the driver resets the register to default
	rotateDriver.push();

	pinMode(EN_PIN, OUTPUT);
	pinMode(STEP_PIN, OUTPUT);

	rotateDriver.pdn_disable(true);     // Use PDN/UART pin for communication
	rotateDriver.I_scale_analog(false); // Use internal voltage reference
	rotateDriver.rms_current(200); // Set driver current = 500mA, 0.5 multiplier for hold current and RSENSE = 0.11.
	rotateDriver.toff(2);               // Enable driver in software

	uint32_t drv_status;
	rotateDriver.DRV_STATUS(&drv_status);
	Serial.print("drv_status=");
	Serial.println(drv_status, HEX);

	switch (rotateDriver.test_connection()) {
	case 1:
		Serial.println("Connection error: F");
		break;

	case 2:
		Serial.println("Connection error: 0");
		break;

	default:
		Serial.println("Connection OK");
	}

	rotateDriver.microsteps(256);
	rotateDriver.intpol(true);
	digitalWrite(EN_PIN, LOW);    // Enable driver in hardware

	// driver 2
	tiltDriver.beginSerial(115200);
	// Push at the start of setting up the driver resets the register to default
	tiltDriver.push();

	pinMode(EN_PIN2, OUTPUT);
	pinMode(STEP_PIN2, OUTPUT);

	tiltDriver.pdn_disable(true);     // Use PDN/UART pin for communication
	tiltDriver.I_scale_analog(false); // Use internal voltage reference
	tiltDriver.rms_current(350); // Set driver current = 500mA, 0.5 multiplier for hold current and RSENSE = 0.11.
	tiltDriver.toff(2);               // Enable driver in software

	uint32_t drv_status2;
	tiltDriver.DRV_STATUS(&drv_status2);
	Serial.print("drv_status=");
	Serial.println(drv_status2, HEX);

	switch (tiltDriver.test_connection()) {
	case 1:
		Serial.println("Connection error: F");
		break;

	case 2:
		Serial.println("Connection error: 0");
		break;

	default:
		Serial.println("Connection OK");
	}

	tiltDriver.microsteps(256);
	tiltDriver.intpol(true);
	digitalWrite(EN_PIN2, LOW);    // Enable driver in hardware
}

bool homing = false;

int px = 0;
bool dx = false;
int py = 0;
bool dy = false;

void loop() {
	if (homing) {
		int buttonState = digitalRead(TASTER_PIN);
		if (buttonState == HIGH) {
			homing = false;
			px = 0;
			py = 0;
			changeDirectionUp();

			for (int i = 0; i < 150; i++) {
				doStepVertical();
				delay(20);
			}

			Serial.println("HOMED");

		} else {
			doStepVertical();
			delay(40);
		}
	} else {
		if (Serial.available() > 0) {
			char receivedChar = Serial.read();
			if (receivedChar == 'i') {
				homing = true;
				changeDirectionDown();
			} else if (receivedChar == 'v') {
				doStepVertical();
			} else if (receivedChar == 'h') {
				doStepHorrizontal();
			} else if (receivedChar == 'r') {
				changeDirectionRight();
			} else if (receivedChar == 'l') {
				changeDirectionLeft();
			} else if (receivedChar == 'd') {
				changeDirectionDown();
			} else if (receivedChar == 'u') {
				changeDirectionUp();
			} else if (receivedChar == 's') {
				Serial.print("x:");
				Serial.print(px);
				Serial.print(" y:");
				Serial.print(py);

				if (dx)
					Serial.print(" right");
				else
					Serial.print(" left");

				if (dy)
					Serial.println(" up");
				else
					Serial.println(" down");
			}
		}
	}
	delay(10);
}

void doStepVertical() {
	digitalWrite(STEP_PIN2, 1);
	delay(1);
	digitalWrite(STEP_PIN2, 0);
	delay(1);

	py = py + (dy ? 1 : -1);
}

void doStepHorrizontal() {
	digitalWrite(STEP_PIN, 1);
	delay(1);
	digitalWrite(STEP_PIN, 0);
	delay(1);

	px = px + (dx ? 1 : -1);
}

void changeDirectionUp() {
	tiltDriver.shaft(1);
	dy = true;
}

void changeDirectionDown() {
	tiltDriver.shaft(0);
	dy = false;
}

void changeDirectionRight() {
	rotateDriver.shaft(0);
	dx = true;
}

void changeDirectionLeft() {
	rotateDriver.shaft(1);
	dx = false;
}

