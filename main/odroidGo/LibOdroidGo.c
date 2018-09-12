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
#include <esp_heap_caps.h>
#include "sound.h"
#include "odroid_audio.h"
#include "ff.h"

#include "minIni.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"



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
    if (keyMapping[Key] == -1) {
        keyMapping[Key] = mappingString[0] << 8;
    }
}
char LoadKeyMapping(char* KeyFile) {
    //long  ini_getl(const "KEYMAPPING", "UP", long DefValue, const mTCHAR *Filename);
    char buffer[16];
    int res;
   
    res = ini_gets("KEYMAPPING", "UP", "JST_UP", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_UP, buffer);
    
    res = ini_gets("KEYMAPPING", "RIGHT", "JST_RIGHT", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_RIGHT, buffer);
    
    res = ini_gets("KEYMAPPING", "DOWN", "JST_DOWN", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_DOWN, buffer);
    
    res = ini_gets("KEYMAPPING", "LEFT", "JST_LEFT", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_LEFT, buffer);
    
    res = ini_gets("KEYMAPPING", "SELECT", "2", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_SELECT, buffer);
    
    res = ini_gets("KEYMAPPING", "START", "1", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_START, buffer);
    
    res = ini_gets("KEYMAPPING", "A", "JST_FIREA", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_A, buffer);
    
    res = ini_gets("KEYMAPPING", "B", "JST_FIREB", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_B, buffer);
    
      
    
    
    
}


void loadKeyMappingFromGame(const char* gameFileName) {
    const char* filename = getFileName(gameFileName);
    char* keyBoardFile = malloc(256);
    char* keyBoardFileFullPath = malloc(612);
    strncpy(keyBoardFile, filename, 256);
    keyBoardFile = cutExtension(keyBoardFile);
    snprintf(keyBoardFileFullPath, 612, "/sd/odroid/data/msx/%s.ini", keyBoardFile);
    setDefaultKeymapping();
    LoadKeyMapping(keyBoardFileFullPath);
    free(keyBoardFile);
    free(keyBoardFileFullPath);
}
/** InitMachine() ********************************************/
/** Allocate resources needed by the machine-dependent code.**/
/************************************ TO BE WRITTEN BY USER **/

int InitMachine(void){ 
    
    
    lastGame = malloc(1024);
    initFiles();
    setDefaultKeymapping();

    InitVideo();
    odroidFmsxGUI_initMenu();

    int res = ini_gets("FMSX", "LASTGAME", "", lastGame, 1024, FMSX_CONFIG_FILE);
    if (res) {
        loadKeyMappingFromGame(lastGame);
        if (hasExt(lastGame, ".rom\0.mx1\0.mx2\0\0")) ROMName[0] = lastGame;
        if (hasExt(lastGame, ".dsk\0\0")) DSKName[0] = lastGame;
        if (hasExt(lastGame, ".cas\0\0")) CasName = lastGame;
        
        odroidFmsxGUI_setLastLoadedFile(lastGame);

    }

    InitSound(AUDIO_SAMPLE_RATE, 0);
   

   

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
unsigned int Joystick(void) {
    unsigned int returnState = 0;
    
    
    
    odroid_gamepad_state out_state;
    odroid_input_gamepad_read(&out_state);
   
    bool pressed = false;
    for (int i = 0; i < ODROID_INPUT_MAX; i++) if (out_state.values[i]) pressed = true;
    if (! vKeyboardShow) {
        if (out_state.values[ODROID_INPUT_A] && out_state.values[ODROID_INPUT_MENU]){
            vKeyboardShow = 2;
            showVirtualKeyboard();
            return 0;
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
        return 0;
    }
    if (inMenue && !pressed) inMenue = 0; 
    
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
      
    
    if (!inMenue){    
        if (out_state.values[ODROID_INPUT_VOLUME]) {
            if (! pushedKeys[ODROID_INPUT_VOLUME]) {
                audio_volume_set_change();
                pushedKeys[ODROID_INPUT_VOLUME] = 1;
            }
        } else { pushedKeys[ODROID_INPUT_VOLUME] = 0; }
        
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

    }
   

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








