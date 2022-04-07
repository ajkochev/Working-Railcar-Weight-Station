
// Version 1.0-2-2022  (Version)-(Month)-(Year) Released
// Created by Anthony J. Kochevar(ajkochev) for N Scale Working Railcar Weight Station on Thingiverse.com
// This code is public domain and can be copied, modified and freely distributed.

#include <Wire.h>
#include <HX711_ADC.h>  // Library for HX711 Sensor and Board: https://github.com/olkal/HX711_ADC
#include <LiquidCrystal_I2C.h>  // Library used for LCD with i2C:  https://github.com/johnrickman/LiquidCrystal_I2C

// Define I2C Address - change if reqiuired.  Default is usually 0x27.
LiquidCrystal_I2C lcd(0x27,20,4);  // Initalize I2C Library for LCD with 16x2(use 20,4) screen.  "lcd" will be its name in the code.

// Set Button Inputs
int TareCalButton = 6;
int WeightButton = 7;
int SettingsButton = 8;

boolean initCal = false;
int timeDelay = 3000;  //Set LCD display time per line.  3000 = 3 seconds. Use only 1500, 3000 and 5000 as values.  See Settings button in loop function.

unsigned long startTime = 0;
unsigned long startButton = 0;
unsigned long endButton = 0;

float grams = 0;

boolean secondLine = false;  //  User to set if second line should be displayed
boolean backlightOn = true;
boolean TareButtonPress = false;
boolean SettingsButtonPress = false;

//pins:
const int HX711_dout = 4; //Arduino pin to HX711 DAT/DT pin
const int HX711_sck = 5; //Arduino pin to HX711 SCK/SCL pin

//HX711 constructor:
HX711_ADC WeightScale(HX711_dout, HX711_sck);  // Init of library.  "WeightScale" will be the name of the scale.
  

// F() used in serial and LCD output as strings do not need saved in RAM.

  void setup()
  {
    Serial.begin(57600);
    lcd.begin(16,2);  // Start LCD screen with 16x2 display.
    lcd.backlight();
    delay(2000);
    Serial.println(F("Model Railroad Railcar Weight Scale by Anthony J. Kochevar."));
    Serial.println(F("Version 1.0-2-2022"));  // Change Version to match top comments.
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F(" Model Railroad "));
    lcd.setCursor(0,1);
    lcd.print(F("  Weight Scale  "));
    delay(5000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F(" By AJKochevar"));
    lcd.setCursor(0,1);
    lcd.print(F("ver 1.0-2-2022"));  // Change Version to match top comments.
    delay(5000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Starting up..."));
    Serial.println(F("Starting up..."));
    
    pinMode(TareCalButton, INPUT_PULLUP);  // Tare and Calibration Button
    pinMode(WeightButton, INPUT_PULLUP);  //  Weight Button
    pinMode(SettingsButton, INPUT_PULLUP);  // Settings and Cancel Button

    delay(1000);  // Power up stabilizing time.
    WeightScale.begin();
    unsigned long stabilizingtime = 2000; // Preciscion right after power-up can be improved by adding a few seconds of stabilizing time.  2000 = 2 Seconds
    WeightScale.start(stabilizingtime, true);  // Initalize scale.  Parameter "true" will also tare the scale.
    
    //  Start up error checks for scale
    if (WeightScale.getSPS() < 7) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(F("Sampling to low "));
      lcd.setCursor(0,1);
      lcd.print(F("check wiring."));
      Serial.println(F("!!Sampling rate is lower than specification, check wiring from Arduino to HX711 and pin designations."));
      while(1);  //  Lock Aurduino to require a reboot
    }
    else if (WeightScale.getSPS() > 100) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(F("Sampling to high"));
      lcd.setCursor(0,1);
      lcd.print(F("check wiring."));
      Serial.println("!!Sampling rate is higher than specification, check wiring from Arduino to HX711 and pin designations.");
      while(1);  //  Lock Aurduino to require a reboot
    }
    if (WeightScale.getTareTimeoutFlag() || WeightScale.getSignalTimeoutFlag()) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(F("Timeout, "));
      lcd.setCursor(0,1);
      lcd.print(F("check wiring."));
      Serial.println(F("Timeout getting signal, check wiring from Arduino to HX711 and pin designations."));
      while(1);  //  Lock Arduino to require a reboot.
    }
    else {
      WeightScale.setCalFactor(1.0); // Initalize calibration value
      calibrate_scale();
    }
  }

  void tare_scale() {
    lcd.clear();
    Serial.println(F("Tareing... Clear the track and press button."));
    startTime = millis();
    secondLine = false;
    while (digitalRead(TareCalButton) == HIGH) {  // Wait for track to clear and tare button pressed when done.
      lcd.setCursor(0,0);
      lcd.print(F("Tareing... "));
      if ((millis() <= startTime + timeDelay) && (secondLine == false)) {
        lcd.setCursor(0,1);
        lcd.print(F("Clear the track "));
      }
      else if ((millis() > startTime + timeDelay) && (secondLine == false)) {
        secondLine = true;
        startTime = millis();
      }
      if ((millis() <= startTime + timeDelay) && (secondLine == true)) {
        lcd.setCursor(0,1);
        lcd.print(F("and press button"));
      }
      else if ((millis() > startTime + timeDelay) && (secondLine == true)) {
        secondLine = false;
        startTime = millis();
      }
    if (digitalRead(SettingsButton) == LOW) {  // Can cancel tare with settings button.
        Serial.println(F("Tareing cancelled..."));
        lcd.setCursor(0,0);
        lcd.print(F("Tareing         "));
        lcd.setCursor(0,1);
        lcd.print(F("cancelled...    "));
        delay(timeDelay);
        ready_print();
        return;
        }
     }
     Serial.println(F("Tareing scale..."));
     lcd.setCursor(0,0);
     lcd.print(F("Tareing         "));
     lcd.setCursor(0,1);
     lcd.print(F("scale...        "));

     // Perform tare
     while (WeightScale.update()) {};  // Update scale weight data until smooth.
     WeightScale.refreshDataSet();
     WeightScale.tareNoDelay();
     WeightScale.tare();  
     delay(2000);
     
     if (WeightScale.getTareStatus() == true) {
       Serial.println(F("Tareing complete."));
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print(F("Tareing"));
       lcd.setCursor(0,1);
       lcd.print(F("complete."));
       delay(timeDelay);
       ready_print();
       return; 
     }
     else {
       Serial.println(F("Tareing failed."));
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print(F("Tareing"));
       lcd.setCursor(0,1);
       lcd.print(F("failed."));
       delay(timeDelay);
       tare_scale(); 
     }
  }

  void calibrate_scale() {
    lcd.clear();
    Serial.println(F("Calibrating... Clear the track and press button."));
    startTime = millis();
    secondLine = false;
    while (digitalRead(TareCalButton) == HIGH) {  // Wait for track to clear and tare button pressed when done.
      lcd.setCursor(0,0);
      lcd.print(F("Calibrating... "));
      if ((millis() <= startTime + timeDelay) && (secondLine == false)) {
        lcd.setCursor(0,1);
        lcd.print(F("Clear the track "));
      }
      else if ((millis() > startTime + timeDelay) && (secondLine == false)) {
        secondLine = true;
        startTime = millis();
      }
      if ((millis() <= startTime + timeDelay) && (secondLine == true)) {
        lcd.setCursor(0,1);
        lcd.print(F("and press button"));
      }
      else if ((millis() > startTime + timeDelay) && (secondLine == true)) {
        secondLine = false;
        startTime = millis();
      }
      if ((digitalRead(SettingsButton) == LOW) && (initCal == false)) {  // Startup calibration needed to get inital values.  Calibration cannot be cancelled on startup.
        Serial.println(F("Startup Calibration cannot be cancelled."));
        lcd.setCursor(0,0);
        lcd.print(F("Startup         "));
        lcd.setCursor(0,1);
        lcd.print(F("Calibration     "));
        delay(timeDelay);
        lcd.setCursor(0,0);
        lcd.print(F("cannot be       "));
        lcd.setCursor(0,1);
        lcd.print(F("cancelled...    "));
        delay(timeDelay);
        lcd.clear();
        Serial.println(F("Calibrating... Clear the track and press button."));
        startTime = millis();
        secondLine = false;
      }
      else if ((digitalRead(SettingsButton) == LOW) && (initCal == true)) {  // Can cancel with Settings button if startup calibration already done.
        Serial.println(F("Calibration cancelled..."));
        lcd.setCursor(0,0);
        lcd.print(F("Calibration     "));
        lcd.setCursor(0,1);
        lcd.print(F("cancelled...    "));
        delay(timeDelay);
        ready_print();
        return;
        }
       }
       Serial.println(F("Tareing first..."));
       lcd.setCursor(0,1);
       lcd.print(F("Tareing first..."));

       while (WeightScale.update()) {};  // Update scale weight data until smooth.
       WeightScale.refreshDataSet();
       WeightScale.tareNoDelay();
       WeightScale.tare();
       delay(2000);

       if (WeightScale.getTareStatus() == true) {
         Serial.println(F("Tare complete."));
         lcd.setCursor(0,1);
         lcd.print(F("Tare complete.  "));
         delay(timeDelay); 
       }
       else {
         Serial.println(F("Tareing failed."));
         lcd.setCursor(0,1);
         lcd.print(F("Taring failed.  "));
         delay(timeDelay);
         calibrate_scale(); 
       }
       
       delay(timeDelay);
       Serial.println(F("Calibrating..."));
       Serial.println(F("Place 50 gram weight on scale and press button."));
       startTime = millis();
       secondLine = false;
       while (digitalRead(TareCalButton) == HIGH) {  // Wait for 50 gram weight to be placed on track and tare button pressed after.
        lcd.setCursor(0,0);
        lcd.print(F("Calibrating... "));
        if ((millis() <= startTime + timeDelay) && (secondLine == false)) {
          lcd.setCursor(0,1);
          lcd.print(F("Place 50 gram   "));
        }
        else if ((millis() > startTime + timeDelay) && (secondLine == false)) {
          secondLine = true;
          startTime = millis();
        }
        if ((millis() <= startTime + timeDelay) && (secondLine == true)) {
          lcd.setCursor(0,1);
          lcd.print(F("and press button"));
        }
        else if ((millis() > startTime + timeDelay) && (secondLine == true)) {
          secondLine = false;
          startTime = millis();
        }
       }
       
       Serial.println(F("Calibrating now... "));  
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print(F("Calibrating     "));
       lcd.setCursor(0,1);
       lcd.print(F("now...          "));

       // Perform Actual calibration.
       while (WeightScale.update()) {};  // Update scale weight data until smooth.
       WeightScale.refreshDataSet();  
       float CalibrationValue = WeightScale.getNewCalibration(50);  //  Use 50 gram weight.  Change number if a different weight is used for calibration.
       WeightScale.setCalFactor(CalibrationValue);

       delay(timeDelay);
       
       startTime = millis();
       secondLine = false;
       Serial.println(F("Remove 50 gram weight and press button."));
       while (digitalRead(TareCalButton) == HIGH) {  // Wait for 20 gram weight to be removed on track and tare button pressed after.
        lcd.setCursor(0,0);
        lcd.print(F("Calibrating...    "));
        if ((millis() <= startTime + timeDelay) && (secondLine == false)) {
          lcd.setCursor(0,1);
          lcd.print(F("Remove 50 gram  "));
        }
        else if ((millis() > startTime + timeDelay) && (secondLine == false)) {
          secondLine = true;
          startTime = millis();
        }
        if ((millis() <= startTime + timeDelay) && (secondLine == true)) {
          lcd.setCursor(0,1);
          lcd.print(F("and press button"));
        }
        else if ((millis() > startTime + timeDelay) && (secondLine == true)) {
          secondLine = false;
          startTime = millis();
        }
       }
       Serial.println(F("Calibration finishing up..."));
       lcd.setCursor(0,1);
       lcd.print(F("Finishing up...  "));
       delay(timeDelay);

       //Perform Tare Again
       while (WeightScale.update()) {};  // Update scale weight data until smooth.
       WeightScale.refreshDataSet();
       WeightScale.tareNoDelay();
       WeightScale.tare();
       
       Serial.println(F("Calibration complete."));
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print(F("Calibration   "));
       lcd.setCursor(0,1);
       lcd.print(F("complete.     "));
       delay(timeDelay);
       if (initCal == false) {
         lcd.clear();
         lcd.setCursor(0,0);
         lcd.print(F("Startup done..."));
         Serial.println(F("Startup done."));
         initCal = true; // Set startup calibration to true so future calibrations can be cancelled.
         delay(timeDelay);
       }
       ready_print();
       return;
      }

   void get_weight() {
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print(F("Railcar Weight:"));
     lcd.setCursor(0,1);
     delay(500);  // 0.5 second pause to stableize weight reading
     
     //  Get weight on scale
     while (WeightScale.update())  {};  // Update scale weight data until smooth.
     WeightScale.refreshDataSet();
     grams = WeightScale.getData();  // Get smoothed value from the dataset.
     if (grams <= 0) {  //  Used to avoid negitive on display.
      grams = 0;
     }
     float ounces = (grams*(0.035));  
     float pounds = (grams*(3034.5));  //Just for fun, convert N Scale grams to prototype pounds.
     
     Serial.println(F("Railcar Weight:"));
     Serial.print(grams, 1);  //Prints grams with one decimal place.
     Serial.println(F("  grams"));
     Serial.print(ounces, 1);  //Prints ounces with one decimal place.
     Serial.println(F("  ounces"));
     Serial.print(pounds, 1);  //Prints pounds with one decimal place.
     Serial.println(F("  pounds"));
     startTime = millis();
     secondLine = false;
     while (digitalRead(WeightButton) == LOW) {   // Keeps weight on LCD display while weight button is pressed
       if ((millis() <= startTime + timeDelay) && (secondLine == false)) {
         lcd.setCursor(0,1);
         lcd.print(grams, 1);  //Prints grams with one decimal place.
         lcd.print(F(" gr "));
         lcd.setCursor(9,1);
         lcd.print(ounces ,1);  //Prints ounces with one decimal place.
         lcd.print(F(" oz  "));  
       }
       else if ((millis() > startTime + timeDelay) && (secondLine == false)) {
         secondLine = true;
         startTime = millis();
         lcd.setCursor(0,1);
         lcd.print(F("                "));  //Clear the second line for next change.
       }
       if ((millis() <= startTime + timeDelay) && (secondLine == true)) {
         lcd.setCursor(0,1);
         lcd.print(pounds, 1);  //Prints pounds with one decimal place.
         lcd.print(F(" lbs         "));
       }
       else if ((millis() > startTime + timeDelay) && (secondLine == true)) {
         secondLine = false;
         startTime = millis();
         lcd.setCursor(0,1);
         lcd.print(F("                "));  //Clear the second line for next change.
       }
     }
     delay(timeDelay);  // Keep weight on display a few seconds after button released
     ready_print();
     return;
   }

  void ready_print() {  //  Function needed to set screens and variables for start of loop.
    lcd.clear();
    Serial.println(F("Scale Ready... place rail car on track and press button."));
    lcd.setCursor(0,0);
    lcd.print(F("Scale Ready..."));
    boolean TareButtonPress = false;
    boolean SettingsButtonPress = false;
    startTime = millis();
    startButton = millis();
    secondLine = false;
    return;
  }
  
  void loop (){
   
    if ((millis() <= startTime + timeDelay) && (secondLine == false)) {
      lcd.setCursor(0,1);
      lcd.print(F("Place railcar   "));
    }
    else if ((millis() > startTime + timeDelay) && (secondLine == false)) {
      secondLine = true;
      startTime = millis();
    }
    if ((millis() <= startTime + timeDelay) && (secondLine == true)) {
      lcd.setCursor(0,1);
      lcd.print(F("and press button"));
    }
    else if ((millis() > startTime + timeDelay) && (secondLine == true)) {
      secondLine = false;
      startTime = millis();
    }

    //  Read if Tare button is pressed. If pressed and held for 2 to 5 seconds, start Tare function.
    //  If button is pressed for 5 seconds or more start Calibration function.
    //  These timers are to help prevent accidental calling ot the Tare or calibration function by the user
    if ((digitalRead(TareCalButton) == LOW) && (TareButtonPress == false)) { 
      startButton = millis();
      endButton = millis();
      TareButtonPress = true;
      }
    else if ((digitalRead(TareCalButton) == HIGH) && (TareButtonPress == true)) {
      if (endButton < startButton + 2000) {
        TareButtonPress = false; 
      }
      if ((endButton >= startButton + 2000) && (endButton < startButton + 5000)) {  //  Call Tare function if button pressed for over 2 seconds and less than 5 seconds.
        TareButtonPress = false;
        tare_scale();
      }
      else if (endButton >= startButton + 5000) {  //  Call Calibration function if button pressed for over 5 seconds.
        TareButtonPress = false;
        calibrate_scale();
      }
    }

    // Check weight button and start weight function if pressed
    WeightScale.update();  //  Often update weight data in while loop to keep data current
    if (digitalRead(WeightButton) == LOW) {
      get_weight();
    }
    
    //  Set LCD Backlight on and off if settings button pressed for less than 2 seconds
    //  If pressed for more than 2 seconds cycle through setting 1.5, 3 and 5 seconds LCD line pause settings.
    if ((digitalRead(SettingsButton) == LOW) && (SettingsButtonPress == false)) { 
      startButton = millis();
      endButton = millis();
      SettingsButtonPress = true;
      }
    else if ((digitalRead(SettingsButton) == HIGH) && (SettingsButtonPress == true)) {
      if (endButton < startButton + 2000) {  // Turn on or off LCD backlight if button pressed for less than 2 seconds
        SettingsButtonPress = false;
        if (backlightOn == true) {  
          lcd.noBacklight();
          backlightOn = false;
          delay(500);
        }
        else if (backlightOn == false) {
          lcd.backlight();
          backlightOn = true;
          delay(500);
        } 
      }
      if ((endButton >= startButton + 2000) && (endButton < startButton + 5000) ) {  //  Cycle through the three LCD display pause times if settings button is pressed for more than 2 seconds
        lcd.clear();
        Serial.println(F("LCD second line pause set to:"));
        lcd.setCursor(0,0);
        lcd.print(F("LCD time set to:"));
        if ((timeDelay == 5000) && (SettingsButtonPress == true)) {
          SettingsButtonPress = false;
          timeDelay = 1500;
          Serial.println(F("1.5 seconds  "));
          lcd.setCursor(0,1);
          lcd.print(F("1.5 seconds    "));
          delay(timeDelay);
          ready_print();
        }
        if ((timeDelay == 3000) && (SettingsButtonPress == true)) {
          SettingsButtonPress = false;
          timeDelay = 5000;
          Serial.println(F("5 seconds"));
          lcd.setCursor(0,1);
          lcd.print(F("5 seconds     "));
          delay(timeDelay);
          ready_print();
        }
        if ((timeDelay == 1500) && (SettingsButtonPress == true)) {
          SettingsButtonPress = false;
          timeDelay = 3000;
          Serial.println(F("3 seconds"));
          lcd.setCursor(0,1);
          lcd.print(F("3 seconds     "));
          delay(timeDelay);
          ready_print();
        }
        
      }
      if (endButton >= startButton + 5000) {  // Possible future routine for settings button if pressed for 5 seconds
        SettingsButtonPress = false;
      }

    }
    WeightScale.update();  //  Often update weight data in loop to keep data current
    endButton = millis();  //  Always set endButton time in the main while loop for button beening pressed and held.
  }
  
