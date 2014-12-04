#include <3ds.h>
// #include <math.h>

#define XLOWER -2.375
#define XUPPER 1.375
#define YLOWER -1.125
#define YUPPER 1.125
#define SCALING 0.006875
#define MAX_ITER 42
#define APPROXIMATELY_INFINITY 1.0e5

#define WIDTH_TOP 400
#define WIDTH_BOTTOM 320
#define HEIGHT 240

#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080) //thanks shinyquagsire23
#define MAX_STEREO_SEPARATION 10
#define STEREO_SEPARATION (MAX_STEREO_SEPARATION * CONFIG_3D_SLIDERSTATE)

void draw_mandelbrot()
{
    //implementation of Wikipedia's escape time pseudocode
    int i,j,k;
    u32 idx;
    bool escaped[WIDTH_TOP*HEIGHT];
    float x1[WIDTH_TOP*HEIGHT];
    float y1[WIDTH_TOP*HEIGHT];
    float x0;
    float y0;
    float x,y;
    float xtemp;
    float px_iteration;
    int stereo_adj;
    u16 fbWidth, fbHeight;

    u8* fbAdr_L = gfxGetFramebuffer(GFX_TOP,GFX_LEFT,&fbWidth,&fbHeight);
    //u8* fbAdr_R = gfxGetFramebuffer(GFX_TOP,GFX_RIGHT,&fbWidth,&fbHeight);

    for (k=0;k<MAX_ITER;k++)
    {
        for (i=0;i<WIDTH_TOP;i++)
        {
            x0 = XLOWER+i*SCALING;
            for (j=0;j<HEIGHT;j++)
            {
                y0 = YLOWER+j*SCALING;
                idx = i*HEIGHT+j;
                if(0==k)
                {
                    escaped[idx]=false;
                    x1[idx]=0.0;
                    y1[idx]=0.0;
                }
                if(escaped[idx])
                {
                    continue;
                }
                x = x1[idx];
                y = y1[idx];
                xtemp = x*x-y*y+x0;
                y = 2*x*y+y0;
                x = xtemp;
                x1[idx] = x;
                y1[idx] = y;
                if (x*x+y*y>APPROXIMATELY_INFINITY)
                {
                    escaped[idx]=true;
                    px_iteration = 1.0;//(float)(k)/MAX_ITER;
                    stereo_adj = 0;//(int)(px_iteration*STEREO_SEPARATION);

                    if (i>=stereo_adj)
                    {
                        idx = 3*(j+HEIGHT*i); //-stereo_adj);
                        // u8* fbdL = &fbAdr_L[idx*3];
                        fbAdr_L[idx] = 127;
                        fbAdr_L[idx+1] = 0;
                        fbAdr_L[idx+2] = 127;
                        /* fbdL[1] = 0;
                        fbdL[2] = 127;//(u8)(255*px_iteration); */
                    }
                    /*
                    if (CONFIG_3D_SLIDERSTATE>0)
                    {
                        i+= stereo_adj;
                        if (i<WIDTH_TOP)
                        {
                            idx = j+HEIGHT*i;
                            fbdR = &fbAdr_R[idx*3];
                            fbdR[0] = (u8)(255*px_iteration);
                            fbdR[1] = (u8)0;
                            fbdR[2] = (u8)(255*px_iteration);
                        }
                    }
                    */
                    continue;
                }
            }
        }
    }
}

void fill_magenta()
{
    u16 fbWidth, fbHeight;
    int i,j,k,idx;
    u8* fbAdr_L = gfxGetFramebuffer(GFX_TOP,GFX_LEFT,&fbWidth,&fbHeight);
    u8* fbdL;
    u8 fbdR[1]={255};
    for (k=0;k<1;k++)
    {
        for (i=0;i<WIDTH_TOP;i++)
        {
            for (j=0;j<HEIGHT;j++)
            {
                idx = i*HEIGHT+j;
                fbdL = &fbAdr_L[idx*3];
                fbdL[0]=fbdR[k];
                fbdL[1]=0;
                fbdL[2]=fbdR[k];
            }
        }
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
  bool key_pressed = false;

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

    if(!key_pressed)
    {
        fill_magenta();
    }
    if (kDown & KEY_A){
        key_pressed = true;
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
