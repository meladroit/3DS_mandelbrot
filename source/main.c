#include <3ds.h>
#include <string.h>
// #include <math.h>

#define XLOWER -2.375
#define XUPPER 1.375
#define YLOWER -1.125
#define YUPPER 1.125
#define SCALING 0.009375
#define MAX_ITER 24
#define APPROXIMATELY_INFINITY 1.0e5

#define WIDTH_TOP 400
#define WIDTH_BOTTOM 320
#define WIDTH_DIFF (WIDTH_TOP-WIDTH_BOTTOM)/2
#define HEIGHT 240
#define ARRAY_LENGTH (u32)WIDTH_TOP*HEIGHT

#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080) //thanks shinyquagsire23
#define MAX_STEREO_SEPARATION 42
#define STEREO_SEPARATION (MAX_STEREO_SEPARATION * CONFIG_3D_SLIDERSTATE)

void draw_mandelbrot(bool render_bottom)
{
    //implementation of Wikipedia's escape time pseudocode
    int i,j,k;
    u32 idx;
    float x0;
    float y0;
    float x,y;
    float xtemp;
    int sep = (int)STEREO_SEPARATION/2;
    float iter_frac;
    int sep_iter;
    u8* fbAdr_L = gfxGetFramebuffer(GFX_TOP,GFX_LEFT,NULL,NULL);
    u8* fbAdr_R = gfxGetFramebuffer(GFX_TOP,GFX_RIGHT,NULL,NULL);
    u8* fbAdr_B = gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,NULL,NULL);
    memset(fbAdr_L,0,WIDTH_TOP*HEIGHT*3);
    memset(fbAdr_R,0,WIDTH_TOP*HEIGHT*3);

    for (i=0;i<WIDTH_TOP;i++)
    {
        x0 = XLOWER+i*SCALING;
        for (j=0;j<HEIGHT;j++)
        {
            y0 = YLOWER+j*SCALING;
            idx = (u32)i*HEIGHT+j;
            x=0.0;
            y=0.0;
            for (k=0;k<MAX_ITER;k++)
            {
                xtemp = x*x-y*y+x0;
                y = 2*x*y+y0;
                x = xtemp;
                if (x*x+y*y>APPROXIMATELY_INFINITY)
                {
                    iter_frac = (float)k/MAX_ITER;
                    sep_iter = (int)(sep-sep*iter_frac);
                    if (render_bottom && ((i>=WIDTH_DIFF)&&(i<WIDTH_TOP-WIDTH_DIFF)))
                    {
                        fbAdr_B[idx*3-3*WIDTH_DIFF*HEIGHT] = (u8)(255*iter_frac);
                    }
                    if (sep_iter<=i)
                    {
                        idx-=sep_iter*HEIGHT;
                        if ((u8)(255*iter_frac)>fbAdr_L[idx*3])
                        {
                            fbAdr_L[idx*3] = (u8)(255*iter_frac);
                        }
                    }
                    if (CONFIG_3D_SLIDERSTATE>0)
                    {
                        if (sep_iter<WIDTH_TOP-i)
                        {
                            idx+=2*sep_iter*HEIGHT;
                            if ((u8)(255*iter_frac)>fbAdr_R[idx*3])
                            {
                                fbAdr_R[idx*3] = (u8)(255*iter_frac);
                            }
                        }
                    }
                    break;
                }
            }
            if(k==MAX_ITER)
            {
                fbAdr_L[idx*3] = 255;
                fbAdr_R[idx*3] = 255;
                if (render_bottom && (i>=WIDTH_DIFF)&&(i<WIDTH_TOP-WIDTH_DIFF))
                {
                    fbAdr_B[idx*3-3*WIDTH_DIFF*HEIGHT] = 255;
                }
            }
        }
    }
}

void draw_mandelbrot_proper(bool render_bottom)
{
    draw_mandelbrot(render_bottom);
    //gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
    draw_mandelbrot(render_bottom);
    
}

int main() //using xem's template
{
  // Initializations
  srvInit();        // services
  aptInit();        // applets
  hidInit(NULL);    // input
  gfxInit();        // graphics
  gfxSet3D(true);   // stereoscopy (true: enabled / false: disabled)
  u32 kDown;        // keys down
  //u32 kHeld;        // keys pressed
  //u32 kUp;          // keys up

  float monitor_3d_slider = CONFIG_3D_SLIDERSTATE;
  draw_mandelbrot_proper(true);
  
  // Main loop
  while (aptMainLoop())
  {

    // Wait for next frame
    gspWaitForVBlank();

    // Read which buttons are currently pressed or not
    hidScanInput();
    kDown = hidKeysDown();
    //kHeld = hidKeysHeld();
    //kUp = hidKeysUp();

    // If START button is pressed, break loop and quit
    if (kDown & KEY_START){
      break;
    }
    
    if (CONFIG_3D_SLIDERSTATE!=monitor_3d_slider){
        monitor_3d_slider = CONFIG_3D_SLIDERSTATE;
        draw_mandelbrot_proper(false);
    }
    //draw_mandelbrot();

    // Flush and swap framebuffers
    gfxFlushBuffers();
    gfxSwapBuffers();
  }

  // Exit
  gfxExit();
  hidExit();
  aptExit();
  srvExit();

  // Return to hbmenu
  return 0;
}
