
/* Acidworm 0.2 - By Aaron Tiensivu - tiensivu@pilot.msu.edu */
/* Gross hack of Eric P. Scott's 'worm' program              */
/* Inspired by Acidwarp and other eyecandy programs          */

/* If you like it, tell me. If you hate it, tell me.         */
/* If it gives you epileptic seisures, enjoy!                */

#include <stdio.h> 
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <vga.h>
#include <vgagl.h>
#include <math.h>

static unsigned char pal[256 * 3];
 
static struct options {
  int nopts;
  int opts[3];
  }
    normal[8] = {
      { 3, { 7, 0, 1 } },
      { 3, { 0, 1, 2 } },
      { 3, { 1, 2, 3 } },
      { 3, { 2, 3, 4 } },
      { 3, { 3, 4, 5 } },
      { 3, { 4, 5, 6 } },
      { 3, { 5, 6, 7 } },
      { 3, { 6, 7, 0 } }},

    upper[8] = {
      { 1, { 1, 0, 0 } },
      { 2, { 1, 2, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 2, { 4, 5, 0 } },
      { 1, { 5, 0, 0 } },
      { 2, { 1, 5, 0 } }},

    left[8] = {
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 2, { 2, 3, 0 } },
      { 1, { 3, 0, 0 } },
      { 2, { 3, 7, 0 } },
      { 1, { 7, 0, 0 } },
      { 2, { 7, 0, 0 } }},

    right[8] = {
      { 1, { 7, 0, 0 } },
      { 2, { 3, 7, 0 } },
      { 1, { 3, 0, 0 } },
      { 2, { 3, 4, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 2, { 6, 7, 0 } }},

    lower[8] = {
      { 0, { 0, 0, 0 } },
      { 2, { 0, 1, 0 } },
      { 1, { 1, 0, 0 } },
      { 2, { 1, 5, 0 } },
      { 1, { 5, 0, 0 } },
      { 2, { 5, 6, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } }},

    upleft[8] = {
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 1, { 3, 0, 0 } },
      { 2, { 1, 3, 0 } },
      { 1, { 1, 0, 0 } }},

    upright[8] = {
      { 2, { 3, 5, 0 } },
      { 1, { 3, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 1, { 5, 0, 0 } }},

    lowleft[8] = {
      { 3, { 7, 0, 1 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 1, { 1, 0, 0 } },
      { 2, { 1, 7, 0 } },
      { 1, { 7, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } }},

    lowright[8] = {
      { 0, { 0, 0, 0 } },
      { 1, { 7, 0, 0 } },
      { 2, { 5, 7, 0 } },
      { 1, { 5, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } }};

const char *LSD_module = "acidworm";

static int initialized = 0;

static short 
  xinc[] = 
    {1,  1,  1,  0, -1, -1, -1,  0},
  yinc[] = 
    {-1,  0,  1,  1,  1,  0, -1, -1};

static struct
worm 
{
  int orientation, head;
  short *xpos, *ypos;
} *worm;

void onsig(int ignore);
void setcustompalette(int color_cycle);
void shiftpalette(void);
void bail(const char *string); 
void printStrArray(char *strArray[]);

int main(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	struct worm *w;
	struct options *op;
	short *ip;

	register int 
	x = 0,
	y = 0;

	unsigned int tmp1,tmp2;

	int 
	framecount = 0,
	max_x = 0,
	max_y = 0,
	max_color = 0,
	last = 0,
	bottom = 0,
	ch = 0,
	length = 0,
	number = 0,
	color_cycle = 0,
        sync = 0,
	vga_mode = 0,
	exitprogram = 0,
	h = 0,
	n = 0, 
        position = 0,
        position2= 0;
 
	short **ref;
        char *mpp;

        const char *header = "\nAcidWorm version 0.2 - By Aaron Tiensivu - [12/05/96] - SpunkMunky International\n";
	char *usage[] = { 
          "usage:   acidworm [-l #] [-n #] [-g #] [-c #] [-p #] [-s #]\n",
          "default: acidworm -l 100 -n 256 -g 10 -c 1 -p 1 -s 0\n\n", 
          " -l: Length of worms -> 2 to 1024 segments\n",
          " -n: Number of worms\n",
          " -c: Color cycling    -> 0:None 1:RGB 2:DiMonoCromny 3:WavyRave 4:Robotron\n",
          " -p: Worm placement   -> 0:Random 1:Center 2:UL 3:UR 4:LL 5:LR 6:Combo of 0-5\n",
          " -s: Wait for retrace -> 0:No 1:Yes\n", 
          " -g: Graphics mode (as of SVGALib 1.2.10)\n",
          "      5: 320x200x256  ",
          "10: 640x480x256\n",
          "     11: 800x600x256  ",
          "12: 1024x768x256\n",
          "     13: 1280x1024x256\n\n",
          " If GSVGAMODE is set, the graphic mode specified will be used.\n",
          " (e.g. under bash, 'export GSVGAMODE=G1280x1024x256' \n\n","" };

	/* Set-up signal handling */
        (void)signal(SIGHUP, onsig);
        (void)signal(SIGINT, onsig);
        (void)signal(SIGQUIT, onsig);
        (void)signal(SIGSTOP, onsig);
        (void)signal(SIGTSTP, onsig);
        (void)signal(SIGTERM, onsig);
        (void)signal(SIGSEGV, onsig);

        /* Default worm length, number, and color cycling mode */
        length = 100;
        number = 256;
        color_cycle = 1; /* Normal color cycling     */
        position = 1;    /* Centered worm placement  */
        sync = 0;        /* Don't wait - full blast  */

        /* Parse command line options */
        while ((ch = getopt(argc, argv, "fc:g:l:n:t:p:?:h:s:")) != EOF)
        switch(ch) {
        case 'c':
          if ((color_cycle = atoi(optarg)) > 4)
            bail("invalid color cycle value [0 = off / 1,2,3,4 = on]");

          break;
        case 'g':
          /* This doesn't check for an upper bounds,
           * new modes could be added to SVGALib in the future */

          if ((vga_mode = atoi(optarg)) < 0)
            bail("invalid graphics mode specified");

          break;
        case 'l':
          if ((length = atoi(optarg)) < 2 || length > 1024)
            bail("invalid worm length [2 thru 1024 only]");

          break;
        case 'n':
          if ((number = atoi(optarg)) < 1)
            bail("invalid number of worms");

          break;
        case 'p':
          if ( (position = atoi(optarg)) < 0 || position > 6)
            bail("invalid position given");

          break;
        case 's':
          if ( (sync = atoi(optarg)) < 0 || sync > 1)
            bail("invalid sync specification");
          
        break;
        case '?':
        case 'h':
        default: /* For -? and -h, etc */
          printf(header);
          printStrArray(usage);
          bail("Have a nice day. 8-]");
        }

	/* This ugly and super-anal, but it works    */
        /* I want to know where it fails             */        

        if (vga_init() == 0)
        {
          initialized = 1;

          /* Check for a default SVGALib graphics mode */
          /* Not defined? Default to 640x480x256       */

          if (vga_mode == 0)
          {
            vga_mode = vga_getdefaultmode();
            if (vga_mode == -1)
              vga_mode = (G640x480x256);
          }

          if ( vga_hasmode(vga_mode) )
          {
            if ( vga_setmode(vga_mode) == 0 )
            {
              if ( gl_setcontextvga(vga_mode) == 0 )
              {
                max_x     = vga_getxdim();
                max_y     = vga_getydim();
                max_color = vga_getcolors();

                if (max_color < 256)
                  color_cycle = 0; 

                setcustompalette(color_cycle);

                last      = (max_x - 1);
                bottom    = (max_y - 1);

              }
              else
              {
                bail("SVGALib error - gl_setcontextvga() failed!");
              }
            }
            else
            {
              bail("SVGALib error - vga_setmode() failed!");
            }
          }
          else
          {
            bail("SVGALib error - vga_hasmode() failed!");
          }
        }
        else
        {
          bail("SVGALib error - vga_init() failed!");
        }

	/* Allocate memory */
        if ( (!(worm = (struct worm *)malloc((u_int)number *
	   sizeof(struct worm))) || !(mpp = malloc((u_int)1024))) ||
           (!(ip = (short *)malloc((u_int)(max_x * max_y * sizeof(short))))) ||
           (!(ref = (short **)malloc((u_int)(max_y * sizeof(short *))))) )
		bail("Not enough memory for worms (part 1)");

	for (n = 0; n < max_y; ++n)
        {
	  ref[n] = ip;
	  ip += max_x;
	}
	for (ip = ref[0], n = max_y * max_x; --n >= 0;)
	  *ip++ = 0;
	for (n = number, w = &worm[0]; --n >= 0; w++) 
        {
	  w->orientation = w->head = 0;
	  if (!(ip = (short *)malloc((u_int)(length * sizeof(short)))))
	    bail("Not enough memory for worms (part 2)");
	  w->xpos = ip;
	  for (x = length; --x >= 0;)
	    *ip++ = -1;
	  if (!(ip = (short *)malloc((u_int)(length * sizeof(short)))))
	    bail("Not enough memory for worms (part 2)");
	  w->ypos = ip;
	  for (y = length; --y >= 0;)
	    *ip++ = -1;
	}

	while (exitprogram == 0) 
        {
	  framecount++;
          if ( (framecount % 100) == 0 )
            exitprogram = vga_getkey();

          if (color_cycle) shiftpalette(); 

          if (sync) vga_waitretrace();

	  for (n = 0, w = &worm[0]; n < number; n++, w++) 
          {

            if ((x = w->xpos[h = w->head]) < 0)
            {
              if (position < 6)
                position2 = position;
              else 
                position2 = (random() % 6);

              switch (position2) 
              {
                case 0:
                  tmp1 = ( (random() % last  ) + 1);
                  tmp2 = ( (random() % bottom) + 1);
                  break;
                case 1:
                  tmp1 = (last / 2);
                  tmp2 = (bottom / 2);
                  break;
                case 2:
                  tmp1 = tmp2 = 1;
                  break;
                case 3: 
                  tmp1 = last;
                  tmp2 = 0;
                  break; 
                case 4:
                  tmp1 = 0; 
                  tmp2 = bottom;
                  break;
                case 5:
                  tmp1 = last - 1;
                  tmp2 = bottom - 1;
                  break;
              }      
              gl_setpixel(x = w->xpos[h] = tmp1,
                          y = w->ypos[h] = tmp2,
                          n);
              ref[y][x]++;
            }
            else y = w->ypos[h];
            if (++h == length)  h = 0;
	    if (w->xpos[w->head = h] >= 0)
            {
              int x1, y1;

              x1 = w->xpos[h];
              y1 = w->ypos[h]; 
              if (--ref[y1][x1] == 0) gl_setpixel(x1, y1, 0);
            }
            op = &(!x ? (!y ? upleft : (y == bottom ? lowleft : left)) : (x == last ? (!y ? upright : (y == bottom ? lowright : right)) : (!y ? upper : (y == bottom ? lower : normal))))[w->orientation];
            switch (op->nopts) 
            {
              case 1:
                w->orientation = op->opts[0];
                break;
              default:
                w->orientation = op->opts[(int)random() % op->nopts];
            }
            gl_setpixel(x += xinc[w->orientation],
                        y += yinc[w->orientation],
                        n                             );
            ref[w->ypos[h] = y][w->xpos[h] = x]++;
          }
        }
       
      /* Restore text mode and exit sanely */
      vga_setmode(TEXT);
      printf(header); 
      printf("Execute 'acidworm -h' for detailed command line options\n");  
      exit(0);
    }

void onsig(int ignore)
{
  vga_setmode(TEXT);
  exit(0);
}

void bail(const char *string)
{
  if (initialized)
    vga_setmode(TEXT);
  printf("%s: %s \n", LSD_module , string);
  exit(0);  
}

/* This routine screams 'rewrite me' */
/* Feel free to do so, I'm working on a new one now */

void setcustompalette(int color_cycle)
{
  register int i;
  int r1,r2,r3;
  int r4,r5,r6;

  switch (color_cycle)
  {
    case 0: /* No color cycles - RGB palette */
    case 1: /* RGB palette - cycles          */

      for (i = 0; i < 32; ++i)
      {
        pal[ i        * 3    ] = (unsigned char)i * 2;
        pal[(i +  64) * 3    ] = (unsigned char)0;
        pal[(i + 128) * 3    ] = (unsigned char)0;
        pal[(i + 192) * 3    ] = (unsigned char)i * 2;
  
        pal[ i        * 3 + 1] = (unsigned char)0;
        pal[(i +  64) * 3 + 1] = (unsigned char)i * 2;
        pal[(i + 128) * 3 + 1] = (unsigned char)0;
        pal[(i + 192) * 3 + 1] = (unsigned char)i * 2;
  
        pal[ i        * 3 + 2] = (unsigned char)0;
        pal[(i +  64) * 3 + 2] = (unsigned char)0;
        pal[(i + 128) * 3 + 2] = (unsigned char)i * 2;
        pal[(i + 192) * 3 + 2] = (unsigned char)i * 2;
      }

      for (i = 32; i < 64; ++i)
      {
        pal[ i        * 3    ] = (unsigned char)(63 - i) * 2;
        pal[(i +  64) * 3    ] = (unsigned char)0;
        pal[(i + 128) * 3    ] = (unsigned char)0;
        pal[(i + 192) * 3    ] = (unsigned char)(63 - i) * 2;
  
        pal[ i        * 3 + 1] = (unsigned char)0;
        pal[(i +  64) * 3 + 1] = (unsigned char)(63 - i) * 2;
        pal[(i + 128) * 3 + 1] = (unsigned char)0;
        pal[(i + 192) * 3 + 1] = (unsigned char)(63 - i) * 2;
  
        pal[ i        * 3 + 2] = (unsigned char)0;
        pal[(i +  64) * 3 + 2] = (unsigned char)0;
        pal[(i + 128) * 3 + 2] = (unsigned char)(63 - i) * 2;
        pal[(i + 192) * 3 + 2] = (unsigned char)(63 - i) * 2;
      }
      break;

    case 2: /* Monochrome to desert palette */
      for (i = 1; i < 128; ++i)
      {
        pal[ i        * 3    ] = (unsigned char)(16 + (i/4));
        pal[ i        * 3 + 1] = (unsigned char)(16 + (i/4));
        pal[ i        * 3 + 2] = (unsigned char)(16 + (i/4));
  
        pal[(i + 128) * 3    ] = (unsigned char)(16 + ((127 - i)/4));
        pal[(i + 128) * 3 + 1] = (unsigned char)(16 + ((127 - i)/4));
        pal[(i + 128) * 3 + 2] = (unsigned char)(16 + ((127 - i)/4));
      }
      break;

    case 3: /* WavyRave palette - don't tell Moby */
      r1 = ( ( random() % 40 ) + 1);
      r2 = ( ( random() % 40 ) + 1);
      r3 = ( ( random() % 40 ) + 1);
      r4 = ( ( random() % 40 ) + 1);
      r5 = ( ( random() % 40 ) + 1);
      r6 = ( ( random() % 40 ) + 1);

      for (i= 1; i< 128; i++)
      {
        pal[i         * 3]     = (unsigned char) ( cos ( i / r1 ) * sin ( i / r2 ) * 31 + 31);
        pal[i         * 3 + 1] = (unsigned char) ( sin ( i / r2 ) * cos ( i / r3 ) * 31 + 31);
        pal[i         * 3 + 2] = (unsigned char) ( sin ( i / r3 ) * sin ( i / r1 ) * 31 + 31);

        pal[(i + 128) * 3]     = (unsigned char) ( sin ( i / r4 ) * cos ( i / r5 ) * 31 + 31);
        pal[(i + 128) * 3 + 1] = (unsigned char) ( cos ( i / r5 ) * sin ( i / r6 ) * 31 + 31);
        pal[(i + 128) * 3 + 2] = (unsigned char) ( cos ( i / r6 ) * cos ( i / r4 ) * 31 + 31);
      }
      break;

    case 4: /* 'Eugene Jarvis is god' palette - Robotron */
      for (i= 1; i< 256; i++)
      {
        pal[i * 3    ] = (unsigned char) random();
        pal[i * 3 + 1] = (unsigned char) random();
        pal[i * 3 + 2] = (unsigned char) random();
      }
      break;
    default:
      break;
  }
  gl_setpalette(pal);
}

/* These last two functions are taken from Acidwarp */
void shiftpalette()
{
  int temp;
  register int x;
  int color = (int)(random() % 3);

    temp = pal[3+color];
    for(x=1; x < (256) ; ++x)
      pal[x*3+color] = pal[(x*3)+3+color];
    pal[((256)*3)-3+color] = temp;
  gl_setpalette(pal);
}

void printStrArray(char *strArray[])
{
  /* Prints an array of strings.  The array is terminated with a null string.     */
  char **strPtr;

  for (strPtr = strArray; **strPtr; ++strPtr)
    printf ("%s", *strPtr);
}

