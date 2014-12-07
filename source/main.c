#include <3ds.h>
#include <string.h>
#include <stdlib.h>

#define XLOWER -2.375
#define XUPPER 1.375
#define YLOWER -1.125
#define YUPPER 1.125
#define SCALING 0.009375
#define MAX_ITER 24
#define ITER_DIFF 5
#define APPROXIMATELY_INFINITY 1.0e5

#define WIDTH_TOP 400
#define WIDTH_BOTTOM 320
#define WIDTH_DIFF (WIDTH_TOP-WIDTH_BOTTOM)/2
#define HEIGHT 240
#define ARRAY_LENGTH (u32)WIDTH_TOP*HEIGHT

#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080) //thanks shinyquagsire23
#define MAX_STEREO_SEPARATION 42
#define STEREO_SEPARATION (MAX_STEREO_SEPARATION * CONFIG_3D_SLIDERSTATE)

typedef struct touchHandler{
    int rudimentaryTimer;
    touchPosition touch1;
    touchPosition touch2;
} touchHandler;

typedef struct Mandelbrot_params{
    float xlower;
    float xupper;
    float ylower;
    float yupper;
    int zoomlevel;
    int max_iter;
} Mandelbrot_params;

void draw_mandelbrot(Mandelbrot_params m, bool render_top, bool render_bottom)
{
    //implementation of Wikipedia's escape time pseudocode
    int i,j,k;
    u32 idx;
    float x0;
    float y0;
    float x,y;
    float xtemp;
    float scaling = SCALING/(int)(1<<m.zoomlevel);
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
        x0 = m.xlower+i*scaling;
        for (j=0;j<HEIGHT;j++)
        {
            y0 = m.ylower+j*scaling;
            idx = (u32)i*HEIGHT+j;
            x=0.0;
            y=0.0;
            for (k=0;k<m.max_iter;k++)
            {
                xtemp = x*x-y*y+x0;
                y = 2*x*y+y0;
                x = xtemp;
                if (x*x+y*y>APPROXIMATELY_INFINITY)
                {
                    iter_frac = (float)k/m.max_iter;
                    sep_iter = (int)(sep-sep*iter_frac);
                    if (render_bottom && ((i>=WIDTH_DIFF)&&(i<WIDTH_TOP-WIDTH_DIFF)))
                    {
                        fbAdr_B[idx*3-3*WIDTH_DIFF*HEIGHT] = (u8)(255*iter_frac);
                    }
                    if (render_top)
                    {
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
                    }
                    break;
                }
            }
            if(k==m.max_iter)
            {
                if (render_top)
                {
                    fbAdr_L[idx*3] = 255;
                    fbAdr_R[idx*3] = 255;
                }
                if (render_bottom && (i>=WIDTH_DIFF)&&(i<WIDTH_TOP-WIDTH_DIFF))
                {
                    fbAdr_B[idx*3-3*WIDTH_DIFF*HEIGHT] = 255;
                }
            }
        }
    }
}

void draw_mandelbrot_proper(Mandelbrot_params m, bool render_top, bool render_bottom)
{
    gfxFlushBuffers();
    draw_mandelbrot(m, render_top,render_bottom);
    gfxSwapBuffers();
    gspWaitForVBlank();
    draw_mandelbrot(m, render_top,render_bottom);
    
}

void Mandelbrot_init(Mandelbrot_params* m)
{
    m->xlower = XLOWER;
    m->ylower = YLOWER;
    m->xupper = XUPPER;
    m->yupper = YUPPER;
    m->zoomlevel = 0;
    m->max_iter = MAX_ITER;
}

bool handle_touch(touchHandler* t) // thanks smea and hbmenu people
{
    touchPosition touch;
    hidTouchRead(&touch);
    if(hidKeysDown()&KEY_TOUCH)
	{
		t->rudimentaryTimer=0;
		t->touch1=touch;
	}
    else if((hidKeysUp() & KEY_TOUCH) && t->rudimentaryTimer<30 && abs(t->touch1.px-t->touch2.px)+abs(t->touch1.py-t->touch2.py)<12){
		return true;
	}
    else if(hidKeysHeld() & KEY_TOUCH){
		t->rudimentaryTimer++;
	}
    
    t->touch2 = touch;	
    return false;
}

int main() //using xem's template
{
  // Initializations
  srvInit();        // services
  aptInit();        // applets
  hidInit(NULL);    // input
  gfxInit();        // graphics
  gfxSet3D(true);   // stereoscopy (true: enabled / false: disabled)
  //u32 kDown;        // keys down
  //u32 kHeld;        // keys pressed
  //u32 kUp;          // keys up

  float monitor_3d_slider = CONFIG_3D_SLIDERSTATE;
  float temp_scaling,temp_x,temp_y;
  touchHandler t;
  Mandelbrot_params m;
  Mandelbrot_init(&m);
  draw_mandelbrot_proper(m,true,true);
  
  // Main loop
  while (aptMainLoop())
  {

    // Wait for next frame
    gspWaitForVBlank();

    // Read which buttons are currently pressed or not
    hidScanInput();
    //kDown = hidKeysDown();
    //kHeld = hidKeysHeld();
    //kUp = hidKeysUp();

    // If START button is pressed, break loop and quit
    if (hidKeysDown() & KEY_START){
      break;
    }
    
    if (handle_touch(&t))
    {
        temp_scaling = SCALING/(int)(1<<m.zoomlevel);
        m.zoomlevel++;
        temp_x = (t.touch2.px+WIDTH_BOTTOM/2-WIDTH_DIFF)*temp_scaling+m.xlower;
        temp_y = (-t.touch2.py+HEIGHT+HEIGHT/4)*temp_scaling+m.ylower;
        m.xlower = temp_x-temp_scaling*WIDTH_TOP/2;
        m.xupper = temp_x+temp_scaling*WIDTH_TOP/2;
        m.ylower = temp_y-temp_scaling*HEIGHT/2;
        m.yupper = temp_y+temp_scaling*HEIGHT/2;
        m.max_iter+=ITER_DIFF;
        draw_mandelbrot_proper(m,true,true);        
    }
    
    if (hidKeysDown() & KEY_B){
      Mandelbrot_init(&m);
      draw_mandelbrot_proper(m,true,true);
    }
    
    if (CONFIG_3D_SLIDERSTATE!=monitor_3d_slider){
        monitor_3d_slider = CONFIG_3D_SLIDERSTATE;
        draw_mandelbrot_proper(m,true,false);
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
