/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: Test
 *
 * Created on 25. Juli 2018, 09:38
 */



#include <stdint.h>
#include <esp_err.h>
#include "odroid_sdcard.h"
#include "odroid_display.h"
#include "odroid_system.h"
#include "MSX.h"
#include "EMULib.h"
#include "Console.h"

#include "esp_system.h"
#include "odroid_input.h"
#include "nvs_flash.h"


const char* SD_BASE_PATH = "/sd";

//mkfw.exe "fMSX" tile.raw 0 16 2097152 fMSX goMSX.bin
void app_main(void) {
  
  nvs_flash_init();
 
   uint8_t initOkay;
   uint8_t failure = 0;
   
   
   odroid_system_init();
   
   odroid_input_gamepad_init();
   
   
   esp_err_t r = odroid_sdcard_open(SD_BASE_PATH);
    if (r != ESP_OK)
    {
    //   odroid_display_show_sderr(ODROID_SD_ERR_NOCARD);
        failure = 1;
        printf("ODROID_SD_ERR_NOCARD\n");
    }

   
    
    // Display
   ili9341_prepare();
   ili9341_init();
   
   /// draw frames in %
   UPeriod = 50;

   initOkay = InitMachine();
   
   
   if (initOkay) {
        Mode=(Mode&~MSX_VIDEO)|MSX_PAL;
        Mode=(Mode&~MSX_MODEL)|MSX_MSX2;
        Mode=(Mode&~MSX_JOYSTICKS)|MSX_JOY1;
        
        RAMPages = 2;
        VRAMPages = 2;
        
        StartMSX(Mode,RAMPages,VRAMPages);
        switch (failure) {
           case 1: 
                CONMsg(-1,-1,-1,-1,PIXEL(0,0,0),PIXEL(0,255,0),"Error","Cannot mount SDCard\0\0");
                break;
           default:
                CONMsg(-1,-1,-1,-1,PIXEL(0,0,0),PIXEL(0,255,0),"Error","Start fMSX failed. Missing bios files?\0\0");
        }
        ShowVideo();
       
       
    } 
   
}

