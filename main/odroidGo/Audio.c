/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
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


int volLevel;

void audioTask(void* arg)
{
  // sound
  uint16_t* param;

  while(1)
  {
    xQueuePeek(audioQueue, &param, portMAX_DELAY);
#ifndef NO_SOUND
   odroid_audio_submit(currentAudioBufferSend, audioBufferSizeSend/2);
#endif
    xQueueReceive(audioQueue, &param, portMAX_DELAY);
  }

  printf("audioTask: exiting.\n");
  odroid_audio_terminate();

  vTaskDelete(NULL);

  while (1) {}
}


unsigned int InitAudio(unsigned int Rate,unsigned int Latency) {
    
    streamAudioBuffer1 = (sample*)heap_caps_malloc(AUDIO_BUFFER_SAMPLES*sizeof(sample)*2, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    streamAudioBuffer2 = (sample*)heap_caps_malloc(AUDIO_BUFFER_SAMPLES*sizeof(sample)*2, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    currentAudioBufferWrite = streamAudioBuffer1;
    currentAudioBufferSend = streamAudioBuffer2;
    
    audioQueue = xQueueCreate(1, sizeof(uint16_t*));
    
    odroid_audio_init(ODROID_AUDIO_SINK_SPEAKER, Rate);
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
    xQueueSend(audioQueue, &tempPtr, portMAX_DELAY); // to wait until sound was send
    odroid_audio_terminate();
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
