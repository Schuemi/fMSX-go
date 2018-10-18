/*
 * The MIT License
 *
 * Copyright 2018 Schuemi.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "LibOdroidGo.h"


#include "FDIDisk.h"
#include <stddef.h>
#include <stdio.h>
#include "MSX.h"
#include "EMULib.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "odroid_display.h"
#include "odroid_input.h"
#include "odroid_qwerty.h"
#include <esp_heap_caps.h>
#include "sound.h"
#include "odroid_audio.h"
#include "ff.h"

#include "minIni.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "utils.h"


int keyMapping[ODROID_INPUT_MAX];
char pushedKeys[ODROID_INPUT_MAX];
char* lastGame;
int pushedVirtKeyboardKey = -1;
int holdVirtKeyboardKey = -1;
int holdVirtKeyboardSelectKey = -1;
uint8_t inMenue = 0;
uint8_t vKeyboardShow = 0;
/// for the Menu
unsigned int lastKey = ODROID_INPUT_MENU;
unsigned int pressCounter = 0;
///////////
bool odroidQwertyFound = false;
odroid_qwerty_state odroidQwertyPushedKeys;
char holdShift = 0;

void setDefaultKeymapping() {
    keyMapping[ODROID_INPUT_UP] = JST_UP;
    keyMapping[ODROID_INPUT_RIGHT] = JST_RIGHT;
    keyMapping[ODROID_INPUT_DOWN] = JST_DOWN;
    keyMapping[ODROID_INPUT_LEFT] = JST_LEFT;
    keyMapping[ODROID_INPUT_SELECT] = '2' << 8;
    keyMapping[ODROID_INPUT_START] = '1' << 8;
    keyMapping[ODROID_INPUT_A] = JST_FIREA;
    keyMapping[ODROID_INPUT_B] = JST_FIREB;
    keyMapping[ODROID_INPUT_MENU] = '|' << 8;
    keyMapping[ODROID_INPUT_VOLUME] = '~' << 8;
    
}

void SetKeyMapping(int Key, char* mappingString) {
    keyMapping[Key] = -1;
    
    if (!strcmp(mappingString, "JST_UP")) keyMapping[Key] = JST_UP;
    if (!strcmp(mappingString, "JST_RIGHT")) keyMapping[Key] = JST_RIGHT;
    if (!strcmp(mappingString, "JST_DOWN")) keyMapping[Key] = JST_DOWN;
    if (!strcmp(mappingString, "JST_LEFT")) keyMapping[Key] = JST_LEFT;
    if (!strcmp(mappingString, "JST_FIREA")) keyMapping[Key] = JST_FIREA;
    if (!strcmp(mappingString, "JST_FIREB")) keyMapping[Key] = JST_FIREB;
    
    if (!strcmp(mappingString, "KBD_SPACE")) keyMapping[Key] = KBD_SPACE << 8; 
    
    if (!strcmp(mappingString, "KBD_F1")) keyMapping[Key] = KBD_F1 << 8; 
    if (!strcmp(mappingString, "KBD_F2")) keyMapping[Key] = KBD_F2 << 8; 
    if (!strcmp(mappingString, "KBD_F3")) keyMapping[Key] = KBD_F3 << 8; 
    if (!strcmp(mappingString, "KBD_F4")) keyMapping[Key] = KBD_F4 << 8; 
    if (!strcmp(mappingString, "KBD_F5")) keyMapping[Key] = KBD_F5 << 8; 
    
    if (!strcmp(mappingString, "KBD_LEFT")) keyMapping[Key] = KBD_LEFT << 8; 
    if (!strcmp(mappingString, "KBD_UP")) keyMapping[Key] = KBD_UP << 8; 
    if (!strcmp(mappingString, "KBD_DOWN")) keyMapping[Key] = KBD_DOWN << 8; 
    if (!strcmp(mappingString, "KBD_RIGHT")) keyMapping[Key] = KBD_RIGHT << 8; 
    if (!strcmp(mappingString, "KBD_SHIFT")) keyMapping[Key] = KBD_SHIFT << 8; 
    if (!strcmp(mappingString, "KBD_CONTROL")) keyMapping[Key] = KBD_CONTROL << 8; 
    if (!strcmp(mappingString, "KBD_GRAPH")) keyMapping[Key] = KBD_GRAPH << 8; 
    if (!strcmp(mappingString, "KBD_BS")) keyMapping[Key] = KBD_BS << 8; 
    if (!strcmp(mappingString, "KBD_TAB")) keyMapping[Key] = KBD_TAB << 8; 
    if (!strcmp(mappingString, "KBD_CAPSLOCK")) keyMapping[Key] = KBD_CAPSLOCK << 8; 
    if (!strcmp(mappingString, "KBD_SELECT")) keyMapping[Key] = KBD_SELECT << 8; 
    if (!strcmp(mappingString, "KBD_HOME")) keyMapping[Key] = KBD_HOME << 8; 
    if (!strcmp(mappingString, "KBD_ENTER")) keyMapping[Key] = KBD_ENTER << 8; 
    if (!strcmp(mappingString, "KBD_INSERT")) keyMapping[Key] = KBD_INSERT << 8; 
    if (!strcmp(mappingString, "KBD_COUNTRY")) keyMapping[Key] = KBD_COUNTRY << 8; 
    if (!strcmp(mappingString, "KBD_STOP")) keyMapping[Key] = KBD_STOP << 8; 
    
    if (!strcmp(mappingString, "KBD_NUMPAD0")) keyMapping[Key] = KBD_NUMPAD0 << 8; 
    if (!strcmp(mappingString, "KBD_NUMPAD1")) keyMapping[Key] = KBD_NUMPAD1 << 8; 
    if (!strcmp(mappingString, "KBD_NUMPAD2")) keyMapping[Key] = KBD_NUMPAD2 << 8; 
    if (!strcmp(mappingString, "KBD_NUMPAD3")) keyMapping[Key] = KBD_NUMPAD3 << 8; 
    if (!strcmp(mappingString, "KBD_NUMPAD4")) keyMapping[Key] = KBD_NUMPAD4 << 8; 
    if (!strcmp(mappingString, "KBD_NUMPAD5")) keyMapping[Key] = KBD_NUMPAD5 << 8; 
    if (!strcmp(mappingString, "KBD_NUMPAD6")) keyMapping[Key] = KBD_NUMPAD6 << 8; 
    if (!strcmp(mappingString, "KBD_NUMPAD7")) keyMapping[Key] = KBD_NUMPAD7 << 8; 
    if (!strcmp(mappingString, "KBD_NUMPAD8")) keyMapping[Key] = KBD_NUMPAD8 << 8; 
    if (!strcmp(mappingString, "KBD_NUMPAD9")) keyMapping[Key] = KBD_NUMPAD9 << 8; 
    
    if (!strcmp(mappingString, "KBD_ESCAPE")) keyMapping[Key] = KBD_ESCAPE << 8; 
    
    
    
    if (keyMapping[Key] == -1) {
        keyMapping[Key] = mappingString[0] << 8;
    }
}
char LoadKeyMapping(char* KeyFile) {
   
    char buffer[16];
    int res;

    res = ini_gets("KEYMAPPING", "UP", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_UP, buffer);
    
    res = ini_gets("KEYMAPPING", "RIGHT", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_RIGHT, buffer);
    
    res = ini_gets("KEYMAPPING", "DOWN", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_DOWN, buffer);
    
    res = ini_gets("KEYMAPPING", "LEFT", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_LEFT, buffer);
    
    res = ini_gets("KEYMAPPING", "SELECT", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_SELECT, buffer);
    
    res = ini_gets("KEYMAPPING", "START", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_START, buffer);
    
    res = ini_gets("KEYMAPPING", "A", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_A, buffer);
    
    res = ini_gets("KEYMAPPING", "B", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_B, buffer);
    
      
    
    
    
}


void loadKeyMappingFromGame(const char* gameFileName) {
    const char* filename = getFileName(gameFileName);
    char* keyBoardFile = malloc(256);
    char* keyBoardFileFullPath = malloc(612);
    strncpy(keyBoardFile, filename, 256);
    keyBoardFile = cutExtension(keyBoardFile);
    snprintf(keyBoardFileFullPath, 612, "/sd/odroid/data/msx/%s.ini", keyBoardFile);
    LoadKeyMapping(keyBoardFileFullPath);
    free(keyBoardFile);
    free(keyBoardFileFullPath);
}
/** InitMachine() ********************************************/
/** Allocate resources needed by the machine-dependent code.**/
/************************************ TO BE WRITTEN BY USER **/
int InitChangeGame(const char* name) {
    printf("Should load: %s\n", name);
    loadKeyMappingFromGame(name);
    if (hasExt(name, ".rom\0.mx1\0.mx2\0\0")) { ROMName[0] = name; printf("is rom\n");}
    if (hasExt(name, ".dsk\0\0")){  DSKName[0] = name; printf("is disc\n");ROMName[0] = "";}
    if (hasExt(name, ".cas\0\0")){ CasName = name; printf("is cas\n");ROMName[0] = "";}

    odroidFmsxGUI_setLastLoadedFile(name);
}
int InitMachine(void){ 
    

    
    lastGame = malloc(1024);
    initFiles();
    
    setDefaultKeymapping();
    LoadKeyMapping("/sd/odroid/data/msx/config.ini");
  
    InitVideo();
    odroidFmsxGUI_initMenu();
   
    
    int res = ini_gets("FMSX", "LASTGAME", "", lastGame, 1024, FMSX_CONFIG_FILE);
    if (res) {
        InitChangeGame(lastGame);
    }
    
    InitSound(AUDIO_SAMPLE_RATE, 0);
   
    odroidQwertyFound = odroid_qwerty_init();
    printf("odroid_qwerty_found: %d\n",odroidQwertyFound);
    memset(odroidQwertyPushedKeys.values, ODROID_QWERTY_NONE, ODROID_QWERTY_MAX_KEYS);

    return 1; 
}




/** Joystick() ***********************************************/
/** Query positions of two joystick connected to ports 0/1. **/
/** Returns 0.0.B2.A2.R2.L2.D2.U2.0.0.B1.A1.R1.L1.D1.U1.    **/
/************************************ TO BE WRITTEN BY USER **/
void checkKey(int key, odroid_gamepad_state out_state) {
    if (keyMapping[key]  > 0xFF) {
        // its a keyboard key
        if (out_state.values[ODROID_INPUT_START] && ! pushedKeys[ODROID_INPUT_START]){ KBD_SET(keyMapping[ODROID_INPUT_START] >> 8); pushedKeys[ODROID_INPUT_START] = 1;}
        if (! out_state.values[ODROID_INPUT_START] && pushedKeys[ODROID_INPUT_START]){ KBD_RES(keyMapping[ODROID_INPUT_START] >> 8); pushedKeys[ODROID_INPUT_START] = 0;}
    }
}
void keybmoveCursor(odroid_gamepad_state out_state) {
    
    if (out_state.values[ODROID_INPUT_UP]) moveCursor(0,-2);
    if (out_state.values[ODROID_INPUT_DOWN])moveCursor(0,2);
    if (out_state.values[ODROID_INPUT_LEFT]) moveCursor(-2,0);
    if (out_state.values[ODROID_INPUT_RIGHT]) moveCursor(2,0);
   
    
    if (out_state.values[ODROID_INPUT_A] && pushedVirtKeyboardKey == -1){
        int key = mousePress();
        if (key != -1) 
        {
            KBD_SET(key); 
            pushedVirtKeyboardKey = key; 
        }
        
    }
    if (! out_state.values[ODROID_INPUT_A] && pushedVirtKeyboardKey != -1) {
        KBD_RES(pushedVirtKeyboardKey);
        pushedVirtKeyboardKey = -1;
    }
    if (out_state.values[ODROID_INPUT_B] && holdVirtKeyboardKey == -1){
        int key = mousePress();
        if (key != -1) 
        {
            KBD_SET(key); 
            holdVirtKeyboardKey = key; 
        }
    }
    if (! out_state.values[ODROID_INPUT_B] && holdVirtKeyboardKey != -1){
        KBD_RES(holdVirtKeyboardKey);
        holdVirtKeyboardKey = -1;
    }
    if (out_state.values[ODROID_INPUT_SELECT] && holdVirtKeyboardSelectKey == -1){
        doFlipScreen();
        holdVirtKeyboardSelectKey = 1;
    }
    if (! out_state.values[ODROID_INPUT_SELECT]) holdVirtKeyboardSelectKey = -1;
     
}
// standard ctrl buttons:
/*
 ctrl + h = delete
 * ctrl + b cursor up
 * ctrl + j cursor down
 * ctrl + r insert
 */

uint8_t toMSXKey(uint8_t key, bool pressedCtrl) {
    if (key == ODROID_QWERTY_TILDE) return KBD_STOP;
    if (key == ODROID_QWERTY_ESC) return KBD_HOME;
    if (key == ODROID_QWERTY_CTRL) return KBD_CONTROL;
    if (key == ODROID_QWERTY_ALT) return KBD_GRAPH;
    if (key == ODROID_QWERTY_SHIFT) return KBD_SHIFT;
    if (key == ODROID_QWERTY_BS) return KBD_BS;
    if (key == ODROID_QWERTY_ENTER) return KBD_ENTER;
    if (key == ODROID_QWERTY_1 && pressedCtrl) return KBD_F1;
    if (key == ODROID_QWERTY_2 && pressedCtrl) return KBD_F2;
    if (key == ODROID_QWERTY_3 && pressedCtrl) return KBD_F3;
    if (key == ODROID_QWERTY_4 && pressedCtrl) return KBD_F4;
    if (key == ODROID_QWERTY_5 && pressedCtrl) return KBD_F5;
    
    char asc = odroid_qwerty_key_to_ascii(key, true);
    if (asc >= 0x20 &&  asc < 0x61) return asc;
    
    return 0x00;
   
}
unsigned int Joystick(void) {
    
    
    unsigned int returnState = 0;
    odroid_gamepad_state out_state;
    odroid_input_gamepad_read(&out_state);
#ifdef WITH_WLAN
    if (getMultiplayState() != MULTIPLAYER_CONNECTED_CLIENT) {
#endif
    bool pressed = false;
    for (int i = 0; i < ODROID_INPUT_MAX; i++) if (out_state.values[i]) pressed = true;
    if (! vKeyboardShow) {
        if (out_state.values[ODROID_INPUT_A] && out_state.values[ODROID_INPUT_MENU]){
            vKeyboardShow = 2;
            showVirtualKeyboard();
#ifdef WITH_WLAN            
            exchangeJoystickState(&returnState);
#endif
            return returnState;
        }
    } else if (vKeyboardShow == 1) {
        if (out_state.values[ODROID_INPUT_MENU]){
            vKeyboardShow = 3;
            hideVirtualKeyboard();
            clearScreen();
        }
    } else if(vKeyboardShow == 2 && !out_state.values[ODROID_INPUT_MENU]) {
        vKeyboardShow = 1;
    } else if(vKeyboardShow == 3 && !out_state.values[ODROID_INPUT_MENU]) {
        vKeyboardShow = 0;
        
    }
    if (vKeyboardShow) {
        if (vKeyboardShow == 1) keybmoveCursor(out_state);
#ifdef WITH_WLAN
        exchangeJoystickState(&returnState);
#endif        
        return returnState;
    }
    if (inMenue && !pressed) inMenue = 0; 
#ifdef WITH_WLAN
    }
#endif    
    /* joystick mapping */
    for (int i = 0; i < ODROID_INPUT_MAX; i++){
        if (i == ODROID_INPUT_MENU || i == ODROID_INPUT_VOLUME) continue;
        if (keyMapping[i] <= 0xFF){
            // it is a joystick
            if (out_state.values[i]) returnState |= keyMapping[i];
        }
        if (keyMapping[i] > 0xFF){
            // it is a keyboard key
            if (out_state.values[i] && ! pushedKeys[i]){KBD_SET(keyMapping[i] >> 8); pushedKeys[i] = 1;}
            if (! out_state.values[i] && pushedKeys[i]){KBD_RES(keyMapping[i] >> 8); pushedKeys[i] = 0;}
        }
    }
    /* odroid qwerty keyboard */
    if (odroidQwertyFound) {
        odroid_qwerty_state keys;
        bool newKey = odroid_qwerty_read(&keys);
        if (newKey) {
           for (int i = 0; i < ODROID_QWERTY_MAX_KEYS; i++) {
               
               /// hold shift
               if (odroid_qwerty_is_key_pressed(&keys, ODROID_QWERTY_SHIFT) && odroid_qwerty_is_key_pressed(&keys, ODROID_QWERTY_CTRL))  {holdShift = 2;odroid_qwerty_led_Aa(true);}
               else if (holdShift == 2 && ! odroid_qwerty_is_key_pressed(&keys, ODROID_QWERTY_SHIFT)) holdShift = 1;
               else if (holdShift == 1 && odroid_qwerty_is_key_pressed(&keys, ODROID_QWERTY_SHIFT))   {holdShift = 0;odroid_qwerty_led_Aa(false);}
               /////////////
               if (odroidQwertyPushedKeys.values[i] != keys.values[i]) {
                    if (keys.values[i] == ODROID_QWERTY_NONE ) {
                        uint8_t key = toMSXKey(odroidQwertyPushedKeys.values[i], odroid_qwerty_is_key_pressed(&odroidQwertyPushedKeys, ODROID_QWERTY_CTRL));
                        KBD_RES(key);
                    } else {
                        uint8_t key = toMSXKey(keys.values[i], odroid_qwerty_is_key_pressed(&keys, ODROID_QWERTY_CTRL));
                        KBD_SET(key);
                    }
                }
                if (holdShift) KBD_SET(KBD_SHIFT);
            }
            memcpy(odroidQwertyPushedKeys.values, keys.values, ODROID_QWERTY_MAX_KEYS);
        }
    }
    
    if (!inMenue){    
        if (out_state.values[ODROID_INPUT_VOLUME]) {
            if (! pushedKeys[ODROID_INPUT_VOLUME]) {
                audio_volume_set_change();
                pushedKeys[ODROID_INPUT_VOLUME] = 1;
            }
        } else { pushedKeys[ODROID_INPUT_VOLUME] = 0; }
#ifdef WITH_WLAN
    if (getMultiplayState() == MULTIPLAYER_NOT_CONNECTED) {
#endif        
        if (out_state.values[ODROID_INPUT_MENU]) {
            // go into menu
            pressCounter = 0;
            lastKey = ODROID_INPUT_MENU; //the last pressed key for the menu
            inMenue = 1;
            pause_audio();
            clearOverlay();
            
            
            if (vKeyboardShow) hideVirtualKeyboard();
            odroidFmsxGUI_showMenu();
            if (vKeyboardShow) showVirtualKeyboard();
                
                
            
            restart_audio();

            clearScreen();
        }
#ifdef WITH_WLAN
    }
#endif

    }
#ifdef WITH_WLAN   
    if (isConnectionLost()) {
        pause_audio();
        clearOverlay();
        odroidFmsxGUI_msgBox("Connection", " \nThe connection has been lost\nPress a key to restart\n ", 1); 
        esp_restart();
    } 
    exchangeJoystickState(&returnState);
#endif
    
    return returnState;
}



/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/************************************ TO BE WRITTEN BY USER **/
void TrashMachine(void){
   
   TrashVideo();
   free(lastGame);
   
}


/** GetJoystick() ********************************************/
/** Get the state of joypad buttons (1="pressed"). Refer to **/
/** the BTN_* #defines for the button mappings. Notice that **/
/** on Windows this function calls ProcessEvents() thus    **/
/** automatically handling all Windows messages.            **/
/*************************************************************/
unsigned int GetJoystick(void) {
    return 0;
}
/** Keyboard() ***********************************************/
/** This function is periodically called to poll keyboard.  **/
/************************************ TO BE WRITTEN BY USER **/
void Keyboard(void){
      /* Everything is done in Joystick() */
   
   
}

/** Mouse() **************************************************/
/** Query coordinates of a mouse connected to port N.       **/
/** Returns F2.F1.Y.Y.Y.Y.Y.Y.Y.Y.X.X.X.X.X.X.X.X.          **/
/************************************ TO BE WRITTEN BY USER **/
unsigned int Mouse(byte N) {return 0;}

/** DiskPresent()/DiskRead()/DiskWrite() *********************/
/*** These three functions are called to check for floppyd  **/
/*** disk presence in the "drive", and to read/write given  **/
/*** sector to the disk.                                    **/
/************************************ TO BE WRITTEN BY USER **/
//byte DiskPresent(byte ID) {return 0;}
//byte DiskRead(byte ID,byte *Buf,int N) {return 0;}
//byte DiskWrite(byte ID,const byte *Buf,int N) {return 0;}





