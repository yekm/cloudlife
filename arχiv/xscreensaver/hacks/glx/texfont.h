/* texfonts, Copyright (c) 2005-2021 Jamie Zawinski <jwz@jwz.org>
 * Loads X11 fonts into textures for use with OpenGL.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifndef __TEXTURE_FONT_H__
#define __TEXTURE_FONT_H__


#include "xft.h"


typedef struct texture_font_data texture_font_data;

/* Loads the font named by the X resource "res" and returns
   a texture-font object.
*/
extern texture_font_data *load_texture_font (Display *, char *res);

/* Bounding box of the multi-line string, in pixels,
   and overall ascent/descent of the font.
 */
extern void texture_string_metrics (texture_font_data *, const char *,
                                    XCharStruct *metrics_ret,
                                    int *ascent_ret, int *descent_ret);

/* Draws the string in the scene at the origin.
   Newlines and tab stops are honored.
   Any numbers inside [] will be rendered as a subscript.
   Assumes the font has been loaded as with load_texture_font().
 */
extern void print_texture_string (texture_font_data *, const char *);

/* Draws the string on the window at the given pixel position.
   Newlines and tab stops are honored.
   Any numbers inside [] will be rendered as a subscript.
   Assumes the font has been loaded as with load_texture_font().

   Position is 0 for center, 1 for top left, 2 for bottom left.
 */
void print_texture_label (Display *, texture_font_data *,
                          int window_width, int window_height,
                          int position, const char *string);

/* Renders the given string into the prevailing texture.
   Returns the metrics of the text, and size of the texture.
 */
void string_to_texture (texture_font_data *, const char *,
                        XCharStruct *extents_ret,
                        int *tex_width_ret, int *tex_height_ret);

/* Set the various OpenGL parameters for properly rendering things
   with a texture generated by string_to_texture().
 */
void enable_texture_string_parameters (texture_font_data *);


/* True if the string appears to be a "missing" character.  Since there is
   no way to tell whether a font contains a character or has substituted a
   "missing" glyph for it, we examine the bits to see if it is either
   solid black, or is a simple rectangle, which is what most fonts use.
 */
Bool blank_character_p (texture_font_data *, const char *);


/* Releases the texture font.
 */
extern void free_texture_font (texture_font_data *);


#ifdef HAVE_JWXYZ
extern char *texfont_unicode_character_name (texture_font_data *,
                                             unsigned long uc);
#endif


#endif /* __TEXTURE_FONT_H__ */
