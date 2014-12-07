#include <3ds.h>
#include <string.h>
#include <stdlib.h>

#define XRANGE 3.75
#define YRANGE 2.25
#define XCENTR -0.5
#define YCENTR 0.0
#define SCALING 0.009375
#define MAX_ITER 24
#define ITER_DIFF 8
#define APPROXIMATELY_INFINITY 1.0e5
#define STICK_THRESHOLD 15

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
    float xcentr;
    float ycentr;
    int zoomlevel;
    int max_iter;
} Mandelbrot_params;

// eww global variables
// (but I'm not competent enough to make this work as a struct var or pass pointers)
int iteration[ARRAY_LENGTH];

void calc_mandelbrot(Mandelbrot_params m)
{
    //implementation of Wikipedia's escape time pseudocode
    int i,j,k;
    u32 idx;
    float xlower = m.xcentr-XRANGE/(2<<m.zoomlevel);
    float ylower = m.ycentr-YRANGE/(2<<m.zoomlevel);
    float x0;
    float y0;
    float x,y;
    float xtemp;
    float scaling = SCALING/(int)(1<<m.zoomlevel);
    
    for (i=0;i<WIDTH_TOP;i++)
    {
        x0 = xlower+i*scaling;
        for (j=0;j<HEIGHT;j++)
        {
            y0 = ylower+j*scaling;
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
                    iteration[idx] = k;
                    break;
                }
            }
            if(k==m.max_iter)
            {
                iteration[idx] = k;
            }
        }
    }
}

void draw_mandelbrot(Mandelbrot_params m)//, bool render_top, bool render_bottom)
{
    int i,j;
    u32 idx;
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
        for (j=0;j<HEIGHT;j++)
        {
            idx = (u32)i*HEIGHT+j;    
            iter_frac = (float)iteration[idx]/m.max_iter;
            sep_iter = (int)(sep-sep*iter_frac);
            if ((i>=WIDTH_DIFF)&&(i<WIDTH_TOP-WIDTH_DIFF)) //(render_bottom && ((i>=WIDTH_DIFF)&&(i<WIDTH_TOP-WIDTH_DIFF)))
            {
                fbAdr_B[idx*3-3*WIDTH_DIFF*HEIGHT] = (u8)(255*iter_frac);
            }
            /*if (render_top)
            {*/
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
            //}
        }
    }
}

/*
void draw_mandelbrot_proper(Mandelbrot_params m, bool render_top, bool render_bottom)
{
    if (render_bottom)
    {
        calc_mandelbrot(m);
    }
    gfxFlushBuffers();
    draw_mandelbrot(m, render_top,render_bottom);
    gfxSwapBuffers();
    gspWaitForVBlank();
    draw_mandelbrot(m, render_top,render_bottom);
    
}*/

void Mandelbrot_init(Mandelbrot_params* m)
{
    m->xcentr = XCENTR;
    m->ycentr = YCENTR;
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

  //float monitor_3d_slider = CONFIG_3D_SLIDERSTATE;
  float temp_scaling; //,temp_x,temp_y;
  circlePosition c3po;
  touchHandler t;
  Mandelbrot_params m;
  Mandelbrot_init(&m);
  calc_mandelbrot(m);
  //draw_mandelbrot_proper(m,true,true);
  
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
    
    temp_scaling = SCALING/(int)(1<<m.zoomlevel);
    hidCircleRead(&c3po);

    bool xmoved = (abs(c3po.dx)>STICK_THRESHOLD);
    bool ymoved = (abs(c3po.dy)>STICK_THRESHOLD);
    if ((xmoved)||(ymoved))
    {
        if (xmoved)
        {
            m.xcentr += c3po.dx*temp_scaling/4;
        }

        if (ymoved)
        {
            m.ycentr += c3po.dy*temp_scaling/4;
        }
        calc_mandelbrot(m);
        //draw_mandelbrot_proper(m,true,true);        
    }

    if (handle_touch(&t))
    {
        m.zoomlevel++;
        m.xcentr += (-WIDTH_BOTTOM/2+t.touch2.px)*temp_scaling;
        m.ycentr += (HEIGHT/2-t.touch2.py)*temp_scaling;
        calc_mandelbrot(m);
        //draw_mandelbrot_proper(m,true,true);        
    }
    
    if (hidKeysDown() & KEY_B){
        Mandelbrot_init(&m);
        calc_mandelbrot(m);
      //draw_mandelbrot_proper(m,true,true);
    }
    
    if (hidKeysDown() & KEY_A){
        m.max_iter+=ITER_DIFF;
        calc_mandelbrot(m);
      //draw_mandelbrot_proper(m,true,true);
    }
    
    /*if (CONFIG_3D_SLIDERSTATE!=monitor_3d_slider){
        monitor_3d_slider = CONFIG_3D_SLIDERSTATE;
        draw_mandelbrot_proper(m,true,false);
    }*/
    draw_mandelbrot(m);

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
