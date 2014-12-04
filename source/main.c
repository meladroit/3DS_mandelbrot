#include <3ds.h>
#include <string.h>
// #include <math.h>

#define XLOWER -2.375
#define XUPPER 1.375
#define YLOWER -1.125
#define YUPPER 1.125
#define SCALING 0.009375
#define MAX_ITER 42
#define APPROXIMATELY_INFINITY 1.0e5

#define WIDTH_TOP 400
#define WIDTH_BOTTOM 320
#define WIDTH_DIFF (WIDTH_TOP-WIDTH_BOTTOM)/2
#define HEIGHT 240

#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080) //thanks shinyquagsire23
#define MAX_STEREO_SEPARATION 42
#define STEREO_SEPARATION (MAX_STEREO_SEPARATION * CONFIG_3D_SLIDERSTATE)

void calc_mandelbrot()
{
    //implementation of Wikipedia's escape time pseudocode
    int i,j,k;
    int sep = (int)STEREO_SEPARATION/2;
    u32 idx;
    float x0;
    float y0;
    float x,y;
    float xtemp;
    float iter_frac;

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
            idx = i*HEIGHT+j;
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
                    if ((i>=WIDTH_DIFF)&&(i<WIDTH_TOP-WIDTH_DIFF))
                    {
                        fbAdr_B[idx*3-3*WIDTH_DIFF*HEIGHT] = (u8)(255*iter_frac);
                    }
                    if ((1-iter_frac)*sep<=i)
                    {
                        idx-=(int)(HEIGHT*(1-iter_frac)*sep);
                        if ((u8)(255*iter_frac)>fbAdr_L[idx*3])
                        {
                            fbAdr_L[idx*3] = (u8)(255*iter_frac);
                        }
                    }
                    if (CONFIG_3D_SLIDERSTATE>0)
                    {
                        if ((1-iter_frac)*sep<WIDTH_TOP-i)
                        {
                            idx+=2*(int)(HEIGHT*(1-iter_frac)*sep);
                            if ((u8)(255*iter_frac)>fbAdr_R[idx*3])
                            {
                                fbAdr_R[idx*3] = (u8)(255*iter_frac);
                            }
                        }
                    }
                    break;
                }
            }
            if (k==MAX_ITER)
            {
                fbAdr_L[idx*3] = 255;
                fbAdr_R[idx*3] = 255;
                if ((i>=WIDTH_DIFF)&&(i<WIDTH_TOP-WIDTH_DIFF))
                {
                    fbAdr_B[idx*3-3*WIDTH_DIFF*HEIGHT] = 255;
                }
            }
        }
    }
}

void draw_mandelbrot()
{
    int i;
    for (i=0;i<2;i++)
    {
        gspWaitForVBlank();
        gfxFlushBuffers();
        gfxSwapBuffers();
        calc_mandelbrot();
    }
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
  u32 kHeld;        // keys pressed
  u32 kUp;          // keys up
  float monitor_3d_slider = CONFIG_3D_SLIDERSTATE;
  draw_mandelbrot();

  // Main loop
  while (aptMainLoop())
  {

    // Wait for next frame
    gspWaitForVBlank();

    // Read which buttons are currently pressed or not
    hidScanInput();
    kDown = hidKeysDown();
    kHeld = hidKeysHeld();
    kUp = hidKeysUp();

    // If START button is pressed, break loop and quit
    if (kDown & KEY_START){
      break;
    }

    if (CONFIG_3D_SLIDERSTATE!=monitor_3d_slider){
        monitor_3d_slider = CONFIG_3D_SLIDERSTATE;
        draw_mandelbrot();
    }

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
