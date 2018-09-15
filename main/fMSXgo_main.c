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
/* 
 * File:   main.cpp
 * Author: Schuemi
 *
 * Created on 25. Juli 2018, 09:38
 */


// used online Font creator: http://dotmatrixtool.com
// set to : 8px by 8px, row major, little endian.

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

#include "LibOdroidGo.h"

#ifndef PIXEL
    #define PIXEL(R,G,B)    (pixel)(((31*(R)/255)<<11)|((63*(G)/255)<<5)|(31*(B)/255))
#endif
const char* SD_BASE_PATH = "/sd";


//mkfw.exe "fMSX" tile.raw 0 16 2097152 fMSX goMSX.bin
void app_main(void) {
    //diese nops sond dafür da, den fMSX code zu verschieben und zu schauen, wo er am besteln liegt um schnell zu sein.
    //AAVideo liegt derzeit noch hinter LibOdroidGo.c, daher beeinfussen änderungen den auch. vielleicht AAVideo in ein eigenes verzeichnis über odroidGo?
    /*    
    
       __asm__ (
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
                "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop\n"
               "nop"
            );*/
       
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
        
        if (!StartMSX(Mode,RAMPages,VRAMPages)) {
            switch (failure) {
               case 1: 
                    odroidFmsxGUI_msgBox("Error","Cannot mount SDCard", 1);
                    break;
               default:
                    odroidFmsxGUI_msgBox("Error","Start fMSX failed.\nMissing bios files?", 1);
            }
            
        }
       
        odroidFmsxGUI_msgBox("End","The emulation was shutted down\nYou can turn off your device", 1);
    } 
   
}

