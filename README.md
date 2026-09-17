# cloudlife

Xscreensaver hacks and other beautiful programs with Dear ImGui

## Arts

### AcidWarp

Psychedelic geometric patterns animated by cycling colors.

Ported from [AcidWarp’s original source](arχiv/acidwarp/acidwarp.c).

```
ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
Ported to Linux by Steven Wills
```

### AcidWorm

Colorful worms weave across the screen, leaving animated trails.

Ported from [Acidworm 0.2’s original source](arχiv/acidworm-0.2/acidworm.c).

```
/* Acidworm 0.2 - By Aaron Tiensivu - tiensivu@pilot.msu.edu */
/* Gross hack of Eric P. Scott's 'worm' program              */
/* Inspired by Acidwarp and other eyecandy programs          */
```

### RDbomb

Reaction-diffusion patterns grow into spots, stripes, and organic textures.

Ported from [XScreenSaver's rdbomb.c](arχiv/xscreensaver/hacks/rdbomb.c).

```
 *  reaction/diffusion textures
 *  Copyright (c) 1997 Scott Draves spot@transmeta.com
```

```
      /* John E. Pearson "Complex Patterns in a Simple System"
         Science, July 1993 */
```
[John E. Pearson’s paper](arχiv/p5118_0189.pdf)

### Minskytron

A simple iterative system traces intricate geometric curves.
by Marvin Minsky, early 1960s

https://www.masswerk.at/minskytron/

https://beta.dwitter.net/d/26514

### Cloudlife

A colorful Conway’s Life variant whose aging cells disrupt long-lived formations.

Ported from [XScreenSaver's cloudlife.c](arχiv/xscreensaver/hacks/cloudlife.c).

```
by Don Marti <dmarti@zgp.org>, ~2003
```

### Thornbird

A continuously changing chaotic attractor draws delicate branching patterns.

Ported from [XScreenSaver's thornbird.c](arχiv/xscreensaver/hacks/thornbird.c).

```
continuously varying Thornbird set
Copyright (c) 1996 by Tim Auckland <tda10.geo@yahoo.com>
```

### Discrete

Discrete chaotic maps build intricate clouds of colored points.

Ported from [XScreenSaver's discrete.c](arχiv/xscreensaver/hacks/discrete.c).

```
chaotic mappings
 * Copyright (c) 1996 by Tim Auckland <tda10.geo@yahoo.com>
 * 08-Aug-1996: Adapted from hop.c Copyright (c) 1991 by Patrick J. Naughton.
```

### IFS

Iterated function systems generate evolving, multicolored fractals.

Ported from [XScreenSaver's ifs.c](arχiv/xscreensaver/hacks/ifs.c).

```
This version by Chris Le Sueur <thefishface@gmail.com>, Feb 2005
Many improvements by Robby Griffin <rmg@terc.edu>, Mar 2006
Multi-coloured mode added by Jack Grahl <j.grahl@ucl.ac.uk>, Jan 2007
```

### Vermiculate

Wormlike trails form tangled, colorful patterns.

Ported from [XScreenSaver's vermiculate.c](arχiv/xscreensaver/hacks/vermiculate.c).

```
 *  @(#) Copyright (C) 2001 Tyler Pierce (tyler@alumni.brown.edu)
 *  The full program, with documentation, is available at:
 *    http://freshmeat.net/projects/fdm
```

### Hopalong

Hopalong and related chaotic maps trace elaborate point patterns.

Ported from [XScreenSaver's hopalong.c](arχiv/xscreensaver/hacks/hopalong.c).

```
 * Copyright (c) 1991 by Patrick J. Naughton.

 * 24-Jun-1997: EJK and RR functions stolen from xmartin2.2
 *              Ed Kubaitis <ejk@ux2.cso.uiuc.edu> ejk functions and xmartin
 *              Renaldo Recuerdo rr function, generalized exponent version
 *              of the Barry Martin's square root function
 * 27-Jul-1995: added Peter de Jong's hop from Scientific American
 *              July 87 p. 111.  Sometimes they are amazing but there are a
 *              few duds (I did not see a pattern in the parameters).
 * 09-Dec-1994: added Barry Martin's sine hop
 * 23-Mar-1988: Coded HOPALONG routines from Scientific American Sept. 86 p. 14.
 *              Hopalong was attributed to Barry Martin of Aston University
 *              (Birmingham, England)

```

### Marbling

Simulates paper marbling with drops of color stretched and combed into flowing patterns.

Ported from [XScreenSaver's marbling.c](arχiv/xscreensaver/hacks/marbling.c).
Initial version by Jamie Zawinski; pthreads and CPU-specific vector operations
by Dave Odell <dmo2118@gmail.com>.

```
marbling, Copyright © 2021-2022 Jamie Zawinski <jwz@jwz.org>
```

### XLyap

Maps stable and chaotic regions of periodically forced nonlinear systems using Lyapunov exponents.

Ported from [XScreenSaver's xlyap.c](arχiv/xscreensaver/hacks/xlyap.c),
written by Ron Record (rr@sco), 3 September 1991 (Lyap 2.3, patchlevel 4).


### Hopalong 3D

Layers Hopalong orbits into a three-dimensional point cloud.

### Collatz Birb 3D

Turns Collatz sequences into branching, birdlike structures in three dimensions.

### Attractor

Explores a collection of strange attractors with adjustable map parameters.

### Physarum

Simulates slime-mold agents following pheromone trails to form organic networks.

### Prime UMAP 3D

Embeds integers in three dimensions with UMAP, grouping them by shared prime factors.


## using with sway

```
exec swayidle \
    timeout 240 "swaymsg exec 'cloudlife -a 8 -S'" \
    resume "swaymsg exec 'killall cloudlife'"
```

Some sway configuretion regarding window placement shold be done.

Instead of directly calling `cloudlife` some bash script sohould
be used and `-a` number is chosen randomly from preferred screensavers.
TODO: support multiple monitors via filtering by title. How to
place a window in current workspace on certain output in sway config?

https://gist.github.com/pschmitt/909f880e8c7924fab056d42a3d30f9a5
https://gist.github.com/mkalinski/ec112091dc9aa1e9f5e039ed7dd4b1fe#file-sway-track-pseudo-maximize-placeholder-sh

