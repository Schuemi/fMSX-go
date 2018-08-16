/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   LibOdroidGo.h
 * Author: Test
 *
 * Created on 25. Juli 2018, 09:57
 */

#ifndef LIBODROIDGO_H
#define LIBODROIDGO_H

#define FMSX_CONFIG_FILE "/sd/odroid/data/msx/config.ini"
#define FMSX_ROOT_GAMESDIR "/sd/roms/msx/games"


///////////////// AUDIO ////////////////////////
#define AUDIO_SAMPLE_RATE 32000
#define SND_CHANNELS    16     /* Number of sound channels   */
#define SND_BITS        8
#define SND_BUFSIZE     (1<<SND_BITS)

unsigned int InitAudio(unsigned int Rate,unsigned int Latency);
void audio_volume_set_change(void);
void pause_audio(void);
void restart_audio(void);

//////////////////// VIDEO /////////////////////////////

#define WIDTH   256
#define HEIGHT  216 // normally the msx has 212 lines in PAL, 4 more to send the bytes faster over SPI to the display

#define WIDTH_OVERLAY   320
#define HEIGHT_OVERLAY  240 

#define MSX_DISPLAY_X ((WIDTH_OVERLAY-WIDTH)/2)
#define MSX_DISPLAY_Y ((HEIGHT_OVERLAY-212)/2)


#define Black   0x0000

int InitVideo(void);
void clearScreen(void);
void clearOverlay(void);
void TrashVideo(void);


///////////////////// FILES ////////////////////////////////////
int initFiles();
char* cutExtension(char* file);
const char* getFileName(const char* file);
char* getPath(char* file);

#endif /* LIBODROIDGO_H */

