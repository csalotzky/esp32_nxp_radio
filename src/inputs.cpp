#include "inputs.h"

//#define I2CADDR         0x20

//i2ckeypad kpd = i2ckeypad(I2CADDR, 4, 4);

// Gets given rotary encoders knob state
// https://lastminuteengineers.com/rotary-encoder-arduino-tutorial/
void GetRotary(Rotary *rot, bool *up, bool *down) {
	// Read the current state of CLK
    uint8_t currentStateClk = digitalRead(rot->Clk);

    // If last and current state of CLK are different, then pulse occurred
    // React to only 1 state change to avoid double count
    if (LOW == rot->lastStateClk && currentStateClk == 1){

        // If the DT state is different than the CLK state then
        // the encoder is rotating CCW so decrement
        if (digitalRead(rot->Dt) == LOW) {
            *down = true;
        } else {
            *up = true;
        }
    }

    // Remember last CLK state
    rot->lastStateClk=currentStateClk;
}

// Setup method of this class
void InputsInit(Gui *gui, Radio *radio){
    guiRef = gui;
    radioRef = radio;
    Wire.begin();
    //kpd.init();
    InterfaceInit(gui,radio);

    // Initial states of rotary encoders
    #ifdef TUNE_ROTARY
    rotVol.lastStateClk = digitalRead(rotVol.Clk);
    pinMode(ROT1_CLK,INPUT);
	pinMode(ROT1_DT,INPUT);

    #endif

    #ifdef VOL_ROTARY
    rotTune.lastStateClk = digitalRead(rotTune.Clk);
    pinMode(ROT2_CLK,INPUT);
	pinMode(ROT2_DT,INPUT);

    #endif  

    // Programmable keys - registers button callback based on config's keymap
    for (size_t i = 0; i < TOTAL_BUTTONS; i++)
    {
        // Hw buttons
        KeyMap tmp = radioRef->config.KeyMaps[i];
        for (size_t j = 0; j < HwButtonsSize; j++)
        {
            if (!tmp.LongPress && tmp.Key == HwButtons[j].KeyId) HwButtons[j].Btn.onPressed(Commands[tmp.CmdIndex].Fn);
            if (tmp.LongPress && tmp.Key == HwButtons[j].KeyId)
            {
                if (Commands[tmp.CmdIndex].RepeatableExecute) HwButtons[j].RepeatableLongFn = Commands[tmp.CmdIndex].Fn;
                else HwButtons[j].Btn.onPressedFor(1000,Commands[tmp.CmdIndex].Fn);
            } 

        }

        // Matrix buttons
        for (size_t j = 0; j < VirtButtonsMatrixSize; j++)
        {
            if (!tmp.LongPress && tmp.Key == VirtButtonsMatrix[j].KeyId) VirtButtonsMatrix[j].Btn.onPressed(Commands[tmp.CmdIndex].Fn);
            if (tmp.LongPress && tmp.Key == VirtButtonsMatrix[j].KeyId)
            {
                if (Commands[tmp.CmdIndex].RepeatableExecute) VirtButtonsMatrix[j].RepeatableLongFn = Commands[tmp.CmdIndex].Fn;
                else VirtButtonsMatrix[j].Btn.onPressedFor(1000,Commands[tmp.CmdIndex].Fn);
            } 
        }

        // Rotary encoders
        for (size_t j = 0; j < VirtButtonsRotarySize; j++)
        {
            if (!tmp.LongPress && tmp.Key == VirtButtonsRotary[j].KeyId) VirtButtonsRotary[j].Btn.onPressed(Commands[tmp.CmdIndex].Fn);
            //if (tmp.LongPress && tmp.Key == VirtButtonsRotary[j].KeyId) VirtButtonsRotary[j].Btn.onPressedFor(1000,Commands[tmp.CmdIndex].Fn);
        }        
    }

    // Hardcoded keys - overrides keymap values
    // todoooo
    //VirtButtonsMatrix[VirtButtonsMatrixSize-1].Btn.onPressed(ShortNumCallback);
    //VirtButtonsMatrix[VirtButtonsMatrixSize-1].Btn.onPressedFor(1000,LongNumCallback);
    for (size_t i = 0; i < HwButtonsSize; i++) {
        if (HwButtons[i].KeyId == DEFAULT_KEY_OK) HwButtons[i].Btn.onPressed(Ok);
        if (HwButtons[i].KeyId == DEFAULT_KEY_BACK) HwButtons[i].Btn.onPressed(Back);
        if (HwButtons[i].KeyId == DEFAULT_KEY_LEFT) HwButtons[i].Btn.onPressed(MenuLeft);
        if (HwButtons[i].KeyId == DEFAULT_KEY_RIGHT) HwButtons[i].Btn.onPressed(MenuRight);
        if (HwButtons[i].KeyId == DEFAULT_KEY_UP) HwButtons[i].Btn.onPressed(MenuUp);
        if (HwButtons[i].KeyId == DEFAULT_KEY_DOWN) HwButtons[i].Btn.onPressed(MenuDown);
        if (HwButtons[i].KeyId == DEFAULT_KEY_PWR_TGL) HwButtons[i].Btn.onPressedFor(100,PowerOff);
        if (HwButtons[i].KeyId == DEFAULT_KEY_PWR_BTN) HwButtons[i].Btn.onPressed(PowerSwitch);
    }


    // Init keys!!
    for (size_t i = 0; i < HwButtonsSize; i++)
    {
        HwButtons[i].Btn.begin();
        delay(5);
        // EasyButton bug, todo(?)
        HwButtons[i].Btn.begin();
    }

    for (size_t i = 0; i < VirtButtonsMatrixSize; i++)
    {
        VirtButtonsMatrix[i].Btn.begin();
    }

    for (size_t i = 0; i < VirtButtonsRotarySize; i++)
    {
        VirtButtonsRotary[i].Btn.begin();
    }

    // Tasks
    #if defined(TUNE_ROTARY) || defined(VOL_ROTARY) || defined(NAVI_BUTTONS) || defined(POWER_SWITCH)
    xTaskCreate(ButtonLoop,"Read buttons and rotary",8192,NULL,0,NULL);
    #endif

    #ifdef I2C_KEYPAD
    xTaskCreate(KeypadLoop,"Read Keypad",8192,NULL,0,NULL);
    #endif
}
  

// TODO
/*
void KeypadLoop(void* params){
    while(true) {
           
        bool error = false;

        if(i2cSemaphore != NULL ) {
            if(xSemaphoreTake(i2cSemaphore, (TickType_t) 100) == pdTRUE) {

            }
            else {
                error = true;
            }
        } 
        else {
            error = true;
        }

        if(error) Serial.println("INPUTS | ERROR While fetching keypad button key");
        
   
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
*/

// Read all button state - except matrix keypad
void ButtonLoop(void* params) {
    while(true) {
        #ifdef TUNE_ROTARY
        state_KEY_ROT_ROT1_UP=0, state_KEY_ROT_ROT1_DOWN=0;
        GetRotary(&rotTune,&state_KEY_ROT_ROT1_UP,&state_KEY_ROT_ROT1_DOWN);

        #endif
        #ifdef VOL_ROTARY
        state_KEY_ROT_ROT2_UP=0, state_KEY_ROT_ROT2_DOWN=0;
        GetRotary(&rotVol,&state_KEY_ROT_ROT2_UP,&state_KEY_ROT_ROT2_DOWN);
        #endif     

        for (size_t i = 0; i < VirtButtonsRotarySize; i++) VirtButtonsRotary[i].Btn.read();
        for (size_t i = 0; i < HwButtonsSize; i++)
        {
            if (HwButtons[i].Btn.read() && HwButtons[i].RepeatableLongFn != NULL && HwButtons[i].Btn.pressedFor(1000)) {
                HwButtons[i].RepeatableLongFn();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        } 
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}