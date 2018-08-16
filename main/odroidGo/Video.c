/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Video.c
 * Author: Test
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

#include "MSX.h"
#include "EMULib.h"


#define ILI9341_COLOR(r, g, b)\
	(((uint16_t)b) >> 3) |\
	       ((((uint16_t)g) << 3) & 0x07E0) |\
	       ((((uint16_t)r) << 8) & 0xf800)


pixel* framebuffer[2];
QueueHandle_t videoQueue;
uint16_t VideoTaskCommand;
static uint16_t BPal[256],XPal[80],XPal0; 
Image overlay;
uint32_t FirstLine = 18;
uint16_t lastLine = 0;

void videoTask(void* arg)
{
    // sound
  uint16_t* param;
 
  
 
  
  while(1)
  {
    xQueuePeek(videoQueue, &param, portMAX_DELAY);
     if (VideoTaskCommand == 1) {// clear screen first
       ili9341_write_frame_msx(0,0,WIDTH_OVERLAY,HEIGHT_OVERLAY, NULL, XPal[BGColor]);
     }
     VideoTaskCommand = 0;
   
    ili9341_write_frame_msx(MSX_DISPLAY_X, MSX_DISPLAY_Y                            ,WIDTH,HEIGHT/2, framebuffer[0], XPal[BGColor]);
    ili9341_write_frame_msx(MSX_DISPLAY_X, MSX_DISPLAY_Y + (HEIGHT/2)               ,WIDTH,HEIGHT/2, framebuffer[1], XPal[BGColor]);
    
    xQueueReceive(videoQueue, &param, portMAX_DELAY);
  }

  printf("videoTask: exiting.\n");
  

  vTaskDelete(NULL);

  while (1) {}
}


int InitVideo(void) {
     videoQueue = xQueueCreate(1, sizeof(uint16_t*));
   
    for (int i = 0; i < 2; i++) {
         framebuffer[i] = (pixel*)heap_caps_malloc(WIDTH*(HEIGHT/2)*sizeof(pixel), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
         memset(framebuffer[i], 0, WIDTH*(HEIGHT/2)*sizeof(pixel));
        if (!framebuffer[i]){ printf("malloc framebuffer%d failed!\n", i); return 0; }
    }
 
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
    
    
    /* Reset the palette */
    for(int J=0;J<16;J++) XPal[J]=255*J;
    XPal0=Black;
    
     /* Set SCREEN8 colors */
    for(int J=0;J<64;J++)
    {
      int I=(J&0x03)+(J&0x0C)*16+(J&0x30)/2;
      XPal[J+16]=J*I;
      BPal[I]=BPal[I|0x04]=BPal[I|0x20]=BPal[I|0x24]=XPal[J+16];
    }
    XPal[15] = Black;
    
     // clear screen
    ili9341_write_frame_msx(0,0,WIDTH_OVERLAY,HEIGHT_OVERLAY, NULL, XPal[BGColor]);
    
    
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 5, NULL, 1);
    
    
    
    
}
void TrashVideo(void){
    for (int i = 0; i < 2; i++) {
        free(framebuffer[i]);
    }
    free(overlay.Data);
}
 
void clearScreen(void) {
    ili9341_write_frame_msx(0,0,WIDTH_OVERLAY,HEIGHT_OVERLAY, NULL, XPal[BGColor]);
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
pixel YJKColor(register int Y,register int J,register int K)
{
  register int R,G,B;
		
  R=Y+J;
  G=Y+K;
  B=((5*Y-2*J-K)/4);

  R=R<0? 0:R>31? 31:R;
  G=G<0? 0:G>31? 31:G;
  B=B<0? 0:B>31? 31:B;

  return(BPal[(R&0x1C)|((G&0x1C)<<3)|(B>>3)]);
}



/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/************************************ TO BE WRITTEN BY USER **/

int64_t mtimer;
char scounter = 0; 

void RefreshScreen(void) {
   
  
    
    for (register int y = lastLine+1; y < HEIGHT; y++) {
        for (register int w = 0; w < WIDTH; w++) {
            framebuffer[1][w+((y-(HEIGHT/2))*WIDTH)]= XPal[BGColor];
        }
    }
   xQueueSend(videoQueue, (void*)&VideoTaskCommand, portMAX_DELAY);
   scounter++;
    if (scounter == 10) {
        printf("%llu\n", (esp_timer_get_time() - mtimer) / 10);
        mtimer = esp_timer_get_time();
        scounter = 0;
    }
    
}

/** ClearLine() **********************************************/
/** Clear 256 pixels from P with color C.                   **/
/*************************************************************/
static void ClearLine(register pixel *P,register pixel C)
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
void Sprites(register byte Y,register pixel *Line)
{
  static const byte SprHeights[4] = { 8,16,16,32 };
  register pixel *P,C;
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
        C  = XPal[C];

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

pixel *GetBuffer(register byte Y,register pixel C, register int M)
{
    
    if(!Y){
        register int H; 
        FirstLine=(ScanLines212? 8:18)+VAdjust;
        for(H=WIDTH*FirstLine-1;H>=0;H--) framebuffer[0][H]=C;
    }
     
    /* Return 0 if we've run out of the screen buffer due to overscan */
  if(Y+FirstLine>=HEIGHT){return(0);}
  lastLine = Y+FirstLine;
      
  int16_t ln = lastLine;
  if (ln < 0) ln = 0;
  uint8_t cf = 0;
  if (ln >= HEIGHT/2) {
      ln -= HEIGHT/2;
      cf = 1;
  }
 

  /* Set up the transparent color */
  XPal[0]=(!BGColor||SolidColor0)? XPal0:XPal[BGColor];
  pixel* P = &framebuffer[cf][ln * WIDTH];
 
  /* Paint left/right borders */
  for(register int H=(WIDTH-256)/2+HAdjust;H>0;H--) P[H-1]=C;
  for(register int H=(WIDTH-256)/2-HAdjust;H>0;H--) P[WIDTH-H]=C;
  
 
  return((&framebuffer[cf][ln * WIDTH])+(WIDTH-256)/2+HAdjust);
}

/** RefreshLineTx80() ****************************************/
/** Refresh line Y (0..191/211) of TEXT80.                  **/
/*************************************************************/
void RefreshLineTx80(byte Y) 
{
   
    register pixel *P,FC,BC;
  register byte X,M,*T,*C,*G;

  BC=XPal[BGColor];
  P=GetBuffer(Y,BC, 80);
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
      if(M&0x80) { FC=XPal[XFGColor];BC=XPal[XBGColor]; }
      else       { FC=XPal[FGColor];BC=XPal[BGColor]; }
      M<<=1;
      Y=*(G+((int)*T<<3));
      P[0]=Y&0xC0? FC:BC;
      P[1]=Y&0x30? FC:BC;
      P[2]=Y&0x0C? FC:BC;
    }

    P[0]=P[1]=P[2]=P[3]=P[4]=P[5]=P[6]=XPal[BGColor];
  }

}

void RefreshLine0(byte Y) { 
     
  register pixel *P,FC,BC;
  register byte X,*T,*G;
  byte line = Y;
  BC=XPal[BGColor];
  P=GetBuffer(Y,BC, 0);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BC);
  else
  {
    P[0]=P[1]=P[2]=P[3]=P[4]=P[5]=P[6]=P[7]=P[8]=BC;

    G=(FontBuf&&(Mode&MSX_FIXEDFONT)? FontBuf:ChrGen)+((Y+VScroll)&0x07);
    T=ChrTab+40*(Y>>3);
    FC=XPal[FGColor];
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
void RefreshLine1(register byte Y) // start screen MSX1. this mode works
{
  register pixel *P,FC,BC;
  register byte K,X,*T,*G;
  byte line = Y;
  P=GetBuffer(Y,XPal[BGColor], 1);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
  else
  {
    Y+=VScroll;
    G=(FontBuf&&(Mode&MSX_FIXEDFONT)? FontBuf:ChrGen)+(Y&0x07);
    T=ChrTab+((int)(Y&0xF8)<<2);

    for(X=0;X<32;X++,T++,P+=8)
    {
      K=ColTab[*T>>3];
      FC=XPal[K>>4];
      BC=XPal[K&0x0F];
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
void RefreshLine2(register byte Y)
{
  register pixel *P,FC,BC;
  register byte K,X,*T;
  register int I,J;
  byte line = Y;
  P=GetBuffer(Y,XPal[BGColor], 2);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
  else
  {
    Y+=VScroll;
    T=ChrTab+((int)(Y&0xF8)<<2);
    I=((int)(Y&0xC0)<<5)+(Y&0x07);

    for(X=0;X<32;X++,T++,P+=8)
    {
      J=(int)*T<<3;
      K=ColTab[(I+J)&ColTabM];
      FC=XPal[K>>4];
      BC=XPal[K&0x0F];
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
void RefreshLine3(register byte Y)
{
  register pixel *P;
  register byte X,K,*T,*G;
  byte line = Y;
  P=GetBuffer(Y,XPal[BGColor], 3);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
  else
  {
    Y+=VScroll;
    T=ChrTab+((int)(Y&0xF8)<<2);
    G=ChrGen+((Y&0x1C)>>2);

    for(X=0;X<32;X++,T++,P+=8)
    {
      K=G[(int)*T<<3];
      P[0]=P[1]=P[2]=P[3]=XPal[K>>4];
      P[4]=P[5]=P[6]=P[7]=XPal[K&0x0F];
    }

    if(!SpritesOFF) Sprites(Y,P-256);
  }

}

/** RefreshLine4() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN4, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine4(register byte Y)
{
  register pixel *P,FC,BC;
  register byte K,X,C,*T,*R;
  register int I,J;
  byte line = Y;
  byte ZBuf[320];

  P=GetBuffer(Y,XPal[BGColor], 4);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
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
      FC=XPal[K>>4];
      BC=XPal[K&0x0F];
      K=ChrGen[(I+J)&ChrGenM];

      C=R[0];P[0]=C? XPal[C]:(K&0x80)? FC:BC;
      C=R[1];P[1]=C? XPal[C]:(K&0x40)? FC:BC;
      C=R[2];P[2]=C? XPal[C]:(K&0x20)? FC:BC;
      C=R[3];P[3]=C? XPal[C]:(K&0x10)? FC:BC;
      C=R[4];P[4]=C? XPal[C]:(K&0x08)? FC:BC;
      C=R[5];P[5]=C? XPal[C]:(K&0x04)? FC:BC;
      C=R[6];P[6]=C? XPal[C]:(K&0x02)? FC:BC;
      C=R[7];P[7]=C? XPal[C]:(K&0x01)? FC:BC;
    }
  }

}

/** RefreshLine5() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN5, including       **/
/** sprites in this line.                                   **/
/*************************************************************/

void RefreshLine5(register byte Y) 
{
  register pixel *P;
  register byte I,X,*T,*R;
  byte ZBuf[320];

  P=GetBuffer(Y,XPal[BGColor], 5);
  if(!P) return;
   
  if(!ScreenON)
      ClearLine(P,XPal[BGColor]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<7)&ChrTabM&0x7FFF);

    for(X=0;X<16;X++,R+=16,P+=16,T+=8)
    {
      I=R[0];P[0]=XPal[I? I:T[0]>>4];
      I=R[1];P[1]=XPal[I? I:T[0]&0x0F];
      I=R[2];P[2]=XPal[I? I:T[1]>>4];
      I=R[3];P[3]=XPal[I? I:T[1]&0x0F];
      I=R[4];P[4]=XPal[I? I:T[2]>>4];
      I=R[5];P[5]=XPal[I? I:T[2]&0x0F];
      I=R[6];P[6]=XPal[I? I:T[3]>>4];
      I=R[7];P[7]=XPal[I? I:T[3]&0x0F];
      I=R[8];P[8]=XPal[I? I:T[4]>>4];
      I=R[9];P[9]=XPal[I? I:T[4]&0x0F];
      I=R[10];P[10]=XPal[I? I:T[5]>>4];
      I=R[11];P[11]=XPal[I? I:T[5]&0x0F];
      I=R[12];P[12]=XPal[I? I:T[6]>>4];
      I=R[13];P[13]=XPal[I? I:T[6]&0x0F];
      I=R[14];P[14]=XPal[I? I:T[7]>>4];
      I=R[15];P[15]=XPal[I? I:T[7]&0x0F];
    }
  }

}

/** RefreshLine8() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN8, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine8(register byte Y)
{
    
    static byte SprToScr[16] =
  {
    0x00,0x02,0x10,0x12,0x80,0x82,0x90,0x92,
    0x49,0x4B,0x59,0x5B,0xC9,0xCB,0xD9,0xDB
  };
  register pixel *P;
  register byte C,X,*T,*R;
  byte ZBuf[320];

  P=GetBuffer(Y,BPal[VDP[7]], 8);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BPal[VDP[7]]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF);

    for(X=0;X<32;X++,T+=8,R+=8,P+=8)
    {
      C=R[0];P[0]=BPal[C? SprToScr[C]:T[0]];
      C=R[1];P[1]=BPal[C? SprToScr[C]:T[1]];
      C=R[2];P[2]=BPal[C? SprToScr[C]:T[2]];
      C=R[3];P[3]=BPal[C? SprToScr[C]:T[3]];
      C=R[4];P[4]=BPal[C? SprToScr[C]:T[4]];
      C=R[5];P[5]=BPal[C? SprToScr[C]:T[5]];
      C=R[6];P[6]=BPal[C? SprToScr[C]:T[6]];
      C=R[7];P[7]=BPal[C? SprToScr[C]:T[7]];
    }
  }

}

/** RefreshLine10() ******************************************/
/** Refresh line Y (0..191/211) of SCREEN10/11, including   **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine10(register byte Y)
{
  
    register pixel *P;
  register byte C,X,*T,*R;
  register int J,K;
  byte ZBuf[320];
  byte line = Y;
  P=GetBuffer(Y,BPal[VDP[7]], 10);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BPal[VDP[7]]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF);

    /* Draw first 4 pixels */
    C=R[0];P[0]=C? XPal[C]:BPal[VDP[7]];
    C=R[1];P[1]=C? XPal[C]:BPal[VDP[7]];
    C=R[2];P[2]=C? XPal[C]:BPal[VDP[7]];
    C=R[3];P[3]=C? XPal[C]:BPal[VDP[7]];
    R+=4;P+=4;

    for(X=0;X<63;X++,T+=4,R+=4,P+=4)
    {
      K=(T[0]&0x07)|((T[1]&0x07)<<3);
      if(K&0x20) K-=64;
      J=(T[2]&0x07)|((T[3]&0x07)<<3);
      if(J&0x20) J-=64;

      C=R[0];Y=T[0]>>3;P[0]=C? XPal[C]:Y&1? XPal[Y>>1]:YJKColor(Y,J,K);
      C=R[1];Y=T[1]>>3;P[1]=C? XPal[C]:Y&1? XPal[Y>>1]:YJKColor(Y,J,K);
      C=R[2];Y=T[2]>>3;P[2]=C? XPal[C]:Y&1? XPal[Y>>1]:YJKColor(Y,J,K);
      C=R[3];Y=T[3]>>3;P[3]=C? XPal[C]:Y&1? XPal[Y>>1]:YJKColor(Y,J,K);
    }
  }

}

/** RefreshLine12() ******************************************/
/** Refresh line Y (0..191/211) of SCREEN12, including      **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine12(register byte Y)
{
  
    register pixel *P;
  register byte C,X,*T,*R;
  register int J,K;
  byte ZBuf[320];

  P=GetBuffer(Y,BPal[VDP[7]], 12);
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
    C=R[0];P[0]=C? XPal[C]:BPal[VDP[7]];
    C=R[1];P[1]=C? XPal[C]:BPal[VDP[7]];
    C=R[2];P[2]=C? XPal[C]:BPal[VDP[7]];
    C=R[3];P[3]=C? XPal[C]:BPal[VDP[7]];
    R+=4;P+=4;

    for(X=1;X<64;X++,T+=4,R+=4,P+=4)
    {
      K=(T[0]&0x07)|((T[1]&0x07)<<3);
      if(K&0x20) K-=64;
      J=(T[2]&0x07)|((T[3]&0x07)<<3);
      if(J&0x20) J-=64;

      C=R[0];P[0]=C? XPal[C]:YJKColor(T[0]>>3,J,K);
      C=R[1];P[1]=C? XPal[C]:YJKColor(T[1]>>3,J,K);
      C=R[2];P[2]=C? XPal[C]:YJKColor(T[2]>>3,J,K);
      C=R[3];P[3]=C? XPal[C]:YJKColor(T[3]>>3,J,K);
    }
  }

}


void RefreshLine6(byte Y) {
register pixel *P;
  register byte X,*T,*R,C;
  byte ZBuf[304];

  P=GetBuffer(Y,XPal[BGColor&0x03], 6);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor&0x03]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<7)&ChrTabM&0x7FFF);

    for(X=0;X<32;X++)
    {
      C=R[0];P[0]=XPal[C? C:T[0]>>6];
      C=R[1];P[1]=XPal[C? C:(T[0]>>2)&0x03];
      C=R[2];P[2]=XPal[C? C:T[1]>>6];
      C=R[3];P[3]=XPal[C? C:(T[1]>>2)&0x03];
      C=R[4];P[4]=XPal[C? C:T[2]>>6];
      C=R[5];P[5]=XPal[C? C:(T[2]>>2)&0x03];
      C=R[6];P[6]=XPal[C? C:T[3]>>6];
      C=R[7];P[7]=XPal[C? C:(T[3]>>2)&0x03];
      R+=8;P+=8;T+=4;
    }
  }

  

}
/** RefreshLine7() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN7, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine7(register byte Y)
{
  register pixel *P;
  register byte C,X,*T,*R;
  byte ZBuf[304];

  P=GetBuffer(Y,XPal[BGColor], 7);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF);

    for(X=0;X<32;X++)
    {
      C=R[0];P[0]=XPal[C? C:T[0]>>4];
      C=R[1];P[1]=XPal[C? C:T[1]>>4];
      C=R[2];P[2]=XPal[C? C:T[2]>>4];
      C=R[3];P[3]=XPal[C? C:T[3]>>4];
      C=R[4];P[4]=XPal[C? C:T[4]>>4];
      C=R[5];P[5]=XPal[C? C:T[5]>>4];
      C=R[6];P[6]=XPal[C? C:T[6]>>4];
      C=R[7];P[7]=XPal[C? C:T[7]>>4];
      R+=8;P+=8;T+=8;
    }
  }

}

//////////////////// Overlay Menu functions /////////////////////////


/** ShowVideo() **********************************************/
/** Show "active" image at the actual screen or window (overlay).     **/
/*************************************************************/
int ShowVideo(void) {
   
    ili9341_write_frame_msx(0,0,WIDTH_OVERLAY,HEIGHT_OVERLAY, overlay.Data, XPal[BGColor]);
    return 0;
}

