/* Pango
 * basic-win32.c:
 *
 * Copyright (C) 1999 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include "pango.h"
#include "pangowin32.h"
#include "pango-utils.h"

static PangoEngineRange basic_ranges[] = {
  /* Language characters */
  { 0x0000, 0x02af, "*" },
  { 0x02b0, 0x02ff, "" },
  { 0x0380, 0x058f, "*" },
  { 0x0591, 0x05f4, "*" }, /* Hebrew */
  { 0x060c, 0x06f9, "" }, /* Arabic */
  { 0x0e01, 0x0e5b, "" },  /* Thai */
  { 0x1e00, 0x1fff, "*" },
  { 0x2000, 0x9fff, "*" },
  { 0xac00, 0xd7a3, "kr" },
  { 0xf900, 0xfa0b, "kr" },
  { 0xff00, 0xffe3, "*" }
};

static PangoEngineInfo script_engines[] = {
  {
    "BasicScriptEngineWin32",
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_WIN32,
    basic_ranges, G_N_ELEMENTS(basic_ranges)
  }
};

static gint n_script_engines = G_N_ELEMENTS (script_engines);

/*
 * Win32 system script engine portion
 */

static PangoGlyph 
find_char (PangoFont *font,
	   gunichar   wc)
{
  PangoWin32UnicodeSubrange subrange;
  PangoWin32Subfont *subfonts;
  int i;
  int n_subfonts;

  subrange = pango_win32_unicode_classify (wc);

  if (PANGO_WIN32_U_LAST_PLUS_ONE == subrange)
      return 0;

  n_subfonts = pango_win32_list_subfonts (font, subrange, &subfonts);

  for (i = 0; i < n_subfonts; i++)
    {
      PangoGlyph glyph;

      glyph = PANGO_WIN32_MAKE_GLYPH (subfonts[i], wc);

      if (pango_win32_has_glyph (font, glyph))
	return glyph;	  
    }

  return 0;
}

static void
set_glyph (PangoFont        *font,
	   PangoGlyphString *glyphs,
	   int               i,
	   int               offset,
	   PangoGlyph        glyph)
{
  PangoRectangle logical_rect;

  glyphs->glyphs[i].glyph = glyph;
  
  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  glyphs->log_clusters[i] = offset;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;
}

static void
swap_range (PangoGlyphString *glyphs,
	    int               start,
	    int               end)
{
  int i, j;
  
  for (i = start, j = end - 1; i < j; i++, j--)
    {
      PangoGlyphInfo glyph_info;
      gint log_cluster;
      
      glyph_info = glyphs->glyphs[i];
      glyphs->glyphs[i] = glyphs->glyphs[j];
      glyphs->glyphs[j] = glyph_info;
      
      log_cluster = glyphs->log_clusters[i];
      glyphs->log_clusters[i] = glyphs->log_clusters[j];
      glyphs->log_clusters[j] = log_cluster;
    }
}

static void 
basic_engine_shape (PangoFont        *font,
		    const char       *text,
		    gint              length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs)
{
  int n_chars;
  int i;
  const char *p;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  n_chars = g_utf8_strlen (text, length);
  pango_glyph_string_set_size (glyphs, n_chars);

  p = text;
  for (i = 0; i < n_chars; i++)
    {
      gunichar wc;
      gunichar mirrored_ch;
      PangoGlyph index;

      wc = g_utf8_get_char (p);

      if (analysis->level % 2)
	if (pango_get_mirror_char (wc, &mirrored_ch))
	  wc = mirrored_ch;

      if (wc == 0x200B || wc == 0x200E || wc == 0x200F)	/* Zero-width characters */
	{
	  set_glyph (font, glyphs, i, p - text, 0);
	}
      else
	{
	  index = find_char (font, wc);
	  if (index)
	    {
	      set_glyph (font, glyphs, i, p - text, index);
	      
	      if (g_unichar_type (wc) == G_UNICODE_NON_SPACING_MARK)
		{
		  if (i > 0)
		    {
		      PangoRectangle logical_rect, ink_rect;
		      
		      glyphs->glyphs[i].geometry.width = MAX (glyphs->glyphs[i-1].geometry.width,
							      glyphs->glyphs[i].geometry.width);
		      glyphs->glyphs[i-1].geometry.width = 0;
		      glyphs->log_clusters[i] = glyphs->log_clusters[i-1];

		      /* Some heuristics to try to guess how overstrike glyphs are
		       * done and compensate
		       */
		      pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, &ink_rect, &logical_rect);
		      if (logical_rect.width == 0 && ink_rect.x == 0)
			glyphs->glyphs[i].geometry.x_offset = (glyphs->glyphs[i].geometry.width - ink_rect.width) / 2;
		    }
		}
	    }
	  else
	    set_glyph (font, glyphs, i, p - text, pango_win32_get_unknown_glyph (font));
	}
      
      p = g_utf8_next_char (p);
    }

  /* Simple bidi support... may have separate modules later */

  if (analysis->level % 2)
    {
      int start, end;

      /* Swap all glyphs */
      swap_range (glyphs, 0, n_chars);
      
      /* Now reorder glyphs within each cluster back to LTR */
      for (start = 0; start < n_chars;)
	{
	  end = start;
	  while (end < n_chars &&
		 glyphs->log_clusters[end] == glyphs->log_clusters[start])
	    end++;
	  
	  swap_range (glyphs, start, end);
	  start = end;
	}
    }
}

static PangoCoverage *
basic_engine_get_coverage (PangoFont  *font,
			   const char *lang)
{
  PangoCoverage *result = pango_coverage_new ();
  gunichar wc;
  gint found = 0;
  gint tested = 0;
  gint irange = 0;
  gunichar last;
#if DEBUGGING
  GTimeVal tv0, tv1;
  g_get_current_time (&tv0);
#endif

  for (irange = 0; irange < G_N_ELEMENTS(basic_ranges); irange++)
    {
      for (wc = basic_ranges[irange].start; wc <= basic_ranges[irange].end; wc++)
        {
          if (find_char (font, wc))
            {
              pango_coverage_set (result, wc, PANGO_COVERAGE_EXACT);
              found++;
              last = wc;
            }
          tested++;
        }
    }

#if DEBUGGING
  {
    PangoFontDescription *desc = pango_font_describe(font);

    g_get_current_time (&tv1);
    if (tv1.tv_usec < tv0.tv_usec)
      tv1.tv_sec--, tv1.tv_usec += 1000000L;
    g_print ("\"%s\" (%d) found: %d tested: %d last: %d time: %ld.%06ld s\n",
		desc->family_name, desc->weight, 
		found, tested, last,
            tv1.tv_sec - tv0.tv_sec, tv1.tv_usec - tv0.tv_usec);

    pango_font_description_free (desc);
  }
#endif

  return result;
}

static PangoEngine *
basic_engine_win32_new (void)
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = "BasicScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof (result);
  result->script_shape = basic_engine_shape;
  result->get_coverage = basic_engine_get_coverage;

  return (PangoEngine *)result;
}

/* The following three functions provide the public module API for
 * Pango
 */
#ifdef MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_basic_##func
#else
#define MODULE_ENTRY(func) func
#endif

void 
MODULE_ENTRY(script_engine_list) (PangoEngineInfo **engines,
				  gint             *n_engines)
{
  *engines = script_engines;
  *n_engines = n_script_engines;
}

PangoEngine *
MODULE_ENTRY(script_engine_load) (const char *id)
{
  if (!strcmp (id, "BasicScriptEngineLangWin32"))
    return basic_engine_lang_new ();
  else if (!strcmp (id, "BasicScriptEngineWin32"))
    return basic_engine_win32_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}
