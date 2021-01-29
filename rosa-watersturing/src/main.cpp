//#define DEBUG 1

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#ifndef DEBUG
#include <DMXSerial.h>
#endif

int dmxAddress = 222;
const int dmxAddressOffset = 0;
const int dipswitchPinCount = 8;
const int dipswitchPin[dipswitchPinCount] = {11, 10, 9, 8, 7, 6, 5, 4};

const int dirPin = 2;
const int stepPin = 3;
const int stepsPerRevolution = 200;

const int pinBoundery[2] = {A0, A1};

int slowdown = 4;
const int speedRange[2] = {0, 50};
int lastSpeed = 4;
int anglePercentage = 50;
const int angleRange[2] = {10, 100};
int lastAngle = 50;
int totalAngle;
long previousSweep = 0;

bool currentDirection = 0;

int boundary[2];
int currentStep = 0;

void printDisplay() {
    char textline[64];
    int isRunning = ((slowdown < speedRange[1]) ? 1 : 0);
    sprintf(textline, "dmx: %d %d", dmxAddress, isRunning);
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(2, 2);
    display.print(textline);
    display.setTextSize(1);
    sprintf(textline, "speed %d  angle %d", lastSpeed, lastAngle);
    display.setCursor(0, 25);
    display.print(textline);
    display.display();
}

void nextStep() {
    currentStep = currentStep + (1 * ((currentDirection) ? 1 : -1));
}

void calibrate() {
    display.clearDisplay();
    display.setCursor(12, 15);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.print("Calibrating...");
    display.display();

    currentStep = 0;

#ifdef DEBUG
    Serial.print("calibrating ...");
#endif

    for (int i = 0; i < 2; i++) {
        currentDirection = i;
        digitalWrite(dirPin, currentDirection);
        boundary[i] = currentStep;
        bool isWithinBoundery = digitalRead(pinBoundery[i]);

        while (isWithinBoundery) {
            digitalWrite(stepPin, HIGH);
            delayMicroseconds(100);
            digitalWrite(stepPin, LOW);
            delayMicroseconds(100);
            delay(2);
            nextStep();
            isWithinBoundery = digitalRead(pinBoundery[i]);
        }

        boundary[i] = currentStep;
    }

    totalAngle = abs(boundary[1] - boundary[0]);

#ifdef DEBUG
    Serial.print("Boundery left: ");
    Serial.println(boundary[0]);
    Serial.print("Boundery right: ");
    Serial.println(boundary[1]);
    Serial.print("total Angle: ");
    Serial.println(totalAngle);
#endif

    boundary[0] = 0;
    boundary[1] = totalAngle;
    currentStep = boundary[1];
    currentDirection = 0;

#ifdef DEBUG
    Serial.println(" done");
    Serial.print("Boundery left: ");
    Serial.println(boundary[0]);
    Serial.print("Boundery right: ");
    Serial.println(boundary[1]);
    delay(3000);
#endif
}

void readDipswitch() {
    dmxAddress = dmxAddressOffset;

    for (int i = 0; i < dipswitchPinCount; i++) {
        bool pin = !digitalRead(dipswitchPin[i]);
        dmxAddress = dmxAddress + (pin << i);
    }

#ifdef DEBUG
    //if (dmxAddress == 222) {
    Serial.println(dmxAddress);
    delay(100);
    //}
#endif
}

#ifndef DEBUG
void readDmx() {
    int speed = DMXSerial.read(dmxAddress);
    int angle = DMXSerial.read(dmxAddress + 1);
    bool isChanged = false;

    if (speed != lastSpeed) {
        isChanged = true;
        lastSpeed = speed;
        slowdown = speedRange[1] - map(speed, 0, 200, speedRange[0], speedRange[1]);
    }

    if (angle != lastAngle) {
        isChanged = true;
        lastAngle = angle;
        anglePercentage = map(angle, 0, 200, angleRange[0], angleRange[1]);
    }

    if (isChanged) {
        printDisplay();
    }
}
#endif

void sweep() {
    if (slowdown < speedRange[1]) {
        if (millis() > (previousSweep + slowdown)) {
            previousSweep = millis();
            bool isWithinBoundery = digitalRead(pinBoundery[currentDirection]);

            if (!isWithinBoundery) {
#ifdef DEBUG
                Serial.print("boundery: ");
                Serial.println(currentDirection);
#endif
                currentDirection = 1 - currentDirection;
            } else {
                float currentPercentage;

                if (currentDirection) {
                    currentPercentage = 100.00 * (((float)currentStep - ((float)totalAngle / 2)) / ((float)totalAngle / 2));

                    if (currentPercentage >= anglePercentage) {
                        isWithinBoundery = false;
                    }
                } else {
                    currentPercentage = 100.00 * ((((float)totalAngle / 2) - (float)currentStep) / ((float)totalAngle / 2));

                    if (currentPercentage >= anglePercentage) {
                        isWithinBoundery = false;
                    }
                }
#ifdef DEBUG
/*
                Serial.print("currentstep: ");
                Serial.print(currentStep);
                Serial.print(" - percentage: ");
                Serial.println(currentPercentage);
*/
#endif
                if (!isWithinBoundery) {
#ifdef DEBUG
                    Serial.print("auto boundery: ");
                    Serial.println(currentStep);
#endif
                    currentDirection = 1 - currentDirection;
                }
            }

            digitalWrite(dirPin, currentDirection);

            nextStep();

            digitalWrite(stepPin, HIGH);
            delayMicroseconds(100);
            digitalWrite(stepPin, LOW);
            delayMicroseconds(100);
        }
    }
}

void setup() {
    for (int i = 0; i < dipswitchPinCount; i++) {
        pinMode(dipswitchPin[i], INPUT_PULLUP);
    }

    readDipswitch();

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

#ifdef DEBUG
    Serial.begin(115200);
    Serial.println("hallo");
#endif

    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);

    pinMode(pinBoundery[0], INPUT_PULLUP);
    pinMode(pinBoundery[1], INPUT_PULLUP);

#ifndef DEBUG
    DMXSerial.init(DMXReceiver);
#endif

    calibrate();
    printDisplay();
}

void loop() {
#ifndef DEBUG
    readDmx();
#endif
    sweep();
}
