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


#include "MSX.h"
#include "EMULib.h"
#include <esp_heap_caps.h>
#include "odroid_audio.h"
#include "LibOdroidGo.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "minIni.h"

#include "Sound.h"

#include <string.h>

#define AUDIO_BUFFER_SAMPLES 128


//#define NO_SOUND


static sample *streamAudioBuffer1;
static sample *streamAudioBuffer2;
sample *currentAudioBufferWrite;
sample *currentAudioBufferSend;
uint16_t audioBufferSize;
uint16_t audioBufferSizeSend;
QueueHandle_t audioQueue;
ODROID_AUDIO_SINK sink = ODROID_AUDIO_SINK_NONE;

int volLevel;
char stop = 0;
void audioTask(void* arg)
{
  // sound
  uint16_t* param;

  while(!stop)
  {
    xQueuePeek(audioQueue, &param, portMAX_DELAY);
#ifndef NO_SOUND
   if (! stop) odroid_audio_submit(currentAudioBufferSend, audioBufferSizeSend/2);
#endif
    xQueueReceive(audioQueue, &param, portMAX_DELAY);
  }

  printf("audioTask: exiting.\n");
  odroid_audio_terminate();

  vTaskDelete(NULL);

  
}


unsigned int InitAudio(unsigned int Rate,unsigned int Latency) {
    
    stop = 0;
    char buf[3];
    
    if (sink == ODROID_AUDIO_SINK_NONE) ini_gets("FMSX", "DAC", "0", buf, 3, FMSX_CONFIG_FILE);
    
    if (atoi(buf)) sink = ODROID_AUDIO_SINK_DAC; else sink = ODROID_AUDIO_SINK_SPEAKER;
    
    streamAudioBuffer1 = (sample*)heap_caps_malloc(AUDIO_BUFFER_SAMPLES*sizeof(sample)*2, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    streamAudioBuffer2 = (sample*)heap_caps_malloc(AUDIO_BUFFER_SAMPLES*sizeof(sample)*2, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (! streamAudioBuffer1){ printf("streamAudioBuffer1 failed!\n"); return 0;}
    if (! streamAudioBuffer2){ printf("streamAudioBuffer2 failed!\n"); return 0;}
    currentAudioBufferWrite = streamAudioBuffer1;
    currentAudioBufferSend = streamAudioBuffer2;
    
    audioQueue = xQueueCreate(1, sizeof(uint16_t*));
    
    odroid_audio_init(sink, Rate);
    volLevel = ini_getl("FMSX", "VOLUME", ODROID_VOLUME_LEVEL1, FMSX_CONFIG_FILE);
    
    if (volLevel >= ODROID_VOLUME_LEVEL_COUNT || volLevel < ODROID_VOLUME_LEVEL0) volLevel = ODROID_VOLUME_LEVEL1;
    odroid_audio_volume_set(volLevel);
    
    xTaskCreatePinnedToCore(&audioTask, "audioTask", 2048, NULL, 5, NULL, 1);
    
    return Rate;
}
void TrashAudio(void){
    free(streamAudioBuffer1);
    free(streamAudioBuffer2);
}

void audio_volume_set_change() {
    ets_delay_us(100000); // have to wait a little (other transactions have to finish?)
    volLevel = (volLevel + 1) % ODROID_VOLUME_LEVEL_COUNT;
    odroid_audio_volume_set(volLevel);
    ini_putl("FMSX", "VOLUME", volLevel, FMSX_CONFIG_FILE);
    
}

void pause_audio() {
    void* tempPtr = (void*)0x1234;
    stop=1;
    xQueueSend(audioQueue, &tempPtr, portMAX_DELAY); // to wait until sound was send
    
    TrashAudio();
}

void restart_audio() {
    InitAudio(AUDIO_SAMPLE_RATE, 0);
}

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/

/** PlayAllSound() *******************************************/
/** Render and play given number of microseconds of sound.  **/
/************************************ TO BE WRITTEN BY USER **/
void PlayAllSound(int uSec) {
    
    RenderAndPlayAudio(2*uSec*AUDIO_SAMPLE_RATE/1000000);
    
}
void switchAudioBuffer(){
    if (currentAudioBufferWrite == streamAudioBuffer1){
        currentAudioBufferWrite = streamAudioBuffer2;
        currentAudioBufferSend = streamAudioBuffer1;
    } else {
        currentAudioBufferWrite = streamAudioBuffer1;
        currentAudioBufferSend = streamAudioBuffer2;
    }
    audioBufferSizeSend = audioBufferSize;
    
}


unsigned int WriteAudio(sample *Data,unsigned int Length)
{
    void* tempPtr = (void*)0x1234;
    if (audioBufferSize + Length > AUDIO_BUFFER_SAMPLES) Length = AUDIO_BUFFER_SAMPLES - audioBufferSize;
    
    
    memcpy((currentAudioBufferWrite + audioBufferSize), Data, Length*sizeof(sample));
    audioBufferSize += Length;
    
    
    if (audioBufferSize >= AUDIO_BUFFER_SAMPLES-(Length<<1)) { 
        switchAudioBuffer();
        xQueueSend(audioQueue, &tempPtr, portMAX_DELAY);
        audioBufferSize = 0;
   }
    return Length;
}
unsigned int GetFreeAudio(void)
{
    return AUDIO_BUFFER_SAMPLES;
    
}
