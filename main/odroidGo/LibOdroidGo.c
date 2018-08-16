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



uint8_t inMenue = 0;

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
    
    
   initFiles();
    
    
   
    
    setDefaultKeymapping();
    
    
    int res = ini_gets("FMSX", "LASTGAME", "", GetLastLoadedFile(), 512, FMSX_CONFIG_FILE);
    if (res) {
        loadKeyMappingFromGame(GetLastLoadedFile());
        if (strstr(GetLastLoadedFile(), ".rom") || strstr(GetLastLoadedFile(), ".mx2")) ROMName[0] = GetLastLoadedFile();
        if (strstr(GetLastLoadedFile(), ".dsk")) DSKName[0] = GetLastLoadedFile();
       
    }
    
    
    
    
  
    InitVideo();
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
unsigned int Joystick(void) {
    unsigned int returnState = 0;
    
    
    
    odroid_gamepad_state out_state;
    odroid_input_gamepad_read(&out_state);
   
    bool pressed = false;
    for (int i = 0; i < ODROID_INPUT_MAX; i++) if (out_state.values[i]) pressed = true;
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
            
            int action = MenuMSX();
            if (action == MENU_ACTION_LOADFILE){
                char* lastLoadedFile = GetLastLoadedFile();
                if (lastLoadedFile) {
                    ini_puts("FMSX", "LASTGAME", lastLoadedFile, FMSX_CONFIG_FILE);
                }

                if (lastLoadedFile && strlen(lastLoadedFile) > 0) {
                  loadKeyMappingFromGame(lastLoadedFile);
                }
            }
            if (action == MENU_ACTION_DROPROM){
                ini_puts("FMSX", "LASTGAME", "", FMSX_CONFIG_FILE);
            }
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










/** WaitKeyOrMouse() *****************************************/
/** Wait for a key or a mouse button to be pressed. Returns **/
/** the same result as GetMouse(). If no mouse buttons      **/
/** reported to be pressed, call GetKey() to fetch a key.   **/
/*************************************************************/

unsigned int WaitKeyOrMouse(void) {
    register unsigned int j = 0xFFFFFF;
    odroid_gamepad_state out_state;
    bool pressed;
    do{
        pressed = false;
        odroid_input_gamepad_read(&out_state);
        for (int i = 0; i < ODROID_INPUT_MAX; i++) if (out_state.values[i]) {pressed = true; j = i;}
        if (!pressed) {j = lastKey = 0xFFFFFF;pressCounter=0;} else pressCounter++;
        vTaskDelay(6 / portTICK_PERIOD_MS);
        
    }while(j == lastKey && pressCounter < 100);
    
   lastKey = j;
   
   switch(j) {
       case ODROID_INPUT_RIGHT: return CON_RIGHT;
       case ODROID_INPUT_LEFT: return CON_LEFT;
       case ODROID_INPUT_UP: return CON_UP;
       case ODROID_INPUT_DOWN: return CON_DOWN;
       case ODROID_INPUT_A: return CON_OK;
       default: return 0;
   }
   
   
   return 0;
}
unsigned int GetKey(void) {
   // printf("GetKey\n");
    return 0;
}
unsigned int WaitKey(void) {
    return WaitKeyOrMouse();
}
