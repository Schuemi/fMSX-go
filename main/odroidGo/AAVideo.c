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
 * File:   Video.c
 * Author: Schuemi
 *
 * Created on 16. August 2018, 14:11
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "odroid_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "LibOdroidGo.h"

#ifndef  BPP16
    #define BPP16
#endif

#include "ImageMux.h"
#include "MSX.h"
#include "EMULib.h"

#define VERBOSE_VIDEO
#define WITH_OVERLAY

int cursorX = 10;
int cursorY = 10;
byte* ZBuf;

uint16_t lastBGColor = 0;
enum cursorTypeEnum{
    CURSOR_ARROW,
    CURSOR_HAND
};
enum cursorTypeEnum cursorType = CURSOR_ARROW;

#define KEYS_COUNT 73
struct keyPosition{
    uint16_t x1; 
    uint16_t y1; 
    uint16_t x2; 
    uint16_t y2;
    uint16_t key;
};

static const struct {
    uint8_t keys;
    struct keyPosition kyPos[KEYS_COUNT];
    
} keyPositions = {
    KEYS_COUNT,
   //row 1, 8 keys
    24,7,55,20,KBD_F1,
    60,7,90,20,KBD_F2,
    94,7,125,20,KBD_F3,
    130,7,159,20,KBD_F4,
    165,7,195,20,KBD_F5,
    235,7,266,20,KBD_STOP,
    274,7,288,20,KBD_INSERT,
    292,7,306,20,KBD_DELETE,
   // row 2, 17 keys
    5,24,20,38,KBD_ESCAPE,
    24,24,36,38,'1',
    41,24,53,38,'2',
    60,24,71,38,'3',
    76,24,89,38,'4',
    94,24,107,38,'5',
    111,24,124,38,'6',
    130,24,142,38,'7',
    147,24,160,38,'8',
    164,24,176,38,'9',
    184,24,194,38,'0',
    199,24,211,38,'-',
    215,24,229,38,'=',
    234,24,246,38,'\\',
    251,24,263,38,KBD_BS,
    276,24,288,38,KBD_SELECT,
    292,24,307,38,KBD_HOME,
   // row 3, 14 
    9,42,28,55,KBD_TAB,
    31,42,44,55,'Q',
    50,42,62,55,'W',
    66,42,79,55,'E',
    85,42,97,55,'R',
    101,42,114,55,'T',
    120,42,132,55,'Y',
    137,42,151,55,'U',
    155,42,169,55,'I',
    174,42,186,55,'O',
    191,42,204,55,'P',
    208,42,222,55,'[',
    225,42,238,55,']',
    245,42,265,73,KBD_ENTER,
  // row 4, 14 
    12,60,30,73,KBD_TAB,
    35,60,48,73,'A',
    53,60,66,73,'S',
    70,60,83,73,'D',
    88,60,102,73,'F',
    106,60,120,73,'G',
    124,60,137,73,'H',
    142,60,155,73,'J',
    159,60,173,73,'K',
    177,60,189,73,'L',
    194,60,207,73,':',
    212,60,225,73,'"',
    230,60,242,73,'~',
    284,60,301,73,KBD_UP,
  // row 5, 15 
    16,76,39,88,KBD_SHIFT,       
    43,76,56,88,'Z',       
    62,76,75,88,'X',       
    80,76,93,88,'C',       
    97,76,110,88,'V',       
    116,76,128,88,'B',       
    133,76,145,88,'N',       
    152,76,164,88,'M',       
    168,76,180,88,'<',       
    185,76,200,88,'>',       
    205,76,216,88,'?',       
    222,76,244,88,KBD_SHIFT,       
    249,76,260,88,'$',       
    273,76,291,88,KBD_LEFT,       
    292,76,313,88,KBD_RIGHT,    
    
  // row 6, 5 
    42,95,55,108,KBD_CAPSLOCK,       
    61,95,73,108,KBD_GRAPH,       
    78,95,199,108,KBD_SPACE,       
    203,95,218,108,KBD_GRAPH,       
    285,95,305,108,KBD_DOWN,       
    
    
};




#define CURSOR_MAX_WIDTH (22)
#define CURSOR_MAX_HEIGHT (22)

#define ILI9341_COLOR(r, g, b)\
	(((uint16_t)b) >> 3) |\
	       ((((uint16_t)g) << 3) & 0x07E0) |\
	       ((((uint16_t)r) << 8) & 0xf800)


uint8_t* msxFramebuffer;
pixel* cursor;

pixel* cursorBackGround;
int backGroundX = -1;
int backGroundY = -1;
int backGroundHeight = -1;
int backGroundWidth = -1;

QueueHandle_t videoQueue;
uint16_t VideoTaskCommand = 1;
static uint16_t* BPal;
static uint16_t* XPal;
static XPal0; 
Image overlay;
uint32_t FirstLine = 18;
uint16_t lastLine = 0;

char showKeyboard = 0;
char reDrawKeyboard = 0;
char reDrawCursor = 0;
char flipScreen = 0;

//////////////////////////////////////////////////////7
 

void videoTask(void* arg)
{
    // sound
  uint16_t* param;
 
  
 
  
  while(1)
  {
    xQueuePeek(videoQueue, &param, portMAX_DELAY);
    uint16_t* palette = XPal;
    
    if (ScrMode == 10 || ScrMode == 12 || ScrMode == 8) palette = BPal;
    
    if (VideoTaskCommand == 1 || lastBGColor != XPal[BGColor]) {// clear screen first
       if (! showKeyboard) 
         ili9341_write_frame_msx(0,0,WIDTH_OVERLAY,HEIGHT_OVERLAY, NULL, XPal[BGColor], palette);
       else
         ili9341_write_frame_msx(0,0,WIDTH_OVERLAY,HEIGHT_OVERLAY/2, NULL, XPal[BGColor], palette);
       
       lastBGColor = XPal[BGColor];
     }
     VideoTaskCommand = 0;
   
     if (! showKeyboard) {
          ili9341_write_frame_msx(MSX_DISPLAY_X, MSX_DISPLAY_Y  ,WIDTH,HEIGHT, msxFramebuffer, XPal[BGColor], palette);
     } else {
         if (! flipScreen)
           ili9341_write_frame_msx(MSX_DISPLAY_X, MSX_DISPLAY_Y  ,WIDTH,HEIGHT/2, msxFramebuffer, XPal[BGColor], palette);
        else
           ili9341_write_frame_msx(MSX_DISPLAY_X, MSX_DISPLAY_Y  ,WIDTH,HEIGHT/2, msxFramebuffer + WIDTH*(HEIGHT/2), XPal[BGColor], palette);
     }
     
    if (showKeyboard && (reDrawKeyboard || reDrawCursor)) {
        pixel* cp;
        pixel* bg;    
        
        const pixel* keyb = (pixel*)(keyboard_image.pixel_data + keyboard_image.width*2);
        const pixel* op;
        int imageWidth;
        int imageHeight;
       
        if (cursorType == CURSOR_ARROW){
            op = (pixel*)cursor_image.pixel_data;
            imageWidth = cursor_image.width;
            imageHeight = cursor_image.height;
        } else{
            op = (pixel*)hand_image.pixel_data;
            imageWidth = hand_image.width;
            imageHeight = hand_image.height;
           
        }

        
        int cursorWidth = imageWidth;
        int cursorHeight = imageHeight;
        if (cursorHeight + cursorY > keyboard_image.height) cursorHeight = keyboard_image.height - cursorY;
        // recover background
        if (backGroundX != -1){
            ili9341_write_frame_rectangleLE(backGroundX, HEIGHT_OVERLAY/2 + backGroundY, backGroundWidth, backGroundHeight, (uint16_t*)cursorBackGround);
        }
        // copy backgroud in cursor and background
        cp = cursor;
        bg = cursorBackGround;
        for (int y = 0; y < cursorHeight; y++) {
            for (int x = 0; x < cursorWidth; x++) {
                pixel p = keyb[(cursorX+x) + ((cursorY + y)*keyboard_image.width)];
                *cp++ = p;
                *bg++ = p;
            }
        }
       
        
        backGroundX = cursorX;
        backGroundY = cursorY;
        backGroundWidth = cursorWidth;
        backGroundHeight = cursorHeight;
               
        cp = cursor;
        for (int y = 0; y < imageHeight; y++) {
            for (int x = 0; x < imageWidth; x++) {
                pixel p = *op++;
                if (p != 0xFFFF && y > 0) *cp++ = p; else cp++; // TODO: had a problem with gimp: the first row is not right, don't know why.
            }
        }
        // TODO: the same in my keyboard image. the first row is crap, so i down't draw it. have to find out why GIMP's first line is crap...
        if (reDrawKeyboard) ili9341_write_frame_rectangleLE(0, HEIGHT_OVERLAY/2, keyboard_image.width, keyboard_image.height - 1, (uint16_t*)(keyboard_image.pixel_data + keyboard_image.width*2));
        ili9341_write_frame_rectangleLE(cursorX, HEIGHT_OVERLAY/2 + cursorY, cursorWidth, cursorHeight, (uint16_t*)cursor);
        reDrawKeyboard = 0;
        reDrawCursor = 0;
    }
    
    xQueueReceive(videoQueue, &param, portMAX_DELAY);
  }
#ifdef VERBOSE_VIDEO
  printf("videoTask: exiting.\n");
#endif
  

  vTaskDelete(NULL);

  while (1) {}
}

///////////////////////////////////////////////////////


int InitVideo(void) {
    videoQueue = xQueueCreate(1, sizeof(uint16_t*));
    
    BPal = heap_caps_malloc(256*sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    XPal = heap_caps_malloc(80*sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    ZBuf = heap_caps_malloc(320, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    

    msxFramebuffer = (uint8_t*)heap_caps_malloc(WIDTH*HEIGHT*sizeof(uint8_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    memset(msxFramebuffer, 0, WIDTH*HEIGHT);
    if (!msxFramebuffer){ printf("malloc msxFramebuffer failed!\n"); return 0; }
    
    cursor = (pixel*)heap_caps_malloc(CURSOR_MAX_WIDTH*CURSOR_MAX_HEIGHT*sizeof(pixel), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!cursor){ printf("malloc cursor failed!\n"); return 0; }
    
    cursorBackGround= (pixel*)heap_caps_malloc(CURSOR_MAX_WIDTH*CURSOR_MAX_HEIGHT*sizeof(pixel), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!cursorBackGround){ printf("malloc cursorBackGround failed!\n"); return 0; }
    
#ifdef WITH_OVERLAY
     /* Menu overlay*/
    overlay.Data = (pixel*)heap_caps_malloc(WIDTH_OVERLAY*HEIGHT_OVERLAY*sizeof(pixel), MALLOC_CAP_DEFAULT);
    if (!overlay.Data){ printf("malloc overlay failed!\n"); return 0; }
    
    memset(overlay.Data, 0, WIDTH_OVERLAY*HEIGHT_OVERLAY*sizeof(pixel));
    overlay.Cropped = 1;
    overlay.H = HEIGHT_OVERLAY;
    overlay.W = WIDTH_OVERLAY;
    overlay.D = sizeof(pixel)<<3;
    overlay.L = WIDTH_OVERLAY;
    SetVideo(&overlay,0,0,WIDTH_OVERLAY,HEIGHT_OVERLAY);
#endif   
    
    /* Reset the palette */
    for(int J=0;J<16;J++) XPal[J]=255*J;
    XPal0=Black;
    
    
    /* Create SCREEN8 palette (GGGRRRBB) */
      for(int J=0;J<256;J++) {
        uint16_t r = (0xFF / 7) * (((J&0x1C)>> 2));
        uint16_t g = (0xFF / 7) * (((J&0xE0)>> 5));
        uint16_t b = J & 0x03;
        if (b == 1) b = 73;
        if (b == 2) b = 146;
        if (b == 3) b = 255;
        
        BPal[J]=ILI9341_COLOR(r,g,b);
        BPal[J]=( BPal[J] << 8) | (( BPal[J] >> 8) & 0xFF);
        
      }
      XPal[15] = Black;
    
     // clear screen
    ili9341_write_frame_msx(0,0,WIDTH_OVERLAY,HEIGHT_OVERLAY, NULL, XPal[BGColor], XPal);
    
    
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 5, NULL, 1);
    
    
    
    
}
void TrashVideo(void){
    
    free(msxFramebuffer);
    free(overlay.Data);
}
 
void clearScreen(void) {
    VideoTaskCommand = 1;
}
void clearOverlay(void) {
    memset(overlay.Data,0,WIDTH_OVERLAY*HEIGHT_OVERLAY*sizeof(pixel));
}

/** SetColor() ***********************************************/
/** Set color N (0..15) to (R,G,B).                         **/
/************************************ TO BE WRITTEN BY USER **/
void SetColor(byte N,byte R,byte G,byte B) {
    register uint16_t col = ILI9341_COLOR(R,G,B);
    col=( col << 8) | (( col >> 8) & 0xFF);
    if(N) XPal[N]=col; else XPal0=col;
}



/** YJKColor() ***********************************************/
/** Given a color in YJK format, return the corresponding   **/
/** palette entry.                                          **/
/*************************************************************/
uint8_t YJKColor(register int Y,register int J,register int K)
{
  register int R,G,B;
		
  R=Y+J;
  G=Y+K;
  B=((5*Y-2*J-K)/4);

  R=R<0? 0:R>31? 31:R;
  G=G<0? 0:G>31? 31:G;
  B=B<0? 0:B>31? 31:B;

  return((R&0x1C)|((G&0x1C)<<3)|(B>>3)); // BPAL
}



/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/************************************ TO BE WRITTEN BY USER **/

int64_t mtimer;
#ifdef VERBOSE_VIDEO
char scounter = 0; 
#endif

void RefreshScreen(void) {
   
  
    
    for (register int y = lastLine+1; y < HEIGHT; y++) {
        for (register int w = 0; w < WIDTH; w++) {
            msxFramebuffer[w+(y*WIDTH)]= BGColor;
        }
    }
   xQueueSend(videoQueue, (void*)&VideoTaskCommand, portMAX_DELAY);
  #ifdef VERBOSE_VIDEO
   scounter++;
    if (scounter == 10) {
       // printf("%llu fps\n", 1000000 / ((esp_timer_get_time() - mtimer) / 10));
        printf("%llu Screen %d\n", (esp_timer_get_time() - mtimer) / 10, ScrMode);
        mtimer = esp_timer_get_time();
        scounter = 0;

    }
    #endif
}

/** ClearLine() **********************************************/
/** Clear 256 pixels from P with color C.                   **/
/*************************************************************/
static void ClearLine(register uint8_t *P,register uint8_t C)
{
  register int J;

  for(J=0;J<256;J++) P[J]=C;
}

/** ColorSprites() *******************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** color sprites in SCREENs 4-8. The result is returned in **/
/** ZBuf, whose size must be 320 bytes (32+256+32).         **/
/*************************************************************/
void ColorSprites(register byte Y,byte *ZBuf)
{
  static const byte SprHeights[4] = { 8,16,16,32 };
  register byte C,IH,OH,J,OrThem;
  register byte *P,*PT,*AT;
  register int L,K;
  register unsigned int M;

  /* No extra sprites yet */
  VDPStatus[0]&=~0x5F;

  /* Clear ZBuffer and exit if sprites are off */
  memset(ZBuf+32,0,256);
  if(SpritesOFF) return;

  /* Assign initial values before counting */
  OrThem = 0x00;
  OH = SprHeights[VDP[1]&0x03];
  IH = SprHeights[VDP[1]&0x02];
  AT = SprTab-4;
  C  = MAXSPRITE2+1;
  M  = 0;

  /* Count displayed sprites */
  for(L=0;L<32;++L)
  {
    M<<=1;AT+=4;              /* Iterating through SprTab      */
    K=AT[0];                  /* Read Y from SprTab            */
    if(K==216) break;         /* Iteration terminates if Y=216 */
    K=(byte)(K-VScroll);      /* Sprite's actual Y coordinate  */
    if(K>256-IH) K-=256;       /* Y coordinate may be negative  */

    /* Mark all valid sprites with 1s, break at MAXSPRITE2 sprites */
    if((Y>K)&&(Y<=K+OH))
    {
      /* If we exceed the maximum number of sprites per line... */
      if(!--C)
      {
        /* Set 9thSprite flag in the VDP status register */
        VDPStatus[0]|=0x40;
        /* Stop drawing sprites, unless all-sprites option enabled */
        if(!OPTION(MSX_ALLSPRITE)) break;
      }

      /* Mark sprite as ready to draw */
      M|=1;
    }
  }

  /* Mark last checked sprite (9th in line, Y=216, or sprite #31) */
  VDPStatus[0]|=L<32? L:31;

  /* Draw all marked sprites */
  for(;M;M>>=1,AT-=4)
    if(M&1)
    {
      K = (byte)(AT[0]-VScroll);  /* K = sprite Y coordinate */
      if(K>256-IH) K-=256;        /* Y coordinate may be negative */

      J = Y-K-1;
      J = OH>IH? (J>>1):J;
      C = SprTab[-0x0200+((AT-SprTab)<<2)+J];
      OrThem|=C&0x40;

      if(C&0x0F)
      {
        PT = SprGen+((int)(IH>8? AT[2]&0xFC:AT[2])<<3)+J;
        P  = ZBuf+AT[1]+(C&0x80? 0:32);
        C &= 0x0F;
        J  = PT[0];

        if(OrThem&0x20)
        {
          if(OH>IH)
          {
            if(J&0x80) { P[0]|=C;P[1]|=C; }
            if(J&0x40) { P[2]|=C;P[3]|=C; }
            if(J&0x20) { P[4]|=C;P[5]|=C; }
            if(J&0x10) { P[6]|=C;P[7]|=C; }
            if(J&0x08) { P[8]|=C;P[9]|=C; }
            if(J&0x04) { P[10]|=C;P[11]|=C; }
            if(J&0x02) { P[12]|=C;P[13]|=C; }
            if(J&0x01) { P[14]|=C;P[15]|=C; }
            if(IH>8)
            {
              J=PT[16];
              if(J&0x80) { P[16]|=C;P[17]|=C; }
              if(J&0x40) { P[18]|=C;P[19]|=C; }
              if(J&0x20) { P[20]|=C;P[21]|=C; }
              if(J&0x10) { P[22]|=C;P[23]|=C; }
              if(J&0x08) { P[24]|=C;P[25]|=C; }
              if(J&0x04) { P[26]|=C;P[27]|=C; }
              if(J&0x02) { P[28]|=C;P[29]|=C; }
              if(J&0x01) { P[30]|=C;P[31]|=C; }
            }
          }
          else
          {
            if(J&0x80) P[0]|=C;
            if(J&0x40) P[1]|=C;
            if(J&0x20) P[2]|=C;
            if(J&0x10) P[3]|=C;
            if(J&0x08) P[4]|=C;
            if(J&0x04) P[5]|=C;
            if(J&0x02) P[6]|=C;
            if(J&0x01) P[7]|=C;
            if(IH>8)
            {
              J=PT[16];
              if(J&0x80) P[8]|=C;
              if(J&0x40) P[9]|=C;
              if(J&0x20) P[10]|=C;
              if(J&0x10) P[11]|=C;
              if(J&0x08) P[12]|=C;
              if(J&0x04) P[13]|=C;
              if(J&0x02) P[14]|=C;
              if(J&0x01) P[15]|=C;
            }
          }
        }
        else
        {
          if(OH>IH)
          {
            if(J&0x80) P[0]=P[1]=C;
            if(J&0x40) P[2]=P[3]=C;
            if(J&0x20) P[4]=P[5]=C;
            if(J&0x10) P[6]=P[7]=C;
            if(J&0x08) P[8]=P[9]=C;
            if(J&0x04) P[10]=P[11]=C;
            if(J&0x02) P[12]=P[13]=C;
            if(J&0x01) P[14]=P[15]=C;
            if(IH>8)
            {
              J=PT[16];
              if(J&0x80) P[16]=P[17]=C;
              if(J&0x40) P[18]=P[19]=C;
              if(J&0x20) P[20]=P[21]=C;
              if(J&0x10) P[22]=P[23]=C;
              if(J&0x08) P[24]=P[25]=C;
              if(J&0x04) P[26]=P[27]=C;
              if(J&0x02) P[28]=P[29]=C;
              if(J&0x01) P[30]=P[31]=C;
            }
          }
          else
          {
            if(J&0x80) P[0]=C;
            if(J&0x40) P[1]=C;
            if(J&0x20) P[2]=C;
            if(J&0x10) P[3]=C;
            if(J&0x08) P[4]=C;
            if(J&0x04) P[5]=C;
            if(J&0x02) P[6]=C;
            if(J&0x01) P[7]=C;
            if(IH>8)
            {
              J=PT[16];
              if(J&0x80) P[8]=C;
              if(J&0x40) P[9]=C;
              if(J&0x20) P[10]=C;
              if(J&0x10) P[11]=C;
              if(J&0x08) P[12]=C;
              if(J&0x04) P[13]=C;
              if(J&0x02) P[14]=C;
              if(J&0x01) P[15]=C;
            }
          }
        }
      }

      /* Update overlapping flag */
      OrThem>>=1;
    }
}


/** Sprites() ************************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** sprites in SCREENs 1-3.                                 **/
/*************************************************************/
void Sprites(register byte Y,register uint8_t *Line)
{
  static const byte SprHeights[4] = { 8,16,16,32 };
  register uint8_t *P,C;
  register byte OH,IH,*PT,*AT;
  register unsigned int M;
  register int L,K;

  /* No extra sprites yet */
  VDPStatus[0]&=~0x5F;

  /* Assign initial values before counting */
  OH = SprHeights[VDP[1]&0x03];
  IH = SprHeights[VDP[1]&0x02];
  AT = SprTab-4;
  Y += VScroll;
  C  = MAXSPRITE1+1;
  M  = 0;

  /* Count displayed sprites */
  for(L=0;L<32;++L)
  {
    M<<=1;AT+=4;        /* Iterating through SprTab      */
    K=AT[0];            /* K = sprite Y coordinate       */
    if(K==208) break;   /* Iteration terminates if Y=208 */
    if(K>256-IH) K-=256; /* Y coordinate may be negative  */

    /* Mark all valid sprites with 1s, break at MAXSPRITE1 sprites */
    if((Y>K)&&(Y<=K+OH))
    {
      /* If we exceed the maximum number of sprites per line... */
      if(!--C)
      {
        /* Set 5thSprite flag in the VDP status register */
        VDPStatus[0]|=0x40;
        /* Stop drawing sprites, unless all-sprites option enabled */
        if(!OPTION(MSX_ALLSPRITE)) break;
      }

      /* Mark sprite as ready to draw */
      M|=1;
    }
  }

  /* Mark last checked sprite (5th in line, Y=208, or sprite #31) */
  VDPStatus[0]|=L<32? L:31;

  /* Draw all marked sprites */
  for(;M;M>>=1,AT-=4)
    if(M&1)
    {
      C = AT[3];                  /* C = sprite attributes */
      L = C&0x80? AT[1]-32:AT[1]; /* Sprite may be shifted left by 32 */
      C&= 0x0F;                   /* C = sprite color */

      if((L<256)&&(L>-OH)&&C)
      {
        K = AT[0];                /* K = sprite Y coordinate */
        if(K>256-IH) K-=256;      /* Y coordinate may be negative */

        P  = Line+L;
        K  = Y-K-1;
        PT = SprGen+((int)(IH>8? AT[2]&0xFC:AT[2])<<3)+(OH>IH? (K>>1):K);
       

        /* Mask 1: clip left sprite boundary */
        K=L>=0? 0xFFFF:(0x10000>>(OH>IH? (-L>>1):-L))-1;

        /* Mask 2: clip right sprite boundary */
        L+=(int)OH-257;
        if(L>=0)
        {
          L=(IH>8? 0x0002:0x0200)<<(OH>IH? (L>>1):L);
          K&=~(L-1);
        }

        /* Get and clip the sprite data */
        K&=((int)PT[0]<<8)|(IH>8? PT[16]:0x00);

        /* If output size is bigger than the input size... */
        if(OH>IH)
        {
          /* Big (zoomed) sprite */

          /* Draw left 16 pixels of the sprite */
          if(K&0xFF00)
          {
            if(K&0x8000) P[1]=P[0]=C;
            if(K&0x4000) P[3]=P[2]=C;
            if(K&0x2000) P[5]=P[4]=C;
            if(K&0x1000) P[7]=P[6]=C;
            if(K&0x0800) P[9]=P[8]=C;
            if(K&0x0400) P[11]=P[10]=C;
            if(K&0x0200) P[13]=P[12]=C;
            if(K&0x0100) P[15]=P[14]=C;
          }

          /* Draw right 16 pixels of the sprite */
          if(K&0x00FF)
          {
            if(K&0x0080) P[17]=P[16]=C;
            if(K&0x0040) P[19]=P[18]=C;
            if(K&0x0020) P[21]=P[20]=C;
            if(K&0x0010) P[23]=P[22]=C;
            if(K&0x0008) P[25]=P[24]=C;
            if(K&0x0004) P[27]=P[26]=C;
            if(K&0x0002) P[29]=P[28]=C;
            if(K&0x0001) P[31]=P[30]=C;
          }
        }
        else
        {
          /* Normal (unzoomed) sprite */

          /* Draw left 8 pixels of the sprite */
          if(K&0xFF00)
          {
            if(K&0x8000) P[0]=C;
            if(K&0x4000) P[1]=C;
            if(K&0x2000) P[2]=C;
            if(K&0x1000) P[3]=C;
            if(K&0x0800) P[4]=C;
            if(K&0x0400) P[5]=C;
            if(K&0x0200) P[6]=C;
            if(K&0x0100) P[7]=C;
          }
   
          /* Draw right 8 pixels of the sprite */
          if(K&0x00FF)
          {
            if(K&0x0080) P[8]=C;
            if(K&0x0040) P[9]=C;
            if(K&0x0020) P[10]=C;
            if(K&0x0010) P[11]=C;
            if(K&0x0008) P[12]=C;
            if(K&0x0004) P[13]=C;
            if(K&0x0002) P[14]=C;
            if(K&0x0001) P[15]=C;
          }
        }
      }
    }
}

uint8_t *GetBuffer(register byte Y,register uint8_t C, register int M)
{
    
    if(!Y){
        register int H; 
        FirstLine=(ScanLines212? 8:18)+VAdjust;
        for(H=WIDTH*FirstLine-1;H>=0;H--) msxFramebuffer[H]=C;
    }
     
    /* Return 0 if we've run out of the screen buffer due to overscan */
  if(Y+FirstLine>=HEIGHT){return(0);}
  lastLine = Y+FirstLine;
      
  int16_t ln = lastLine;
  if (ln < 0) ln = 0;


  /* Set up the transparent color */
  XPal[0]=(!BGColor||SolidColor0)? XPal0:XPal[BGColor];
  uint8_t* P = &msxFramebuffer[ln * WIDTH];
 
  /* Paint left/right borders */
  for(register int H=(WIDTH-256)/2+HAdjust;H>0;H--) P[H-1]=C;
  for(register int H=(WIDTH-256)/2-HAdjust;H>0;H--) P[WIDTH-H]=C;
  
 
  return((&msxFramebuffer[ln * WIDTH])+(WIDTH-256)/2+HAdjust);
}

/** RefreshLineTx80() ****************************************/
/** Refresh line Y (0..191/211) of TEXT80.                  **/
/*************************************************************/
void RefreshLineTx80(byte Y)  // XPal
{
   
    register uint8_t *P,FC,BC;
  register byte X,M,*T,*C,*G;

 
  P=GetBuffer(Y,BGColor, 80);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BC);
  else
  {
    P[0]=P[1]=P[2]=P[3]=P[4]=P[5]=P[6]=P[7]=P[8]=BC;
    G=(FontBuf&&(Mode&MSX_FIXEDFONT)? FontBuf:ChrGen)+((Y+VScroll)&0x07);
    T=ChrTab+((80*(Y>>3))&ChrTabM);
    C=ColTab+((10*(Y>>3))&ColTabM);
    P+=9;

    for(X=0,M=0x00;X<80;X++,T++,P+=3)
    {
      if(!(X&0x07)) M=*C++;
      if(M&0x80) { FC=XFGColor;BC=XBGColor; }
      else       { FC=FGColor;BC=BGColor; }
      M<<=1;
      Y=*(G+((int)*T<<3));
      P[0]=Y&0xC0? FC:BC;
      P[1]=Y&0x30? FC:BC;
      P[2]=Y&0x0C? FC:BC;
    }

    P[0]=P[1]=P[2]=P[3]=P[4]=P[5]=P[6]=BGColor;
  }

}

void RefreshLine0(byte Y) { // XPal
     
  register uint8_t *P,FC,BC;
  register byte X,*T,*G;
  byte line = Y;
  BC=BGColor;
  P=GetBuffer(Y,BC, 0);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BC);
  else
  {
    P[0]=P[1]=P[2]=P[3]=P[4]=P[5]=P[6]=P[7]=P[8]=BC;

    G=(FontBuf&&(Mode&MSX_FIXEDFONT)? FontBuf:ChrGen)+((Y+VScroll)&0x07);
    T=ChrTab+40*(Y>>3);
    FC=FGColor;
    P+=9;

    for(X=0;X<40;X++,T++,P+=6)
    {
      Y=G[(int)*T<<3];
      P[0]=Y&0x80? FC:BC;P[1]=Y&0x40? FC:BC;
      P[2]=Y&0x20? FC:BC;P[3]=Y&0x10? FC:BC;
      P[4]=Y&0x08? FC:BC;P[5]=Y&0x04? FC:BC;
    }

    P[0]=P[1]=P[2]=P[3]=P[4]=P[5]=P[6]=BC;
  }

}
void RefreshLine1(register byte Y) // XPal
{
  register uint8_t *P,FC,BC;
  register byte K,X,*T,*G;
  byte line = Y;
  P=GetBuffer(Y,BGColor, 1);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BGColor);
  else
  {
    Y+=VScroll;
    G=(FontBuf&&(Mode&MSX_FIXEDFONT)? FontBuf:ChrGen)+(Y&0x07);
    T=ChrTab+((int)(Y&0xF8)<<2);

    for(X=0;X<32;X++,T++,P+=8)
    {
      K=ColTab[*T>>3];
      FC=K>>4;
      BC=K&0x0F;
      K=G[(int)*T<<3];
      P[0]=K&0x80? FC:BC;P[1]=K&0x40? FC:BC;
      P[2]=K&0x20? FC:BC;P[3]=K&0x10? FC:BC;
      P[4]=K&0x08? FC:BC;P[5]=K&0x04? FC:BC;
      P[6]=K&0x02? FC:BC;P[7]=K&0x01? FC:BC;
    }

    if(!SpritesOFF) Sprites(Y,P-256);
  }

}

/** RefreshLine2() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN2, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine2(register byte Y) // XPal
{
  register uint8_t *P,FC,BC;
  register byte K,X,*T;
  register int I,J;
 
  P=GetBuffer(Y,BGColor, 2);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BGColor);
  else
  {
    Y+=VScroll;
    T=ChrTab+((int)(Y&0xF8)<<2);
    I=((int)(Y&0xC0)<<5)+(Y&0x07);

    for(X=0;X<32;X++,T++,P+=8)
    {
      J=(int)*T<<3;
      K=ColTab[(I+J)&ColTabM];
      FC=K>>4;
      BC=K&0x0F;
      K=ChrGen[(I+J)&ChrGenM];
      P[0]=K&0x80? FC:BC;P[1]=K&0x40? FC:BC;
      P[2]=K&0x20? FC:BC;P[3]=K&0x10? FC:BC;
      P[4]=K&0x08? FC:BC;P[5]=K&0x04? FC:BC;
      P[6]=K&0x02? FC:BC;P[7]=K&0x01? FC:BC;
    }

    if(!SpritesOFF) Sprites(Y,P-256);
  }

}

/** RefreshLine3() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN3, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine3(register byte Y)// XPal
{
  register uint8_t *P;
  register byte X,K,*T,*G;
  byte line = Y;
  P=GetBuffer(Y,BGColor, 3);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BGColor);
  else
  {
    Y+=VScroll;
    T=ChrTab+((int)(Y&0xF8)<<2);
    G=ChrGen+((Y&0x1C)>>2);

    for(X=0;X<32;X++,T++,P+=8)
    {
      K=G[(int)*T<<3];
      P[0]=P[1]=P[2]=P[3]=K>>4;
      P[4]=P[5]=P[6]=P[7]=K&0x0F;
    }

    if(!SpritesOFF) Sprites(Y,P-256);
  }

}

/** RefreshLine4() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN4, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine4(register byte Y) // XPal
{
  register uint8_t *P,FC,BC;
  register byte K,X,C,*T,*R;
  register int I,J;
  


  P=GetBuffer(Y,BGColor, 4);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BGColor);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    Y+=VScroll;
    T=ChrTab+((int)(Y&0xF8)<<2);
    I=((int)(Y&0xC0)<<5)+(Y&0x07);

    for(X=0;X<32;X++,R+=8,P+=8,T++)
    {
      J=(int)*T<<3;
      K=ColTab[(I+J)&ColTabM];
      FC=K>>4;
      BC=K&0x0F;
      K=ChrGen[(I+J)&ChrGenM];

      C=R[0];P[0]=C? C:(K&0x80)? FC:BC;
      C=R[1];P[1]=C? C:(K&0x40)? FC:BC;
      C=R[2];P[2]=C? C:(K&0x20)? FC:BC;
      C=R[3];P[3]=C? C:(K&0x10)? FC:BC;
      C=R[4];P[4]=C? C:(K&0x08)? FC:BC;
      C=R[5];P[5]=C? C:(K&0x04)? FC:BC;
      C=R[6];P[6]=C? C:(K&0x02)? FC:BC;
      C=R[7];P[7]=C? C:(K&0x01)? FC:BC;
    }
  }

}

/** RefreshLine5() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN5, including       **/
/** sprites in this line.                                   **/
/*************************************************************/

void RefreshLine5(register byte Y)  //XPal
{
  register uint8_t *P;
  register byte I,X,*T,*R;
  

  P=GetBuffer(Y,BGColor, 5);
  if(!P) return;
   
  if(!ScreenON)
      ClearLine(P,BGColor);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<7)&ChrTabM&0x7FFF);

    for(X=0;X<16;X++,R+=16,P+=16,T+=8)
    {
      I=R[0];P[0]=I? I:T[0]>>4;
      I=R[1];P[1]=I? I:T[0]&0x0F;
      I=R[2];P[2]=I? I:T[1]>>4;
      I=R[3];P[3]=I? I:T[1]&0x0F;
      I=R[4];P[4]=I? I:T[2]>>4;
      I=R[5];P[5]=I? I:T[2]&0x0F;
      I=R[6];P[6]=I? I:T[3]>>4;
      I=R[7];P[7]=I? I:T[3]&0x0F;
      I=R[8];P[8]=I? I:T[4]>>4;
      I=R[9];P[9]=I? I:T[4]&0x0F;
      I=R[10];P[10]=I? I:T[5]>>4;
      I=R[11];P[11]=I? I:T[5]&0x0F;
      I=R[12];P[12]=I? I:T[6]>>4;
      I=R[13];P[13]=I? I:T[6]&0x0F;
      I=R[14];P[14]=I? I:T[7]>>4;
      I=R[15];P[15]=I? I:T[7]&0x0F;
    }
  }

}

/** RefreshLine8() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN8, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine8(register byte Y) //BPal
{
    
    static byte SprToScr[16] =
  {
    0x00,0x02,0x10,0x12,0x80,0x82,0x90,0x92,
    0x49,0x4B,0x59,0x5B,0xC9,0xCB,0xD9,0xDB
  };
  register uint8_t *P;
  register byte C,X,*T,*R;


  P=GetBuffer(Y,VDP[7], 8);
  if(!P) return;

  if(!ScreenON) ClearLine(P,VDP[7]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF);

    for(X=0;X<32;X++,T+=8,R+=8,P+=8)
    {
      C=R[0];P[0]=C? SprToScr[C]:T[0];
      C=R[1];P[1]=C? SprToScr[C]:T[1];
      C=R[2];P[2]=C? SprToScr[C]:T[2];
      C=R[3];P[3]=C? SprToScr[C]:T[3];
      C=R[4];P[4]=C? SprToScr[C]:T[4];
      C=R[5];P[5]=C? SprToScr[C]:T[5];
      C=R[6];P[6]=C? SprToScr[C]:T[6];
      C=R[7];P[7]=C? SprToScr[C]:T[7];
    }
  }

}

/** RefreshLine10() ******************************************/
/** Refresh line Y (0..191/211) of SCREEN10/11, including   **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine10(register byte Y) // BPal
{
  
    register uint8_t *P;
  register byte C,X,*T,*R;
  register int J,K;

 
  P=GetBuffer(Y,VDP[7], 10);
  if(!P) return;

  if(!ScreenON) ClearLine(P,VDP[7]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF);

    /* Draw first 4 pixels */
    C=R[0];P[0]=C? C:VDP[7];
    C=R[1];P[1]=C? C:VDP[7];
    C=R[2];P[2]=C? C:VDP[7];
    C=R[3];P[3]=C? C:VDP[7];
    R+=4;P+=4;

    for(X=0;X<63;X++,T+=4,R+=4,P+=4)
    {
      K=(T[0]&0x07)|((T[1]&0x07)<<3);
      if(K&0x20) K-=64;
      J=(T[2]&0x07)|((T[3]&0x07)<<3);
      if(J&0x20) J-=64;

      C=R[0];Y=T[0]>>3;P[0]=C? C:Y&1? Y>>1:YJKColor(Y,J,K);
      C=R[1];Y=T[1]>>3;P[1]=C? C:Y&1? Y>>1:YJKColor(Y,J,K);
      C=R[2];Y=T[2]>>3;P[2]=C? C:Y&1? Y>>1:YJKColor(Y,J,K);
      C=R[3];Y=T[3]>>3;P[3]=C? C:Y&1? Y>>1:YJKColor(Y,J,K);
    }
  }

}


/** RefreshLine12() ******************************************/
/** Refresh line Y (0..191/211) of SCREEN12, including      **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine12(register byte Y) // BPal
{
  
    register uint8_t *P;
  register byte C,X,*T,*R;
  register int J,K;


  P=GetBuffer(Y,VDP[7], 12);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BPal[VDP[7]]);
  else
  {
    ColorSprites(Y,ZBuf);
    R = ZBuf+32;
    T = ChrTab
      + (((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF)
      + (HScroll512&&(HScroll>255)? 0x10000:0)
      + (HScroll&0xFC);

    /* Draw first 4 pixels */
    C=R[0];P[0]=C? C:VDP[7];
    C=R[1];P[1]=C? C:VDP[7];
    C=R[2];P[2]=C? C:VDP[7];
    C=R[3];P[3]=C? C:VDP[7];
    R+=4;P+=4;

    for(X=1;X<64;X++,T+=4,R+=4,P+=4)
    {
      K=(T[0]&0x07)|((T[1]&0x07)<<3);
      if(K&0x20) K-=64;
      J=(T[2]&0x07)|((T[3]&0x07)<<3);
      if(J&0x20) J-=64;

      C=R[0];P[0]=C? C:YJKColor(T[0]>>3,J,K);
      C=R[1];P[1]=C? C:YJKColor(T[1]>>3,J,K);
      C=R[2];P[2]=C? C:YJKColor(T[2]>>3,J,K);
      C=R[3];P[3]=C? C:YJKColor(T[3]>>3,J,K);
    }
  }

}


void RefreshLine6(byte Y) { // XPAL
register uint8_t *P;
  register byte X,*T,*R,C;


  P=GetBuffer(Y,BGColor&0x03, 6);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BGColor&0x03);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<7)&ChrTabM&0x7FFF);

    for(X=0;X<32;X++)
    {
      C=R[0];P[0]=C? C:T[0]>>6;
      C=R[1];P[1]=C? C:(T[0]>>2)&0x03;
      C=R[2];P[2]=C? C:T[1]>>6;
      C=R[3];P[3]=C? C:(T[1]>>2)&0x03;
      C=R[4];P[4]=C? C:T[2]>>6;
      C=R[5];P[5]=C? C:(T[2]>>2)&0x03;
      C=R[6];P[6]=C? C:T[3]>>6;
      C=R[7];P[7]=C? C:(T[3]>>2)&0x03;
      R+=8;P+=8;T+=4;
    }
  }

  

}
/** RefreshLine7() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN7, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine7(register byte Y) // XPAL
{
  register uint8_t *P;
  register byte C,X,*T,*R;


  P=GetBuffer(Y,BGColor, 7);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BGColor);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF);

    for(X=0;X<32;X++)
    {
      C=R[0];P[0]=C? C:T[0]>>4;
      C=R[1];P[1]=C? C:T[1]>>4;
      C=R[2];P[2]=C? C:T[2]>>4;
      C=R[3];P[3]=C? C:T[3]>>4;
      C=R[4];P[4]=C? C:T[4]>>4;
      C=R[5];P[5]=C? C:T[5]>>4;
      C=R[6];P[6]=C? C:T[6]>>4;
      C=R[7];P[7]=C? C:T[7]>>4;
      R+=8;P+=8;T+=8;
    }
  }

}
////////////////// virtual keyboard / cursor /////////////////////
int getKey(int pressX, int pressY) {
    for (int i = 0; i < keyPositions.keys; i++){
        if (keyPositions.kyPos[i].x1<=pressX && keyPositions.kyPos[i].x2>=pressX && keyPositions.kyPos[i].y1<=pressY && keyPositions.kyPos[i].y2>=pressY){
            return keyPositions.kyPos[i].key;
        }
    }
    return -1;
}
void doFlipScreen() {
    if (flipScreen) flipScreen = 0; else flipScreen = 1;
}
int mousePress() {
    return getKey(cursorX + 4, cursorY);
}
void moveCursor(int x, int y) {
    cursorX += x;
    cursorY += y;
    if (cursorX < 2) cursorX = 2;
    if (cursorY < 2) cursorY = 2;
    if (cursorX > WIDTH_OVERLAY - CURSOR_MAX_WIDTH) cursorX = WIDTH_OVERLAY - CURSOR_MAX_WIDTH;
    if (cursorY > HEIGHT_OVERLAY/2 - (CURSOR_MAX_HEIGHT + 1)) cursorY = HEIGHT_OVERLAY/2 - (CURSOR_MAX_HEIGHT + 1);
    
    
    int k = getKey(cursorX + 4, cursorY);
    if (k == -1) cursorType = CURSOR_ARROW; else cursorType = CURSOR_HAND;
    
    reDrawCursor = 1;
}
void showVirtualKeyboard() {
    if (showKeyboard) return;
    showKeyboard = 1;
    reDrawKeyboard = 1;
    
}
void hideVirtualKeyboard() {
    showKeyboard = 0;
    reDrawKeyboard = 0;
    flipScreen = 0;
}

//////////////////// Overlay Menu functions /////////////////////////


/** ShowVideo() **********************************************/
/** Show "active" image at the actual screen or window (overlay).     **/
/*************************************************************/
int ShowVideo(void) {
   
    ili9341_write_frame_rectangleLE(0,0,WIDTH_OVERLAY,HEIGHT_OVERLAY, overlay.Data);
    return 0;
}

int DrawuGui(uint16_t* uGuiMenu, int y) {
    ili9341_write_frame_rectangleLE(0,y,WIDTH_OVERLAY,HEIGHT_OVERLAY - y, uGuiMenu);
    return 0;
}
