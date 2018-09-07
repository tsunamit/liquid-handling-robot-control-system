#include "LiquidHandlingRobot.h"
#include "globals.h"


const int STEPSORVOLTOFILL = 50;

// bool CALIBRATING = true;


StepperMotor xMotor(X_MOTOR_STEP_PIN, X_MOTOR_DIR_PIN, X_MIN_SWITCH_PIN, X_MAX_SWITCH_PIN);
StepperMotor yMotor(Y_MOTOR_STEP_PIN, Y_MOTOR_DIR_PIN, Y_MIN_SWITCH_PIN, Y_MAX_SWITCH_PIN);
StepperMotor zMotor(Z_MOTOR_STEP_PIN, Z_MOTOR_DIR_PIN, Z_MIN_SWITCH_PIN, Z_MAX_SWITCH_PIN);
Pipette pip (PIPETTE_S2V, PIPETTE_MAX_VOL, PIPETTE_STEP_PIN, PIPETTE_DIR_PIN);

LiquidHandlingRobot lhr(xMotor, yMotor, zMotor, pip,
                        SHAPEKO_XMAXMM, SHAPEKO_YMAXMM, SHAPEKO_ZMAXMM);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  lhr.goToHome();
  delay(1000);

  while (CALIBRATING) {
    if (lhr.calibrate() == true) {
      CALIBRATING = false;
      lhr.showMaxSteps();
      delay(500);
    }
  }

  lhr.goToHome();
  lhr.dispenseWaste();
}

void loop() {
  // put your main code here, to run repeatedly:
//  traceGrid();
  lhr.goToHome();
  delay(300);
  serialDilution(100, 2);
  delay(300);

}

// 200 uL per well
// 1/2 => 100 water, 100 dye
// 1:4 => 100 water, 100 1:2 dye
// 1:8 => 100 water, 100 1:4 dye
// 1:16 => ...
// 1:32 =>
// 1:64 =>
// 1:128 =>
// 1:256 =>
// 1:512 =>
// 1:1024 =>
// 1:2048 =>
// 1:4096 =>


void pipetteTester() {
  while (true) {
    lhr.goToHome();
    delay(300);
    //lhr.emptyLiquidInWaste(0, 0, 0);
    lhr.goTo(100, 100, 0);
    lhr.drawLiquid(STEPSORVOLTOFILL, 20);
    delay (100);
    lhr.goTo(50, 100, 0);
    lhr.dispenseLiquid(STEPSORVOLTOFILL, 20);
    delay (100);
  }
}

void serialDilution(float totalVol, float dilutionFactor) {
  int lastWellOffset = WELL_COLS * DISTANCE_BETWEEN_WELLS;

  if (dilutionFactor < 1) {
    Serial.println("Clearly, you don't know how to math... exiting (Dilution factor must be greater than 1");
    return;
  }

  float dilutantVol[12];
  dilutantVol[11] = totalVol - totalVol / dilutionFactor;

  for (int i = sizeof(dilutantVol) / sizeof(dilutantVol[0]) - 2; i >= 0; i--)
    dilutantVol[i] = (totalVol + dilutantVol[i + 1]) * (dilutionFactor - 1) / dilutionFactor;

  for (int i = 0; i < sizeof(dilutantVol) / sizeof(dilutantVol[0]); i++)
    Serial.println(dilutantVol[i]);


  if (dilutantVol[0]*dilutionFactor > MAX_WELL_VOLUME) {
    Serial.println("Well cannot hold that much liquid... try again");
    return;
  }

  lhr.goTo(LOAD_ORIGIN_X, LOAD_ORIGIN_Y, 0);

  for (int i = 0; i < 12; i ++) {
    // for each well, draw and dispense diluatant volume
    if (i != 0) lhr.goTo(lhr.x(), lhr.y(), LOAD_Z_CLEARANCE);
    lhr.goTo(LOAD_ORIGIN_X, LOAD_ORIGIN_Y, LOAD_Z_CLEARANCE);
    lhr.drawLiquid(dilutantVol[i], LOAD_POS_Z);

    lhr.goTo(WELL_ORIGIN_X, WELL_ORIGIN_Y + i * DISTANCE_BETWEEN_WELLS, lhr.z());        // move above well position to dispense in
    lhr.dispenseLiquid(dilutantVol[i], WELL_PLATE_Z_POS);

    Serial.print(i+1); Serial.print(' '); Serial.println(dilutantVol[i]);
  }

  // Load solute into well, performing serial dilution
  float stockVol = dilutantVol[0];

  lhr.goTo(LOAD_ORIGIN_X + LOAD_ORIGIN_OFFSET, LOAD_ORIGIN_Y, LOAD_Z_CLEARANCE);  // move to above the stock load position
  lhr.drawLiquid(stockVol, LOAD_POS_Z);     // draw up stock solution
  lhr.goTo(WELL_ORIGIN_X, WELL_ORIGIN_Y, WELL_PLATE_Z_CLEARANCE);        // move above well position to dispense in
  lhr.dispenseLiquid(stockVol);
  lhr.mix(0.5 * totalVol, WELL_PLATE_Z_POS, 3);       // mix solution

  for (int i = 0; i < 11; i ++) {
    float transVol = dilutantVol[i + 1];
    lhr.goTo(WELL_ORIGIN_X, WELL_ORIGIN_Y + i * DISTANCE_BETWEEN_WELLS, WELL_PLATE_Z_CLEARANCE);        // move above well position to dispense in
    lhr.drawLiquid(transVol, WELL_PLATE_Z_POS);
    
    lhr.goTo(WELL_ORIGIN_X, WELL_ORIGIN_Y + (i+1) * DISTANCE_BETWEEN_WELLS, WELL_PLATE_Z_CLEARANCE);        // move above well position to dispense in
    lhr.dispenseLiquid(transVol);           // dispense
    lhr.mix(0.5 * totalVol, LOAD_POS_Z, 3);       // mix solution
  }
}

void loadFluid() {
  lhr.goTo(LOAD_ORIGIN_X, LOAD_ORIGIN_Y, 0);              // move to above the load solution
  lhr.goTo(LOAD_ORIGIN_X, LOAD_ORIGIN_Y, LOAD_POS_Z);     // drop pipette into solution
  Serial.println("Loading fluid");
  // TODO LOAD FLUID DELAY
  lhr.goTo(LOAD_ORIGIN_X, LOAD_ORIGIN_Y, 0);              // lift pipette back up
}

void dispenseFluid() {

  float ox = lhr.x();
  float oy = lhr.y();
  float oz = lhr.z();

  lhr.goTo(ox, oy, WELL_DISPENSE_POS_Z);  // drop pipette into well
  Serial.println("Dispensing fluid");
  // TODO: FLUID DELAY
  lhr.goTo(ox, oy, 0);                    // lift pipette from well POTENTIALLY A PART OF THE DISPENSE CLASS
  // i.e. return from where you came from

}

void traceGrid() {
lhr.goTo(lhr.x(), lhr.y(), 60);
  
  for (int i = 0; i <= SHAPEKO_XMAXMM; i += 50) {
    for (int j = 0; j <= SHAPEKO_YMAXMM; j += 50) {
      lhr.goTo(i, j, lhr.z());
      lhr.showPos();
      delay(10000);
    }
  }
}

void drawCube(){
  lhr.goTo(50, 50, 20);
  lhr.goTo(90, 50, 20);
  lhr.goTo(90, 90, 20);
  lhr.goTo(50, 90, 20);
  lhr.goTo(50, 90, 60);
  lhr.goTo(90, 90, 60);
  lhr.goTo(90, 50, 60);
  lhr.goTo(50, 50, 60);
}

void drawDiagonals(){
  lhr.goTo(50, 50, 20);
  lhr.goTo(100, 100, 40);
  lhr.goTo(50, 100, 20);
  lhr.goTo(100, 50, 40);
}


