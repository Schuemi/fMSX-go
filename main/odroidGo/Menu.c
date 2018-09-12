/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "LibOdroidGo.h"
#include "ugui.h"
#include "odroid_input.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include "MSX.h"
#include "EMULib.h"

#include "minini.h"

#define MAX_OBJECTS 6
#define MAX_OBJECTS_VIRTUAL_KEYBOARD 64

#define MENU_TEXTBOX_ID 1
#define FILES_TEXTBOX_ID 2 
#define MSG_TEXTBOX_ID 3 

#define FILES_MAX_ROWS   20
#define FILES_MAX_LENGTH 31
#define FILES_HEADER_LENGTH 27
#define FILES_MAX_LENGTH_NAME 256



typedef unsigned short pixelp;


pixelp* pixelBuffer;

UG_GUI   gui;
UG_WINDOW window;
UG_TEXTBOX menuTextBox;
UG_OBJECT obj_buff_wnd_1[MAX_OBJECTS];

UG_WINDOW fileWindow;
UG_TEXTBOX fileTextBox;
UG_OBJECT obj_buff_wnd_file[MAX_OBJECTS];

UG_WINDOW msgWindow;
UG_TEXTBOX msgTextBox;
UG_OBJECT obj_buff_wnd_msg[MAX_OBJECTS];


int m_width = WIDTH_OVERLAY;
int m_height = HEIGHT_OVERLAY;
int currentSelectedItem = 0;
char* menuTxt;
bool keyPressed;
int lastPressedKey;
int lastSelectedItem = 0;
int holdDownCounter = 0;
char* selectedFile;
char* lastOpenedFileFullPath;

int position = 0;
int selPosition = 0;
   
#define MENU_ITEMS 10
static const struct {
    const char* menuItem[MENU_ITEMS];
    
} menuItems = {
   "Load file\n",
   "Save state\n",
   "\n",
   "Eject cartridge\n",
   "Eject disks\n",
   "\n",
   "fMSX Menu\n",
   "Reset\n",
   "\n",
   "About\n",
   
   
};

void pixelSet(UG_S16 ul_x,UG_S16 ul_y, UG_COLOR ul_color){
    if (ul_x > m_width || ul_y > m_height) return;
    pixelBuffer[ul_x + (ul_y*m_width)] = ul_color;
}

void window_callback( UG_MESSAGE* msg ) {
    
}
void odroidFmsxGUI_initMenu() {
   
  
   pixelBuffer = (pixelp*)malloc(m_width*m_height*sizeof(pixelp));
   memset(pixelBuffer, 0, m_width*m_height*sizeof(pixelp));
   
   selectedFile = (char*)malloc(FILES_MAX_LENGTH_NAME+1);
   lastOpenedFileFullPath = (char*)malloc(1024);
   lastOpenedFileFullPath[0] = 0;
   
   menuTxt = (char*)malloc(1000);
   
   UG_Init(&gui,pixelSet,m_width, m_height);
   
   UG_WindowCreate ( &window , obj_buff_wnd_1 , MAX_OBJECTS, window_callback);
   UG_WindowResize(&window, 10, 10, 310, 220);
   UG_WindowSetTitleTextFont ( &window , &FONT_8X8 ) ;
   UG_WindowSetTitleText ( &window , "ODROID-GO fMSX" ) ;
   UG_WindowSetBackColor( &window , C_GRAY );
   UG_TextboxCreate(&window, &menuTextBox, MENU_TEXTBOX_ID, 10, 10, 290, 160); 
   UG_TextboxSetAlignment(&window, MENU_TEXTBOX_ID, ALIGN_TOP_LEFT);
   UG_TextboxSetForeColor(&window, MENU_TEXTBOX_ID, C_BLACK);
   UG_TextboxSetFont ( &window , MENU_TEXTBOX_ID, &FONT_8X8 ) ;
   
   
   
   UG_TextboxShow(&window, MENU_TEXTBOX_ID);
   UG_WindowShow(&window);
   
   
   
   
}

int odroidFmsxGUI_getKey() {
    odroid_gamepad_state out_state;
    odroid_input_gamepad_read(&out_state);
    keyPressed = false;
    for (int i = 0; i < ODROID_INPUT_MAX; i++) if (out_state.values[i]) keyPressed = true;
    if (keyPressed && lastPressedKey != -1){
        holdDownCounter++;
        if (holdDownCounter > 200) return lastPressedKey;
        return -1;
    }
    holdDownCounter = 0;
    
    if (!keyPressed) lastPressedKey = -1; else {
        if (out_state.values[ODROID_INPUT_UP]) lastPressedKey = ODROID_INPUT_UP;
        if (out_state.values[ODROID_INPUT_DOWN]) lastPressedKey = ODROID_INPUT_DOWN;
        if (out_state.values[ODROID_INPUT_LEFT]) lastPressedKey = ODROID_INPUT_LEFT;
        if (out_state.values[ODROID_INPUT_RIGHT]) lastPressedKey = ODROID_INPUT_RIGHT;
        if (out_state.values[ODROID_INPUT_A]) lastPressedKey = ODROID_INPUT_A;
        if (out_state.values[ODROID_INPUT_MENU]) lastPressedKey = ODROID_INPUT_MENU;
    }
    return lastPressedKey;
}
int odroidFmsxGUI_getKey_block() {
    int key;
    do{
        vTaskDelay(1 / portTICK_PERIOD_MS);
        key = odroidFmsxGUI_getKey();
    }while (key == -1);
    return key;
}





const char* odroidFmsxGUI_chooseFile(const char *Ext) {
   
   DIR *D;
   struct dirent *DP;
   struct stat ST;
   

   char* Buf;
   int BufSize = 256;
   char* txtFiles;
   char* shownFiles[FILES_MAX_ROWS];
   
   
   txtFiles = malloc(FILES_MAX_ROWS*(FILES_MAX_LENGTH + 5) + 1);
   for (int i = 0; i < FILES_MAX_ROWS; i++) {
       shownFiles[i] = malloc(FILES_MAX_LENGTH_NAME+1);
   }
   
   Buf = malloc(256);
   char* txtFilesPosition;
   
   
   
   int i, r, s, fileCount;

   
   bool isDir[FILES_MAX_ROWS];
   bool switchedPage = true;
   
   UG_WindowCreate ( &fileWindow , obj_buff_wnd_file , MAX_OBJECTS, window_callback);
   UG_WindowResize(&fileWindow, 20, 20, 300, 210);
   UG_WindowSetTitleTextFont ( &fileWindow , &FONT_8X8 ) ;
   
   if(!getcwd(Buf,BufSize-2)) strncpy(Buf,"Choose File", 256);
   txtFilesPosition = Buf;
   if (strlen(txtFilesPosition) > FILES_HEADER_LENGTH) txtFilesPosition += strlen(txtFilesPosition) - FILES_HEADER_LENGTH;
  
   
   
   UG_WindowSetTitleText ( &fileWindow , txtFilesPosition ) ;
   UG_WindowSetBackColor( &fileWindow , C_GRAY );
   UG_TextboxCreate(&fileWindow, &fileTextBox, FILES_TEXTBOX_ID, 1, 1, 274, 80); 
   
   UG_TextboxSetAlignment(&fileWindow, FILES_TEXTBOX_ID, ALIGN_TOP_LEFT);
   UG_TextboxSetForeColor(&fileWindow, FILES_TEXTBOX_ID, C_BLACK);
   UG_TextboxSetFont ( &fileWindow , FILES_TEXTBOX_ID, &FONT_8X8 ) ;
   UG_TextboxShow(&fileWindow, FILES_TEXTBOX_ID);
   UG_WindowShow(&fileWindow);
   
   int keyNumPressed;
   
   if((D=_opendir("."))){

        
        fileCount = 0;
        // count how many files we have here
        for (_rewinddir(D); (DP=_readdir(D));){
            if (DP->d_type==DT_DIR || hasExt(DP->d_name, Ext)) {fileCount++;}
        }
       
        do {
            
            txtFilesPosition = txtFiles;
         
         // read a new Page
         if (switchedPage){
            // first, go to the position
            _rewinddir(D);
            for (s=0; s < position && (DP=_readdir(D)); s++){
                if (DP->d_type!=DT_DIR && !hasExt(DP->d_name, Ext)) {s--; continue;}
            }
            // than read the next FILES_MAX_ROWS files
            for(i=0;(i < FILES_MAX_ROWS && (DP=_readdir(D)));i++) {
               isDir[i] = false;
               if (DP->d_type==DT_DIR) isDir[i] = true;
               if (isDir[i] == false && !hasExt(DP->d_name, Ext)) {i--; continue;}
               strncpy(shownFiles[i],DP->d_name,FILES_MAX_LENGTH_NAME);
            }
            switchedPage = false;
            if (i == 0) {
                // nothing on this page, go to the first page, if we are not already
                if (position >= FILES_MAX_ROWS) {
                    position = 0;
                    selPosition = 0;
                    switchedPage = true;
                    continue;
                }
                
            }
        } 
       
        /// Draw the TextBox
        r = i;
        for(i=0;i<r;i++) {
           if (selPosition == i + position) *txtFilesPosition = 16;  else *txtFilesPosition = 0x20;
           txtFilesPosition++;
           if (isDir[i]) *txtFilesPosition = 0xfd; else   *txtFilesPosition = 0xfe;
           txtFilesPosition++;*txtFilesPosition = 0x20;txtFilesPosition++;
           strncpy(txtFilesPosition, shownFiles[i], FILES_MAX_LENGTH);
           if (strlen(shownFiles[i]) < FILES_MAX_LENGTH) txtFilesPosition += strlen(shownFiles[i]); else txtFilesPosition += FILES_MAX_LENGTH;
           *txtFilesPosition = '\n';
           txtFilesPosition++;
          
        }
        
        
        *txtFilesPosition = 0;
         
        UG_WindowShow(&fileWindow); // force a window update
        UG_TextboxSetText( &fileWindow , FILES_TEXTBOX_ID, txtFiles);
        
         UG_Update();
         DrawuGui(pixelBuffer, 0);
         keyNumPressed = odroidFmsxGUI_getKey_block();
         
         if (keyNumPressed == ODROID_INPUT_RIGHT) selPosition+= FILES_MAX_ROWS;
         if (keyNumPressed == ODROID_INPUT_LEFT)  selPosition-= FILES_MAX_ROWS;
         if (keyNumPressed == ODROID_INPUT_DOWN)  selPosition++;
         if (keyNumPressed == ODROID_INPUT_UP)    selPosition--;
         
         
         if (selPosition < 0) {
             // go to the last page
             selPosition = fileCount -1;
             position = FILES_MAX_ROWS * (int)(fileCount / FILES_MAX_ROWS);
             if (position == fileCount) position -= FILES_MAX_ROWS;
             switchedPage = true;
             
         }
         if (i != FILES_MAX_ROWS && selPosition >= position + i) { 
             // go to the first page
             selPosition = 0; position = 0;  switchedPage = true;
         }
         
         if (selPosition >= FILES_MAX_ROWS + position){ 
             // go to next page
             position += FILES_MAX_ROWS; selPosition = position;  switchedPage = true;
         }
         
         if (selPosition < position){ 
             // go to previous page
             position -= FILES_MAX_ROWS; 
             if (position < 0) position = 0;
             selPosition = position + FILES_MAX_ROWS - 1; 
             switchedPage = true;
         }
         if (selPosition - position >= 0 && selPosition - position < FILES_MAX_ROWS && keyNumPressed == ODROID_INPUT_A && isDir[selPosition - position]) {
            free(txtFiles);
            free(Buf);
            UG_TextboxDelete(&fileWindow, FILES_TEXTBOX_ID);
            UG_WindowDelete(&fileWindow);
            
            chdir(shownFiles[selPosition - position]);
            for (int i = 0; i < FILES_MAX_ROWS; i++) free(shownFiles[i]);
            closedir(D);
            return odroidFmsxGUI_chooseFile(Ext);
         }
         

        } while(keyNumPressed != ODROID_INPUT_A && keyNumPressed != ODROID_INPUT_MENU);
        
        
        closedir(D);
    }
   if(keyNumPressed == ODROID_INPUT_A) {
       strncpy(selectedFile, shownFiles[selPosition - position], FILES_MAX_LENGTH_NAME);
   }
   free(txtFiles);
   free(Buf);
   for (int i = 0; i < FILES_MAX_ROWS; i++) free(shownFiles[i]);
   UG_TextboxDelete(&fileWindow, FILES_TEXTBOX_ID);
   UG_WindowDelete(&fileWindow);
   
   if(keyNumPressed == ODROID_INPUT_A) {
            return selectedFile;
   }
   return NULL;
}
void odroidFmsxGUI_selectMenuItem(int item) {
    char* menuPos = menuTxt;
    for (int i = 0; i < MENU_ITEMS; i++) {
        if (item == i) *menuPos = 16;  else *menuPos = 0x20;
        menuPos++;*menuPos = 0x20;menuPos++;
        int len = strlen(menuItems.menuItem[i]);
        memcpy(menuPos, menuItems.menuItem[i], len);
        menuPos += len;
    }
    *menuPos = 0;
    UG_TextboxSetText( &window , MENU_TEXTBOX_ID, menuTxt);
    UG_TextboxSetAlignment(&window , MENU_TEXTBOX_ID,ALIGN_H_LEFT|ALIGN_V_CENTER);
}

void odroidFmsxGUI_setLastLoadedFile(const char* file) {
    if (file != NULL)
        strncpy(lastOpenedFileFullPath, file, 1024);
    else 
        lastOpenedFileFullPath[0] = 0;
}

// msg Box: max 33 letters in one row!

void odroidFmsxGUI_msgBox(const char* title, const char* msg, char waitKey) {
   int rows = 1;
   const char* p = msg;
   while(*p++ != 0){
       if (*p == '\n') rows++;
   }
   
   // if there is a old instance
   UG_TextboxDelete(&msgWindow, MSG_TEXTBOX_ID);
   UG_WindowDelete(&msgWindow);
   
   
   UG_WindowCreate ( &msgWindow , obj_buff_wnd_file , MAX_OBJECTS, window_callback);
   UG_WindowResize(&msgWindow, 20, 20, 300, 54 + (rows*8));
  
   
   
   UG_WindowSetTitleTextFont ( &msgWindow , &FONT_8X8 ) ;
   UG_WindowSetTitleText ( &msgWindow , title ) ;
   UG_WindowSetBackColor( &msgWindow , C_GRAY );
   
   
   UG_TextboxCreate(&msgWindow, &msgTextBox, MSG_TEXTBOX_ID, 6, 6, 274, 12+rows*8); 
   
   UG_TextboxSetAlignment(&msgWindow, MSG_TEXTBOX_ID, ALIGN_TOP_LEFT);
   UG_TextboxSetForeColor(&msgWindow, MSG_TEXTBOX_ID, C_BLACK);
   UG_TextboxSetFont ( &msgWindow , MSG_TEXTBOX_ID, &FONT_8X8 ) ;
   UG_TextboxShow(&msgWindow, MSG_TEXTBOX_ID);
   UG_WindowShow(&msgWindow);
   
   UG_TextboxSetText( &msgWindow , MSG_TEXTBOX_ID, msg);
   
   
   UG_Update();
   DrawuGui(pixelBuffer, 0);
       
   if (waitKey && odroidFmsxGUI_getKey_block()){}
   
  
   
}
void odroidFmsxGUI_showMenu() {
   
   char stopMenu = 0;
   odroidFmsxGUI_selectMenuItem(currentSelectedItem);
   
   
   
   
   // wait until the menu button is not pressed anymore
   int keyPressed;
   do {
       keyPressed = odroidFmsxGUI_getKey();
   }while (keyPressed == ODROID_INPUT_MENU);
   
   
   /// now listen for another button

    do {
       int c = 0;
       UG_WindowShow(&window); // force a window update
       UG_Update();
       DrawuGui(pixelBuffer, 0);
       
       keyPressed = odroidFmsxGUI_getKey_block();
       if (keyPressed == ODROID_INPUT_DOWN) c = 1;
       if (keyPressed == ODROID_INPUT_UP) c = -1;
       
       currentSelectedItem += c;
       
       if (currentSelectedItem < 0)currentSelectedItem = MENU_ITEMS - 1;
       if (currentSelectedItem >= MENU_ITEMS)currentSelectedItem = 0;
       
       if (menuItems.menuItem[currentSelectedItem][0] == '\n') currentSelectedItem += c;
           
           
       if (lastSelectedItem != currentSelectedItem) {
           lastSelectedItem = currentSelectedItem;
           odroidFmsxGUI_selectMenuItem(currentSelectedItem);
       }
       
       if (keyPressed == ODROID_INPUT_A) {
           const char* lastSelectedFile = NULL;
           switch(currentSelectedItem){
               ///////////// Load File ///////////////////
               case 0:   
                   lastSelectedFile = odroidFmsxGUI_chooseFile(".rom\0.mx1\0.mx2\0.dsk\0.cas\0\0"); 
                   odroidFmsxGUI_msgBox("Please wait...", "Please wait while loading", 0);
                   if (lastSelectedFile != NULL) {
                       if (LoadFile(lastSelectedFile)){
                           getFullPath(lastOpenedFileFullPath, lastSelectedFile, 1024);
                           ini_puts("FMSX", "LASTGAME", lastOpenedFileFullPath, FMSX_CONFIG_FILE);
                           loadKeyMappingFromGame(lastSelectedFile);
                       } else lastOpenedFileFullPath[0] = 0;
                       stopMenu = true;
                   }
                   break; 
                   
                //////////////// Save State ////////////////////
               case 1:
                   if (lastOpenedFileFullPath[0] != 0) {
                        odroidFmsxGUI_msgBox("Please wait...", "Please wait while saving", 0);
                        char* stateFileName = malloc(1024);
                        char* stateFileNameF = malloc(1024);
                        strncpy(stateFileName, lastOpenedFileFullPath, 1024);
                        cutExtension(stateFileName);
                        snprintf(stateFileNameF, 1024, "%s.sta", stateFileName);
                        if (!SaveSTA(stateFileNameF)){
                            odroidFmsxGUI_msgBox("Error", "Could not save state", 1);
                        }
                        free(stateFileName);
                        free(stateFileNameF);
                   }
                   stopMenu = true;
                   break;
                   
                //////////////// Eject Cardridge ////////////////////
               case 3:
                   for(int J=0;J<MAXSLOTS;++J) LoadCart(0,J,0);
                   ini_puts("FMSX", "LASTGAME", "", FMSX_CONFIG_FILE);
                   stopMenu = true;
                   break;
               case 4:
                   for(int J=0;J<MAXDRIVES;++J) ChangeDisk(J,0);
                   ini_puts("FMSX", "LASTGAME", "", FMSX_CONFIG_FILE);
                   stopMenu = true;
                   break;
               case 6:
                   MenuMSX();
                   stopMenu = true;
                   break;
               case 7:
                   ResetMSX(Mode,RAMPages,VRAMPages);
                   stopMenu = true;
                   break;
               case 9:
                   odroidFmsxGUI_msgBox("About", " \nfMSX\n\n by Marat Fayzullin\n\n ported by Jan P. Schuemann\n\nThanks to the ODROID-GO community\n\nHave fun!\n ", 1);
               break;
           };
       }
       if (keyPressed == ODROID_INPUT_MENU) stopMenu = true;
       
       
   }while (!stopMenu);
   
    
}


// Key handling for the old menu


/** WaitKeyOrMouse() *****************************************/
/** Wait for a key or a mouse button to be pressed. Returns **/
/** the same result as GetMouse(). If no mouse buttons      **/
/** reported to be pressed, call GetKey() to fetch a key.   **/
/*************************************************************/

unsigned int WaitKeyOrMouse(void) {
   int key = odroidFmsxGUI_getKey_block();
   switch(key) {
       case ODROID_INPUT_RIGHT: return CON_RIGHT;
       case ODROID_INPUT_LEFT: return CON_LEFT;
       case ODROID_INPUT_UP: return CON_UP;
       case ODROID_INPUT_DOWN: return CON_DOWN;
       case ODROID_INPUT_A: return CON_OK;
       default: return 0;
   }
}
unsigned int GetKey(void) {
   
    return 0;
}
unsigned int WaitKey(void) {
    return WaitKeyOrMouse();
}

