#include <EepromUtil.h>
#include <SerialUI.h>
#include <EEPROM.h>
#include <Button.h>

#define serial_baud_rate           57600
#define serial_input_terminator   '\n'

#define ledpin  13

enum PinAssignments {
  encoderPinA = 3,
  encoderPinB = 2,
  fastButtonPin = 7
};

volatile unsigned int encoderPos = 0;
unsigned int lastReportedPos = 1;

boolean A_set = false;
boolean B_set = false;
boolean isFastModeEnabled = false;

Button fastButton = Button(fastButtonPin, PULLUP);
Button macro1Button = Button(8, PULLUP);
Button macro2Button = Button(9, PULLUP);
Button macro3Button = Button(10, PULLUP);
Button macro4Button = Button(11, PULLUP);
Button macro5Button = Button(12, PULLUP);
Button macro6Button = Button(13, PULLUP);

SUI_DeclareString(device_greeting, "\n\n+++ Welcome to the M1DST Remote Knob Config +++\r\nEnter '?' to list available options.");
SUI_DeclareString(top_menu_title, "Main Menu");

SUI_DeclareString(config_key, "config");
SUI_DeclareString(config_menu_title, "Config Menu");
SUI_DeclareString(config_help, "Configure the macros or baud rate.");

SUI_DeclareString(settings_title, "Knob Settings");
SUI_DeclareString(settings_key, "settings");
SUI_DeclareString(settings_help, "Perform setup and config");

SUI_DeclareString(status_key, "status");
SUI_DeclareString(status_help, "Shows the current macro settings");

SUI_DeclareString(config_help_key,"help");
SUI_DeclareString(config_help_help,"Shows instructions on how to configure the macros.");

SUI_DeclareString(config_callsign_key,"callsign");
SUI_DeclareString(config_callsign_help,"Please enter your callsign");

SUI_DeclareString(factory_reset_key,"reset");
SUI_DeclareString(factory_reset_help,"Resets the config to factory settings.");

SUI_DeclareString(config_button1_key,"btn1");
SUI_DeclareString(config_button2_key,"btn2");
SUI_DeclareString(config_button3_key,"btn3");
SUI_DeclareString(config_button4_key,"btn4");
SUI_DeclareString(config_button5_key,"btn5");
SUI_DeclareString(config_button6_key,"btn6");
SUI_DeclareString(config_button1_help,"Set the macro sent when button 1 is pressed.");
SUI_DeclareString(config_button2_help,"Set the macro sent when button 2 is pressed.");
SUI_DeclareString(config_button3_help,"Set the macro sent when button 3 is pressed.");
SUI_DeclareString(config_button4_help,"Set the macro sent when button 4 is pressed.");
SUI_DeclareString(config_button5_help,"Set the macro sent when button 5 is pressed.");
SUI_DeclareString(config_button6_help,"Set the macro sent when button 6 is pressed.");

SUI_DeclareString(config_knob_cw_key,"cw");
SUI_DeclareString(config_knob_ccw_key,"ccw");
SUI_DeclareString(config_knob_cwf_key,"cwf");
SUI_DeclareString(config_knob_ccwf_key,"ccwf");
SUI_DeclareString(config_knob_help,"Set the macro sent when knob turned.");
SUI_DeclareString(config_knobcw_help,"Set the macro sent when knob turned clockwise.");
SUI_DeclareString(config_knobccw_help,"Set the macro sent when knob turned counter clockwise.");
SUI_DeclareString(config_knobcwf_help,"Set the macro sent when knob turned clockwise (in fast mode).");
SUI_DeclareString(config_knobccwf_help,"Set the macro sent when knob turned counter clockwise (in fast mode).");

SUI_DeclareString(config_instructions_line_1, "\n\n*** Instructions ***\n");
SUI_DeclareString(config_instructions_line_2, "All commands are case sensitive, always use lowercase for commands.");
SUI_DeclareString(config_instructions_line_3, "Macro text can be any case.  Most radios ignore case of control commands.");

SUI_DeclareString(settings_show_key, "show");

SUI_DeclareString(resetting_to_factory_defaults, "Resetting to factory defaults...");
SUI_DeclareString(saving_eeprom, "Saving to the eeprom...");

SUI::SerialUI mySUI = SUI::SerialUI(device_greeting);

// We'll also define a struct to hold our macros.

// button_macro_maxlen, max string length for the macros stored within the device when a button is pressed:
#define button_macro_maxlen  100

// knob_macro_maxlen, max string length for the macros stored within the device when the knob is rotated:
#define knob_macro_maxlen  20

// knobInfo struct, a place to store our settings
typedef struct knobInfo {

  char callsign[knob_macro_maxlen + 1];

  char knob_cw[knob_macro_maxlen + 1];
  char knob_ccw[knob_macro_maxlen + 1];
  char knob_cw_alt[knob_macro_maxlen + 1];
  char knob_ccw_alt[knob_macro_maxlen + 1];

  char button1[button_macro_maxlen + 1];
  char button2[button_macro_maxlen + 1];
  char button3[button_macro_maxlen + 1];
  char button4[button_macro_maxlen + 1];
  char button5[button_macro_maxlen + 1];
  char button6[button_macro_maxlen + 1];
} 
knobInfo;

// Just declare a global knobInfo structure for use below, initialized to all-zeros:
knobInfo myKnob = {
  0};



/*
** ********************* setup() *********************** 
 **
 ** The standard Arduino setup() function.  Here we'll
 ** setup serial comm and the menu structure.
 */
void setup() 
{

  mySUI.begin(serial_baud_rate); // serial line open/setup
  mySUI.setTimeout(20000);      // timeout for reads (in ms), same as for Serial.
  mySUI.setMaxIdleMs(30000);    // timeout for user (in ms)

  // how we are marking the "end-of-line" (\n, by default):
  mySUI.setReadTerminator(serial_input_terminator);  

  setupMenus();

  // set our blinker pin as an output.
  pinMode(ledpin, OUTPUT);

  // Populate the structure with the values stored from within eeprom.
  load_from_eeprom();

  pinMode(encoderPinA, INPUT); 
  pinMode(encoderPinB, INPUT); 

  digitalWrite(encoderPinA, HIGH);  // turn on pullup resistor
  digitalWrite(encoderPinB, HIGH);  // turn on pullup resistor

  attachInterrupt(0, doEncoderA, CHANGE);
  attachInterrupt(1, doEncoderB, CHANGE);

}

void loop()
{

  if (mySUI.checkForUser(150))
  {
    mySUI.enter();

    while (mySUI.userPresent())
    {
      mySUI.handleRequests();
    }

  }
  
  if(fastButton.uniquePress()){
    isFastModeEnabled = !isFastModeEnabled;
    digitalWrite(ledpin, isFastModeEnabled);
  }

  if(macro1Button.uniquePress()){
    mySUI.println(myKnob.button1);
  }
  if(macro2Button.uniquePress()){
    mySUI.println(myKnob.button2);
  }
  if(macro3Button.uniquePress()){
    mySUI.println(myKnob.button3);
  }
  if(macro4Button.uniquePress()){
    mySUI.println(myKnob.button4);
  }
  if(macro5Button.uniquePress()){
    mySUI.println(myKnob.button5);
  }
  if(macro6Button.uniquePress()){
    mySUI.println(myKnob.button6);
  }

}


void set_button1_macro()
{
  mySUI.showEnterDataPrompt();
  byte numRead = mySUI.readBytesToEOL(myKnob.button1, button_macro_maxlen);
  myKnob.button1[numRead] = '\0';
  save_button1_to_eeprom();
  mySUI.print("\nBTN1 : ");
  mySUI.println(myKnob.button1);
  mySUI.returnOK();
  mySUI.println();
}
void set_button2_macro()
{
  mySUI.showEnterDataPrompt();
  byte numRead = mySUI.readBytesToEOL(myKnob.button2, button_macro_maxlen);
  myKnob.button2[numRead] = '\0';
  save_button2_to_eeprom();
  mySUI.print("\nBTN2 : ");
  mySUI.println(myKnob.button2);
  mySUI.returnOK();
  mySUI.println();
}

void set_button3_macro()
{
  mySUI.showEnterDataPrompt();
  byte numRead = mySUI.readBytesToEOL(myKnob.button3, button_macro_maxlen);
  myKnob.button3[numRead] = '\0';
  save_button3_to_eeprom();
  mySUI.print("\nBTN3 : ");
  mySUI.println(myKnob.button3);
  mySUI.returnOK();
  mySUI.println();
}

void set_button4_macro()
{
  mySUI.showEnterDataPrompt();
  byte numRead = mySUI.readBytesToEOL(myKnob.button4, button_macro_maxlen);
  myKnob.button4[numRead] = '\0';
  save_button4_to_eeprom();
  mySUI.print("\nBTN4 : ");
  mySUI.println(myKnob.button4);
  mySUI.returnOK();
  mySUI.println();
}

void set_button5_macro()
{
  mySUI.showEnterDataPrompt();
  byte numRead = mySUI.readBytesToEOL(myKnob.button5, button_macro_maxlen);
  myKnob.button5[numRead] = '\0';
  save_button5_to_eeprom();
  mySUI.print("\nBTN5 : ");
  mySUI.println(myKnob.button5);
  mySUI.returnOK();
  mySUI.println();
}

void set_button6_macro()
{
  mySUI.showEnterDataPrompt();
  byte numRead = mySUI.readBytesToEOL(myKnob.button6, button_macro_maxlen);
  myKnob.button6[numRead] = '\0';
  save_button6_to_eeprom();
  mySUI.print("\nBTN6 : ");
  mySUI.println(myKnob.button6);
  mySUI.returnOK();
  mySUI.println();
}

void set_callsign()
{
  mySUI.showEnterDataPrompt();
  byte numRead = mySUI.readBytesToEOL(myKnob.callsign, knob_macro_maxlen);
  myKnob.callsign[numRead] = '\0';

  // Loop around each character of the string and uppercase it if appropriate.
  for (int i = 0; i < knob_macro_maxlen + 1; i++ )
  {
    if(myKnob.callsign[i] >='a' && myKnob.callsign[i] <='z') myKnob.callsign[i] -= 32;
  }

  save_callsign_to_eeprom();

  mySUI.print("\nCallsign : ");
  mySUI.println(myKnob.callsign);
  mySUI.returnOK();
  mySUI.println();
}

void show_info()
{
  mySUI.println("\n\n** Button Configuration **");  
  mySUI.print("BTN1  : ");
  mySUI.println(myKnob.button1);  
  mySUI.print("BTN2  : ");
  mySUI.println(myKnob.button2);  
  mySUI.print("BTN3  : ");
  mySUI.println(myKnob.button3);  
  mySUI.print("BTN4  : ");
  mySUI.println(myKnob.button4);  
  mySUI.print("BTN5  : ");
  mySUI.println(myKnob.button5);  
  mySUI.print("BTN6  : ");
  mySUI.println(myKnob.button6);  

  mySUI.println("\n** Knob Configuration **");  
  mySUI.print("CW   : ");
  mySUI.println(myKnob.knob_cw);  
  mySUI.print("CCW  : ");
  mySUI.println(myKnob.knob_ccw);  
  mySUI.print("CCF  : ");
  mySUI.println(myKnob.knob_cw_alt);  
  mySUI.print("CCWF : ");
  mySUI.println(myKnob.knob_ccw_alt);  

  mySUI.print("\nCallsign : ");
  mySUI.println(myKnob.callsign); 
  mySUI.println();

}

void setupMenus()
{

  SUI::Menu * mainMenu = mySUI.topLevelMenu();

  if (!mainMenu)
  {
    return mySUI.returnError("Something is very wrong, could not get top level menu?");
  }

  mainMenu->setName(top_menu_title);

  mainMenu->addCommand(status_key, show_info, status_help);

  SUI::Menu * configMenu = mainMenu->subMenu(config_key, config_help);
  if (!configMenu)
  {
    return mySUI.returnError("Couldn't create config menu!?");
  } 
  configMenu->setName(config_menu_title);

  configMenu->addCommand(config_help_key, showConfigHelp);
  configMenu->addCommand(config_callsign_key, set_callsign, config_callsign_help);
  configMenu->addCommand(config_button1_key, set_button1_macro, config_button1_help);
  configMenu->addCommand(config_button2_key, set_button2_macro, config_button2_help);
  configMenu->addCommand(config_button3_key, set_button3_macro, config_button3_help);
  configMenu->addCommand(config_button4_key, set_button4_macro, config_button4_help);
  configMenu->addCommand(config_button5_key, set_button5_macro, config_button5_help);
  configMenu->addCommand(config_button6_key, set_button6_macro, config_button6_help);

  configMenu->addCommand(config_knob_cw_key, set_callsign, config_knobcw_help);
  configMenu->addCommand(config_knob_ccw_key, set_callsign, config_knobccw_help);
  configMenu->addCommand(config_knob_cwf_key, set_callsign, config_knobcwf_help);
  configMenu->addCommand(config_knob_ccwf_key, set_callsign, config_knobccwf_help);

  mainMenu->addCommand(factory_reset_key, reset_and_save, factory_reset_help);

}

void reset_and_save()
{
  mySUI.println();
  mySUI.println_P(resetting_to_factory_defaults); 
  reset_to_factory_defaults();
  mySUI.println_P(saving_eeprom); 
  save_to_eeprom();
  mySUI.returnOK();
  mySUI.println();
}

void reset_to_factory_defaults()
{
  strcpy(myKnob.callsign, "N0CALL" + '\0');

  strcpy(myKnob.knob_cw, "UP" + '\0');
  strcpy(myKnob.knob_ccw, "DN" + '\0');
  strcpy(myKnob.knob_cw_alt, "UP4" + '\0');
  strcpy(myKnob.knob_ccw_alt, "DN4" + '\0');

  // Useful macros found from http://www.pa1m.nl/pa1m/elecraft-k3/

  // Button 1 - Instant CW split
  // A handy way of instantly dropping the K3 straight into split mode in a pileup 
  // is to use the K3’s fancy new switch macro facility.  It avoids the need to press 
  // a whole set of buttons in the correct sequence when the DX is calling “up 1” (or whatever)
  // and the adrenalin is flowing fast.
  strcpy(myKnob.button1, "SWT13;SWT13;UPB4;FT1;DV0;SB1;RT0;XT0;LK1;BW0060;BW$0270;MN111;MP001;MN255;" + '\0');  

  // Button 2 - Instant CW pileup
  // I have programmed another macro key to do the converse of the “instant split”, enabling me to instantly 
  // start listening ‘up 1’ when I get a CW pileup going and need to split.  In this case, I use the main VFO A 
  // to tune around the pileup, transmitting on a locked VFO B.
  strcpy(myKnob.button2, "SWT13;SWT13;UP4;FT1;DV0;SB1;RT0;XT0;LK$1;BW0270;BW$0060;MN111;MP002;MN255;" + '\0');  

  // Button 3 - Unsplit
  // Having used either of the above split macros, I wanted a further macro to reset the K3 quickly to my normal operating setup.
  strcpy(myKnob.button3, "FT0; LK0; LK$0; SWT13; BW0270; DV1;" + '\0');  

  // Button 4 - Sends CW
  // Sends M1DST via CW
  strcpy(myKnob.button4, "KY M1DST" + '\0');

  // Button 5 - Sends CW
  // Sends 5NN via CW
  strcpy(myKnob.button5, "KY 5NN" + '\0');

  // Button 6 - Toggle headphones / speakers
  // Since the Speaker+phone menu entry only has two states this will toggle speakers on/off.
  strcpy(myKnob.button6, "MN097;UP;MN255;" + '\0');
}

void save_callsign_to_eeprom()
{
  EepromUtil::eeprom_write_string(0 * knob_macro_maxlen, myKnob.callsign);
}

void save_button1_to_eeprom()
{
  EepromUtil::eeprom_write_string((4 * knob_macro_maxlen) + (1 * button_macro_maxlen), myKnob.button1);
}

void save_button2_to_eeprom()
{
  EepromUtil::eeprom_write_string((4 * knob_macro_maxlen) + (2 * button_macro_maxlen), myKnob.button2);
}

void save_button3_to_eeprom()
{
  EepromUtil::eeprom_write_string((4 * knob_macro_maxlen) + (3 * button_macro_maxlen), myKnob.button3);
}


void save_button4_to_eeprom()
{
  EepromUtil::eeprom_write_string((4 * knob_macro_maxlen) + (4 * button_macro_maxlen), myKnob.button4);
}


void save_button5_to_eeprom()
{
  EepromUtil::eeprom_write_string((4 * knob_macro_maxlen) + (5 * button_macro_maxlen), myKnob.button5);
}


void save_button6_to_eeprom()
{
  EepromUtil::eeprom_write_string((4 * knob_macro_maxlen) + (6 * button_macro_maxlen), myKnob.button6);
}



void save_to_eeprom()
{
  EepromUtil::eeprom_erase_all();

  save_callsign_to_eeprom();

  EepromUtil::eeprom_write_string(1 * knob_macro_maxlen, myKnob.knob_cw);
  EepromUtil::eeprom_write_string(2 * knob_macro_maxlen, myKnob.knob_ccw);
  EepromUtil::eeprom_write_string(3 * knob_macro_maxlen, myKnob.knob_cw_alt);
  EepromUtil::eeprom_write_string(4 * knob_macro_maxlen, myKnob.knob_ccw_alt);

  save_button1_to_eeprom();
  save_button2_to_eeprom();
  save_button3_to_eeprom();
  save_button4_to_eeprom();
  save_button5_to_eeprom();
  save_button6_to_eeprom();

}

void load_from_eeprom()
{
  EepromUtil::eeprom_read_string(0 * knob_macro_maxlen, myKnob.callsign, knob_macro_maxlen + 1);

  EepromUtil::eeprom_read_string(1 * knob_macro_maxlen, myKnob.knob_cw, knob_macro_maxlen + 1);
  EepromUtil::eeprom_read_string(2 * knob_macro_maxlen, myKnob.knob_ccw, knob_macro_maxlen + 1);
  EepromUtil::eeprom_read_string(3 * knob_macro_maxlen, myKnob.knob_cw_alt, knob_macro_maxlen + 1);
  EepromUtil::eeprom_read_string(4 * knob_macro_maxlen, myKnob.knob_ccw_alt, knob_macro_maxlen + 1);

  EepromUtil::eeprom_read_string((4 * knob_macro_maxlen) + (1 * button_macro_maxlen), myKnob.button1, button_macro_maxlen + 1);
  EepromUtil::eeprom_read_string((4 * knob_macro_maxlen) + (2 * button_macro_maxlen), myKnob.button2, button_macro_maxlen + 1);
  EepromUtil::eeprom_read_string((4 * knob_macro_maxlen) + (3 * button_macro_maxlen), myKnob.button3, button_macro_maxlen + 1);
  EepromUtil::eeprom_read_string((4 * knob_macro_maxlen) + (4 * button_macro_maxlen), myKnob.button4, button_macro_maxlen + 1);
  EepromUtil::eeprom_read_string((4 * knob_macro_maxlen) + (5 * button_macro_maxlen), myKnob.button5, button_macro_maxlen + 1);
  EepromUtil::eeprom_read_string((4 * knob_macro_maxlen) + (6 * button_macro_maxlen), myKnob.button6, button_macro_maxlen + 1);

}

void showConfigHelp()
{
  mySUI.println_P(config_instructions_line_1); 
  mySUI.println_P(config_instructions_line_2); 
  mySUI.println_P(config_instructions_line_3); 
  mySUI.println();
}

// Interrupt on A changing state
void doEncoderA(){
  A_set = digitalRead(encoderPinA) == HIGH;
  if(A_set != B_set)
  {
    mySUI.println((isFastModeEnabled)? myKnob.knob_cw_alt : myKnob.knob_cw);
  }
  else
  {
    mySUI.println((isFastModeEnabled)? myKnob.knob_ccw_alt : myKnob.knob_ccw);
  }
  encoderPos += (A_set != B_set) ? +1 : -1;
}

// Interrupt on B changing state
void doEncoderB(){
  B_set = digitalRead(encoderPinB) == HIGH;

  if(A_set == B_set)
  {
    mySUI.println((isFastModeEnabled)? myKnob.knob_cw_alt : myKnob.knob_cw);
  }
  else
  {
    mySUI.println((isFastModeEnabled)? myKnob.knob_ccw_alt : myKnob.knob_ccw);
  }

  encoderPos += (A_set == B_set) ? +1 : -1;
}











