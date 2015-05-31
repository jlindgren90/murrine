/* Murrine theme engine
 * Copyright (C) 2006-2007-2008-2009 Andrea Cimitan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <cairo.h>

#include "murrine_draw.h"
#include "murrine_style.h"
#include "murrine_types.h"
#include "support.h"
#include "cairo-support.h"
#include "raico-blur.h"

static void
murrine_draw_inset (cairo_t *cr,
                    const MurrineRGB *bg_color,
                    double x, double y, double w, double h,
                    double radius, uint8 corners)
{
	MurrineRGB highlight, shadow;
	radius = MIN (radius, MIN (w/2.0, h/2.0));

	murrine_shade (bg_color, 1.15, &highlight);
	murrine_shade (bg_color, 0.4, &shadow);

	/* highlight */
	cairo_move_to (cr, x+w+(radius*-0.2928932188), y-(radius*-0.2928932188));

	if (corners & MRN_CORNER_TOPRIGHT)
		cairo_arc (cr, x+w-radius, y+radius, radius, M_PI*1.75, M_PI*2);
	else
		cairo_line_to (cr, x+w, y);

	if (corners & MRN_CORNER_BOTTOMRIGHT)
		cairo_arc (cr, x+w-radius, y+h-radius, radius, 0, M_PI*0.5);
	else
		cairo_line_to (cr, x+w, y+h);

	if (corners & MRN_CORNER_BOTTOMLEFT)
		cairo_arc (cr, x+radius, y+h-radius, radius, M_PI*0.5, M_PI*0.75);
	else
		cairo_line_to (cr, x, y+h);

	murrine_set_color_rgba (cr, &highlight, 0.48);
	cairo_stroke (cr);

	/* shadow */
	cairo_move_to (cr, x+(radius*0.2928932188), y+h+(radius*-0.2928932188));

	if (corners & MRN_CORNER_BOTTOMLEFT)
		cairo_arc (cr, x+radius, y+h-radius, radius, M_PI*0.75, M_PI);
	else
		cairo_line_to (cr, x, y+h);

	if (corners & MRN_CORNER_TOPLEFT)
		cairo_arc (cr, x+radius, y+radius, radius, M_PI, M_PI*1.5);
	else
		cairo_line_to (cr, x, y);

	if (corners & MRN_CORNER_TOPRIGHT)
		cairo_arc (cr, x+w-radius, y+radius, radius, M_PI*1.5, M_PI*1.75);
	else
		cairo_line_to (cr, x+w, y);

	murrine_set_color_rgba (cr, &shadow, 0.12);
	cairo_stroke (cr);
}

static void
murrine_draw_highlight_and_shade (cairo_t *cr,
                                  const MurrineColors    *colors,
                                  const ShadowParameters *widget,
                                  int width, int height, int radius)
{
	MurrineRGB highlight;
	MurrineRGB shadow;
	uint8 corners = widget->corners;
	double x = 1.0;
	double y = 1.0;
	width  -= 3;
	height -= 3;
	radius = MIN (radius, MIN ((double)width/2.0, (double)height/2.0));

	if (radius < 0)
		radius = 0;

	murrine_shade (&colors->bg[0], 1.15, &highlight);
	murrine_shade (&colors->bg[0], 0.4, &shadow);

	cairo_save (cr);

	/* Top/Left highlight */
	if (corners & MRN_CORNER_BOTTOMLEFT)
		cairo_move_to (cr, x, y+height-radius);
	else
		cairo_move_to (cr, x, y+height);

	murrine_rounded_corner (cr, x, y, radius, corners & MRN_CORNER_TOPLEFT);

	if (corners & MRN_CORNER_TOPRIGHT)
		cairo_line_to (cr, x+width-radius, y);
	else
		cairo_line_to (cr, x+width, y);

	if (widget->shadow & MRN_SHADOW_OUT)
		murrine_set_color_rgba (cr, &highlight, 0.5);
	else
		murrine_set_color_rgba (cr, &shadow, 0.13);

	cairo_stroke (cr);

	/* Bottom/Right highlight -- this includes the corners */
	cairo_move_to (cr, x+width-radius, y); /* topright and by radius to the left */
	murrine_rounded_corner (cr, x+width, y, radius, corners & MRN_CORNER_TOPRIGHT);
	murrine_rounded_corner (cr, x+width, y+height, radius, corners & MRN_CORNER_BOTTOMRIGHT);
	murrine_rounded_corner (cr, x, y+height, radius, corners & MRN_CORNER_BOTTOMLEFT);

	if (widget->shadow & MRN_SHADOW_OUT)
		murrine_set_color_rgba (cr, &shadow, 0.13);
	else
		murrine_set_color_rgba (cr, &highlight, 0.5);

	cairo_stroke (cr);

	cairo_restore (cr);
}

static void
murrine_rgba_draw_button (cairo_t *cr,
                          const MurrineColors    *colors,
                          const WidgetParameters *widget,
                          const ButtonParameters *button,
                          int x, int y, int width, int height,
                          boolean horizontal)
{
	double os = (widget->xthickness > 2 && widget->ythickness > 2) ? 1.0 : 0.0;
	double glow_shade_new = widget->glow_shade;
	double highlight_shade_new = widget->highlight_shade;
	double lightborder_shade_new = widget->lightborder_shade;
	MurrineGradients mrn_gradient_new = widget->mrn_gradient;
	MurrineRGB border;
	MurrineRGB fill = colors->bg[widget->state_type];

	murrine_get_fill_color (&fill, &mrn_gradient_new);

	if (widget->disabled)
	{
		mrn_gradient_new = murrine_get_decreased_gradient_shades (widget->mrn_gradient, 3.0);
		mrn_gradient_new.border_shades[0] = murrine_get_decreased_shade (widget->mrn_gradient.border_shades[0], 2.0);
		mrn_gradient_new.border_shades[1] = murrine_get_decreased_shade (widget->mrn_gradient.border_shades[1], 2.0);
		glow_shade_new = murrine_get_decreased_shade (widget->glow_shade, 2.0);
		highlight_shade_new = murrine_get_decreased_shade (widget->highlight_shade, 2.0);
		lightborder_shade_new = murrine_get_decreased_shade (widget->lightborder_shade, 2.0);
		murrine_shade (&fill, murrine_get_contrast(0.5, widget->contrast), &border);
	}
	else
		murrine_shade (&fill, murrine_get_contrast(0.38, widget->contrast), &border);

	/* Default button */
	if (widget->is_default)
	{
		murrine_shade (&border, murrine_get_contrast(0.8, widget->contrast), &border);

		if (button->has_default_button_color)
		{
			mrn_gradient_new.has_border_colors = FALSE;
			mrn_gradient_new.has_gradient_colors = FALSE;
			murrine_mix_color (&fill, &button->default_button_color, 0.8, &fill);
		}
		else
			murrine_mix_color (&fill, &colors->spot[1], 0.2, &fill);


		if (mrn_gradient_new.has_border_colors)
		{
			murrine_shade (&mrn_gradient_new.border_colors[0], 0.8, &mrn_gradient_new.border_colors[0]);
			murrine_shade (&mrn_gradient_new.border_colors[1], 0.8, &mrn_gradient_new.border_colors[1]);
		}
	}

	if (!horizontal)
		murrine_exchange_axis (cr, &x, &y, &width, &height);

	cairo_translate (cr, x, y);

	if (!widget->active && !widget->disabled && widget->reliefstyle > 1 && os > 0.5)
	{
		if (widget->reliefstyle == 5)
			murrine_draw_shadow (cr, &widget->parentbg,
			                     0.5, 0.5, width-1, height-1,
			                     widget->roundness+1, widget->corners,
			                     widget->reliefstyle,
			                     mrn_gradient_new, 0.5);
		else
		{
			murrine_draw_shadow (cr, &border,
			                     os-0.5, os-0.5, width-(os*2)+1, height-(os*2)+1,
			                     widget->roundness+1, widget->corners,
			                     widget->reliefstyle,
			                     mrn_gradient_new, 0.08);
		}
	}
	else if (widget->reliefstyle != 0 && os > 0.5)
	{
		mrn_gradient_new = murrine_get_inverted_border_shades (mrn_gradient_new);
		murrine_draw_inset (cr, &widget->parentbg, os-0.5, os-0.5,
		                    width-(os*2)+1, height-(os*2)+1,
		                    widget->roundness+1, widget->corners);
	}

	murrine_mix_color (&border, &widget->parentbg, 0.2, &border);
	murrine_mix_color (&border, &fill, 0.25, &border);

	cairo_save (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	murrine_rounded_rectangle_closed (cr, os+1, os+1, width-(os*2)-2, height-(os*2)-2, widget->roundness-1, widget->corners);
	cairo_clip_preserve (cr);

	murrine_draw_glaze (cr, &fill,
	                    glow_shade_new, highlight_shade_new, !widget->active ? lightborder_shade_new : 1.0,
	                    mrn_gradient_new, widget,
	                    os+1, os+1, width-(os*2)-2, height-(os*2)-2,
	                    widget->roundness-1, widget->corners, horizontal);

	cairo_restore (cr);

	/* Draw pressed button shadow */
	if (widget->active)
	{
		cairo_pattern_t *pat;
		MurrineRGB shadow;

		murrine_shade (&fill, 0.94, &shadow);

		cairo_save (cr);

		murrine_rounded_rectangle_closed (cr, os+1, os+1, width-(os*2)-2, height-(os*2)-2, widget->roundness-1,
		                                  widget->corners & (MRN_CORNER_TOPLEFT | MRN_CORNER_TOPRIGHT | MRN_CORNER_BOTTOMLEFT));
		cairo_clip (cr);

		cairo_rectangle (cr, os+1, os+1, width-(os*2)-2, 3);
		pat = cairo_pattern_create_linear (os+1, os+1, os+1, os+4);
		murrine_pattern_add_color_stop_rgba (pat, 0.0, &shadow, 0.58);
		murrine_pattern_add_color_stop_rgba (pat, 1.0, &shadow, 0.0);
		cairo_set_source (cr, pat);
		cairo_fill (cr);
		cairo_pattern_destroy (pat);

		cairo_rectangle (cr, os+1, os+1, 3, height-(os*2)-2);
		pat = cairo_pattern_create_linear (os+1, os+1, os+4, os+1);
		murrine_pattern_add_color_stop_rgba (pat, 0.0, &shadow, 0.58);
		murrine_pattern_add_color_stop_rgba (pat, 1.0, &shadow, 0.0);
		cairo_set_source (cr, pat);
		cairo_fill (cr);
		cairo_pattern_destroy (pat);

		cairo_restore (cr);
	}

	murrine_draw_border (cr, &border,
	                     os+0.5, os+0.5, width-(os*2)-1, height-(os*2)-1,
	                     widget->roundness, widget->corners,
	                     mrn_gradient_new, 1.0);
}

static void
murrine_rgba_draw_entry (cairo_t *cr,
                         const MurrineColors    *colors,
                         const WidgetParameters *widget,
                         const FocusParameters  *focus,
                         int x, int y, int width, int height)
{
	MurrineGradients mrn_gradient_new = widget->mrn_gradient;
	const MurrineRGB *base = &colors->base[widget->state_type];
	MurrineRGB border = colors->shade[widget->disabled ? 4 : 5];
	int radius = CLAMP (widget->roundness, 0, 3);

	murrine_shade (&border, 0.92, &border);

	if (widget->focus)
		border = focus->color;

	cairo_translate (cr, x+0.5, y+0.5);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	/* Fill the entry's base color */
	cairo_rectangle (cr, 1.5, 1.5, width-4, height-4);
	murrine_set_color_rgba (cr, base, ENTRY_OPACITY);
	cairo_fill (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	if (widget->reliefstyle != 0)
		murrine_draw_inset (cr, &widget->parentbg, 0, 0, width-1, height-1, radius+1, widget->corners);

	/* Draw the focused border */
	if (widget->focus)
	{
		MurrineRGB focus_shadow;
		murrine_shade (&border, 1.54, &focus_shadow);

		cairo_rectangle (cr, 2, 2, width-5, height-5);
		murrine_set_color_rgba (cr, &focus_shadow, 0.5);
		cairo_stroke(cr);
	}
	else if (widget->mrn_gradient.gradients)
	{
		MurrineRGB highlight;
		murrine_shade (base, 1.15, &highlight);

		cairo_move_to (cr, 2, height-3);
		cairo_line_to (cr, 2, 2);
		cairo_line_to (cr, width-3, 2);

		murrine_set_color_rgba (cr, &highlight, widget->disabled ? 0.3 : 0.6);
		cairo_stroke (cr);
	}

	mrn_gradient_new = murrine_get_inverted_border_shades (mrn_gradient_new);

	/* Draw border */
	murrine_draw_border (cr, &border,
	                     1, 1, width-3, height-3,
	                     radius, widget->corners,
	                     mrn_gradient_new, 1.0);
}

static void
murrine_scale_draw_gradient (cairo_t *cr,
                             const MurrineRGB *c1,
                             const MurrineRGB *c2,
                             double lightborder_shade,
                             int lightborderstyle,
                             int roundness, uint8 corners,
                             int x, int y, int width, int height,
                             boolean horizontal)
{
	murrine_set_color_rgb (cr, c1);
	murrine_rounded_rectangle_closed (cr, x, y, width, height, roundness, corners);
	cairo_fill (cr);

	if (lightborder_shade != 1.0)
	{
		cairo_pattern_t *pat;
		double fill_pos = horizontal ? 1.0-1.0/(double)(height-2) :
		                               1.0-1.0/(double)(width-2);
		MurrineRGB lightborder;
		murrine_shade (c1, lightborder_shade, &lightborder);

		roundness < 2 ? cairo_rectangle (cr, x, y, width, height) :
		                clearlooks_rounded_rectangle (cr, x+1, y+1, width-2, height-2, roundness-1, corners);
		pat = cairo_pattern_create_linear (x+1, y+1, horizontal ? x+1 : width+x+1, horizontal ? height+y+1 : y+1);

		murrine_pattern_add_color_stop_rgba (pat, 0.00,     &lightborder, 0.75);
		murrine_pattern_add_color_stop_rgba (pat, fill_pos, &lightborder, 0.75);
		murrine_pattern_add_color_stop_rgba (pat, fill_pos, &lightborder, 0.0);
		murrine_pattern_add_color_stop_rgba (pat, 1.00,     &lightborder, 0.0);

		cairo_set_source (cr, pat);
		cairo_pattern_destroy (pat);

		cairo_stroke (cr);
	}

	murrine_set_color_rgb (cr, c2);
	murrine_rounded_rectangle (cr, x, y, width, height, roundness, corners);
	cairo_stroke (cr);
}

static void
murrine_scale_draw_trough (cairo_t *cr,
                           const MurrineRGB *c1,
                           const MurrineRGB *c2,
                           MurrineGradients mrn_gradient,
                           int roundness, uint8 corners,
                           int x, int y, int width, int height,
                           boolean horizontal)
{
	murrine_draw_trough (cr, c1, x, y, width, height, roundness, corners, mrn_gradient, 1.0, horizontal);
	murrine_draw_trough_border (cr, c2, x, y, width, height, roundness, corners, mrn_gradient, 1.0, horizontal);
}

#define TROUGH_SIZE 6
static void
murrine_rgba_draw_scale_trough (cairo_t *cr,
                                const MurrineColors    *colors,
                                const WidgetParameters *widget,
                                const SliderParameters *slider,
                                int x, int y, int width, int height)
{
	int     trough_width, trough_height;
	double  translate_x, translate_y;

	cairo_save (cr);

	if (slider->horizontal)
	{
		trough_width  = width;
		trough_height = TROUGH_SIZE;

		translate_x   = x;
		translate_y   = y+(height/2)-(TROUGH_SIZE/2);
	}
	else
	{
		trough_width  = TROUGH_SIZE;
		trough_height = height;

		translate_x   = x+(width/2)-(TROUGH_SIZE/2);
		translate_y   = y;
	}

	cairo_translate (cr, translate_x+0.5, translate_y+0.5);

	if (!slider->fill_level && widget->reliefstyle != 0)
		murrine_draw_inset (cr, &widget->parentbg, 0, 0, trough_width, trough_height, widget->roundness, widget->corners);

	if (!slider->lower && !slider->fill_level)
	{
		MurrineRGB fill, border;
		murrine_shade (&colors->bg[GTK_STATE_ACTIVE], 1.0, &fill);
		murrine_shade (&colors->bg[GTK_STATE_ACTIVE], murrine_get_contrast(0.82, widget->contrast), &border);

		murrine_scale_draw_trough (cr, &fill, &border, widget->mrn_gradient,
		                           widget->roundness, widget->corners,
		                           1.0, 1.0, trough_width-2, trough_height-2,
		                           slider->horizontal);
	}
	else
	{
		MurrineRGB fill, border;
		murrine_mix_color (&colors->bg[GTK_STATE_SELECTED], &widget->parentbg, widget->disabled ? 0.25 : 0.0, &fill);
		murrine_shade (&fill, murrine_get_contrast(0.65, widget->contrast), &border);

		murrine_scale_draw_gradient (cr, &fill, &border,
		                             widget->disabled ? 1.0 : widget->lightborder_shade,
		                             widget->lightborderstyle,
		                             widget->roundness, widget->corners,
		                             1.0, 1.0, trough_width-2, trough_height-2,
		                             slider->horizontal);
	}

	cairo_restore (cr);
}

static void
murrine_draw_slider_path (cairo_t *cr,
                          int x, int y, int width, int height,
                          int roundness)
{
	int radius = MIN (roundness, MIN (width/2.0, height/2.0));

	cairo_move_to (cr, x+radius, y);
	cairo_arc (cr, x+width-radius, y+radius, radius, M_PI*1.5, M_PI*2);
	cairo_line_to (cr, x+width, y+height-width/2.0);
	cairo_line_to (cr, x+width/2.0, y+height);
	cairo_line_to (cr, x, y+height-width/2.0);
	cairo_arc (cr, x+radius, y+radius, radius, M_PI, M_PI*1.5);
}

static void
murrine_rgba_draw_slider (cairo_t *cr,
                          const MurrineColors    *colors,
                          const WidgetParameters *widget,
                          const SliderParameters *slider,
                          int x, int y, int width, int height)
{
	int os = (widget->xthickness > 2 && widget->ythickness > 2) ? 1 : 0;
	double glow_shade_new = widget->glow_shade;
	double highlight_shade_new = widget->highlight_shade;
	double lightborder_shade_new = widget->lightborder_shade;
	MurrineGradients mrn_gradient_new = widget->mrn_gradient;
	MurrineRGB border;
	MurrineRGB fill = colors->bg[widget->state_type];

	murrine_get_fill_color (&fill, &mrn_gradient_new);

	if (widget->disabled)
	{
		mrn_gradient_new = murrine_get_decreased_gradient_shades (widget->mrn_gradient, 3.0);
		mrn_gradient_new.border_shades[0] = murrine_get_decreased_shade (widget->mrn_gradient.border_shades[0], 2.0);
		mrn_gradient_new.border_shades[1] = murrine_get_decreased_shade (widget->mrn_gradient.border_shades[1], 2.0);
		glow_shade_new = murrine_get_decreased_shade (widget->glow_shade, 2.0);
		highlight_shade_new = murrine_get_decreased_shade (widget->highlight_shade, 2.0);
		lightborder_shade_new = murrine_get_decreased_shade (widget->lightborder_shade, 2.0);
		murrine_shade (&fill, murrine_get_contrast(0.5, widget->contrast), &border);
	}
	else
		murrine_shade (&fill, murrine_get_contrast(0.38, widget->contrast), &border);

	if (!slider->horizontal)
		murrine_exchange_axis (cr, &x, &y, &width, &height);

	cairo_save (cr);

	cairo_translate (cr, x+0.5, y+0.5);

	if (!widget->active && !widget->disabled && widget->reliefstyle > 1 && os > 0)
	{
		murrine_draw_slider_path (cr, os-1, os, width-(os*2)+2, height-(os*2)+1, widget->roundness+1);
		if (widget->reliefstyle == 5)
			murrine_draw_shadow_from_path (cr, &widget->parentbg,
			                               os-1, os, width-(os*2)+2, height-(os*2)+1,
			                               widget->reliefstyle,
			                               mrn_gradient_new, 0.5);
		else
			murrine_draw_shadow_from_path (cr, &border,
			                              os-1, os, width-(os*2)+2, height-(os*2)+1,
			                              widget->reliefstyle,
			                              mrn_gradient_new, 0.08);
	}

	murrine_mix_color (&border, &widget->parentbg, 0.2, &border);
	murrine_mix_color (&border, &fill, 0.25, &border);

	cairo_save (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	murrine_draw_slider_path (cr, os, os+1, width-(os*2), height-(os*2)-1, widget->roundness);
	cairo_clip_preserve (cr);

	murrine_draw_glaze (cr, &fill,
	                    glow_shade_new, highlight_shade_new, !widget->active ? lightborder_shade_new : 1.0,
	                    mrn_gradient_new, widget,
	                    os, os+1, width-(os*2), height-(os*2)-1,
	                    widget->roundness, widget->corners, TRUE);

	cairo_restore (cr);

	murrine_draw_slider_path (cr, os, os+1, width-(os*2), height-(os*2)-1, widget->roundness);

	murrine_draw_border_from_path (cr, &border,
	                     os, os+1, width-(os*2), height-(os*2)-1,
	                     mrn_gradient_new, 1.0);

	cairo_restore (cr);

	if (!slider->horizontal)
		murrine_exchange_axis (cr, &x, &y, &width, &height);
}

static void
murrine_rgba_draw_spinbutton (cairo_t *cr,
	                          const MurrineColors    *colors,
	                          const WidgetParameters *widget,
	                          const SpinbuttonParameters *spinbutton,
	                          int x, int y, int width, int height,
	                          boolean horizontal)
{
	ButtonParameters button;
	button.has_default_button_color = FALSE;

	cairo_save (cr);

	widget->style_functions->draw_button (cr, colors, widget, &button, x, y, width, height, horizontal);

	cairo_restore (cr);

	switch (spinbutton->style)
	{
		default:
		case 0:
			break;
		case 1:
		{
			MurrineRGB line = colors->shade[!widget->disabled ? 8 : 6];
			MurrineRGB highlight = colors->bg[widget->state_type];
			double lightborder_shade_new = widget->lightborder_shade;
			MurrineGradients mrn_gradient_new = widget->mrn_gradient;

			if (widget->disabled)
			{
				lightborder_shade_new = murrine_get_decreased_shade (widget->lightborder_shade, 2.0);
				mrn_gradient_new.border_shades[0] = murrine_get_decreased_shade (widget->mrn_gradient.border_shades[0], 2.0);
				mrn_gradient_new.border_shades[1] = murrine_get_decreased_shade (widget->mrn_gradient.border_shades[1], 2.0);
			}
			else
				murrine_shade (&colors->shade[8], 0.95, &line);

			/* adjust line accordingly to buttons */
			if (widget->mrn_gradient.has_border_colors)
				murrine_mix_color (&mrn_gradient_new.border_colors[0], &mrn_gradient_new.border_colors[1], 0.5, &line);
			else
			{
				murrine_mix_color (&line, &widget->parentbg, 0.2, &line);
				if (widget->mrn_gradient.has_gradient_colors)
					murrine_mix_color (&line, &mrn_gradient_new.gradient_colors[2], 0.4, &line);
				else
					murrine_mix_color (&line, &colors->bg[widget->state_type], 0.25, &line);
			}
			murrine_shade (&line, (mrn_gradient_new.border_shades[0]+mrn_gradient_new.border_shades[1])/2.0, &line);

			/* adjust highlight accordingly to buttons */
			if (widget->mrn_gradient.has_gradient_colors)
				murrine_shade (&mrn_gradient_new.gradient_colors[2], mrn_gradient_new.gradient_shades[2], &highlight);
			murrine_shade (&highlight, lightborder_shade_new*mrn_gradient_new.gradient_shades[2], &highlight);

			/* this will align the path to the cairo grid */
			if (height % 2 != 0)
				height++;

			cairo_move_to (cr, x+2, y+height/2.0-0.5);
			cairo_line_to (cr, width-3,  y+height/2.0-0.5);
			murrine_set_color_rgb (cr, &line);
			cairo_stroke (cr);

			cairo_move_to (cr, x+3, y+height/2.0+0.5);
			cairo_line_to (cr, width-4,  y+height/2.0+0.5);
			murrine_set_color_rgba (cr, &highlight, 0.5);
			cairo_stroke (cr);
			break;
		}
	}
}

static void
murrine_rgba_draw_progressbar_trough (cairo_t *cr,
                                      const MurrineColors    *colors,
                                      const WidgetParameters *widget,
                                      const ProgressBarParameters *progressbar,
                                      int x, int y, int width, int height)
{
	MurrineRGB border, fill;
	int roundness = MIN (widget->roundness, MIN ((height-2.0)/2.0, (width-2.0)/2.0));
	boolean horizontal = progressbar->orientation < 2;

	murrine_shade (&colors->bg[GTK_STATE_ACTIVE], 1.0, &fill);
	murrine_shade (&colors->bg[GTK_STATE_ACTIVE], murrine_get_contrast(0.82, widget->contrast), &border);

	/* Create trough box */
	murrine_draw_trough (cr, &fill, x+1, y+1, width-2, height-2, roundness-1, widget->corners, widget->mrn_gradient, 0.8, horizontal);

	/* Draw border */
	murrine_draw_trough_border (cr, &border, x+0.5, y+0.5, width-1, height-1, roundness, widget->corners, widget->mrn_gradient, 0.8, horizontal);

	if (widget->mrn_gradient.gradients &&
	    widget->mrn_gradient.trough_shades[0] == 1.0 &&
	    widget->mrn_gradient.trough_shades[1] == 1.0)
	{
		cairo_pattern_t  *pat;
		MurrineRGB        shadow;

		murrine_shade (&border, 0.94, &shadow);

		/* clip the corners of the shadows */
		murrine_rounded_rectangle_closed (cr, x+1, y+1, width-2, height-2, roundness, widget->corners);
		cairo_clip (cr);

		/* Top shadow */
		cairo_rectangle (cr, x+1, y+1, width-2, 4);
		pat = cairo_pattern_create_linear (x, y, x, y+4);
		murrine_pattern_add_color_stop_rgba (pat, 0.0, &shadow, 0.26);
		murrine_pattern_add_color_stop_rgba (pat, 1.0, &shadow, 0.0);
		cairo_set_source (cr, pat);
		cairo_fill (cr);
		cairo_pattern_destroy (pat);

		/* Left shadow */
		cairo_rectangle (cr, x+1, y+1, 4, height-2);
		pat = cairo_pattern_create_linear (x, y, x+4, y);
		murrine_pattern_add_color_stop_rgba (pat, 0.0, &shadow, 0.26);
		murrine_pattern_add_color_stop_rgba (pat, 1.0, &shadow, 0.0);
		cairo_set_source (cr, pat);
		cairo_fill (cr);
		cairo_pattern_destroy (pat);
	}
}

static void
murrine_rgba_draw_progressbar_fill (cairo_t *cr,
                                    const MurrineColors         *colors,
                                    const WidgetParameters      *widget,
                                    const ProgressBarParameters *progressbar,
                                    int x, int y, int width, int height,
                                    gint offset)
{
	double     tile_pos = 0;
	double     stroke_width;
	int        x_step;
	int        roundness;
	MurrineRGB border = colors->spot[2];
	MurrineRGB effect;
	MurrineRGB fill = colors->spot[1];

	murrine_get_fill_color (&fill, &widget->mrn_gradient);
	murrine_shade (&fill, murrine_get_contrast(0.65, widget->contrast), &effect);

	/* progressbar->orientation < 2 == boolean is_horizontal */
	if (progressbar->orientation < 2)
	{
		if (progressbar->orientation == MRN_ORIENTATION_LEFT_TO_RIGHT)
			rotate_mirror_translate (cr, 0, x, y, FALSE, FALSE);
		else
			rotate_mirror_translate (cr, 0, x+width, y, TRUE, FALSE);
	}
	else
	{
		int tmp = height; height = width; width = tmp;

		x = x+1; y = y-1; width = width+2; height = height-2;

		if (progressbar->orientation == MRN_ORIENTATION_TOP_TO_BOTTOM)
			rotate_mirror_translate (cr, M_PI/2, x, y, FALSE, FALSE);
		else
			rotate_mirror_translate (cr, M_PI/2, x, y+width, TRUE, FALSE);
	}

	roundness = MIN (widget->roundness-widget->xthickness, height/2.0);
	int yos = 0;
	if ((2*roundness > width) && roundness > 0)
	{
		int h = height*sin((M_PI*(width))/(4*roundness));
		roundness = round(width/2.0);
		yos = 0.5+(height-h)/2.0;
		height = h;
	}
	stroke_width = height*2;
	x_step = (((float)stroke_width/10)*offset);

	cairo_save (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	murrine_rounded_rectangle_closed (cr, 2, 1+yos, width-4, height-2, roundness-1, widget->corners);
	cairo_clip (cr);

	cairo_rectangle (cr, 2, 1+yos, width-4, height-2);

	murrine_draw_glaze (cr, &fill,
	                    widget->glow_shade, widget->highlight_shade, widget->lightborder_shade,
	                    widget->mrn_gradient, widget, 2, 1+yos, width-4, height-2,
	                    roundness, widget->corners, TRUE);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	switch (progressbar->style)
	{
		case 0:
			break;
		default:
		case 1:
		{
			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

			/* Draw strokes */
			while (stroke_width > 0 && tile_pos <= width+x_step)
			{
				cairo_move_to (cr, stroke_width/2-x_step, 0);
				cairo_line_to (cr, stroke_width-x_step, 0);
				cairo_line_to (cr, stroke_width/2-x_step, height);
				cairo_line_to (cr, -x_step, height);

				cairo_translate (cr, stroke_width, 0);
				tile_pos += stroke_width;
			}

			murrine_set_color_rgba (cr, &effect, 0.15);
			cairo_fill (cr);
			break;
		}
		case 2:
		{
			MurrineRGB highlight;
			int step = 18;
			int i;

			murrine_shade (&fill, widget->lightborder_shade*widget->highlight_shade, &highlight);

			for (i=step; i<width-3; i+=step)
			{
				cairo_move_to (cr, i-0.5, 1);
				cairo_line_to (cr, i-0.5, height-1);
				murrine_set_color_rgba (cr, &highlight, 0.5*widget->mrn_gradient.rgba_opacity);
				cairo_stroke (cr);

				cairo_move_to (cr, i+0.5, 1);
				cairo_line_to (cr, i+0.5, height-1);
				murrine_set_color_rgba (cr, &effect, 0.25);
				cairo_stroke (cr);
			}
			break;
		}
	}

	cairo_restore (cr);

	cairo_save (cr);

	murrine_rounded_rectangle_closed (cr, 0.5, -0.5+yos, width-1, height+1, roundness-1, widget->corners);
	cairo_clip (cr);

	/* Draw border */
	murrine_mix_color (&border, &fill, 0.28, &border);
	murrine_draw_border (cr, &border,
	                     1.5, 0.5+yos, width-3, height-1,
	                     roundness, widget->corners,
	                     widget->mrn_gradient, 1.0);
	cairo_restore (cr);
}

static void
murrine_rgba_draw_menubar (cairo_t *cr,
                           const MurrineColors *colors,
                           const WidgetParameters *widget,
                           int x, int y, int width, int height,
                           int menubarstyle)
{
	const MurrineRGB *fill = &colors->bg[0];
	MurrineRGB dark = colors->shade[4];

	if(widget->mrn_gradient.has_border_colors)
		dark = widget->mrn_gradient.border_colors[1];

	cairo_translate (cr, x, y);
	cairo_rectangle (cr, 0, 0, width, height);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	/* Glass menubar */
	switch (menubarstyle)
	{
		default:
		case 0:
		{
			cairo_pattern_t *pat;

			pat = cairo_pattern_create_linear (0, 0, width, 0);
			murrine_pattern_add_color_stop_rgba (pat, 0.0, fill, MENUBAR_OPACITY);
			murrine_pattern_add_color_stop_rgba (pat, 0.5, fill, (MENUBAR_OPACITY-0.04));
			murrine_pattern_add_color_stop_rgba (pat, 1.0, fill, MENUBAR_OPACITY);
			cairo_set_source (cr, pat);
			cairo_rectangle  (cr, 0, 0, width, height);
			cairo_fill       (cr);
			cairo_pattern_destroy (pat);

			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
			break;
		}
		case 1:
		{
			/* XXX: should use another gradient rgba_opacity */
			int os = (widget->glazestyle == 2) ? 1 : 0;
			murrine_draw_glaze (cr, fill,
			                    widget->glow_shade, widget->highlight_shade, widget->lightborder_shade,
			                    widget->mrn_gradient, widget, os, os, width-os*2, height-os*2,
			                    widget->roundness, widget->corners, TRUE);
			break;
		}
		case 2:
		{
			cairo_pattern_t *pat;
			MurrineRGB lower;
			murrine_shade (fill, 0.95, &lower);

			pat = cairo_pattern_create_linear (0, 0, 0, height);
			murrine_pattern_add_color_stop_rgba (pat, 0.0, fill, MENUBAR_OPACITY);
			murrine_pattern_add_color_stop_rgba (pat, 1.0, &lower, MENUBAR_OPACITY);
			cairo_set_source (cr, pat);
			cairo_fill (cr);
			cairo_pattern_destroy (pat);

			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
			break;
		}
		case 3:
		{
			cairo_pattern_t *pat;
			MurrineRGB low, top;
			murrine_shade (fill, 0.9, &top);
			murrine_shade (fill, 1.1, &low);
			pat = cairo_pattern_create_linear (0, 0, 0, height);
			murrine_pattern_add_color_stop_rgba (pat, 0.0, &top, MENUBAR_STRIPED_OPACITY);
			murrine_pattern_add_color_stop_rgba (pat, 1.0, &low, MENUBAR_STRIPED_OPACITY);
			cairo_set_source (cr, pat);
			cairo_fill (cr);

			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

			cairo_pattern_destroy (pat);
			int counter = -height;
			murrine_shade (&low, 0.9, &low);
			murrine_set_color_rgba (cr, &low, MENUBAR_STRIPED_OPACITY);
			while (counter < width)
			{
				cairo_move_to (cr, counter, height);
				cairo_line_to (cr, counter+height, 0);
				cairo_stroke  (cr);
				counter += 5;
			}
			break;
		}
	}

	/* Draw bottom line */
	if (menubarstyle == 1 && widget->glazestyle == 2)
		cairo_rectangle (cr, 0.5, 0.5, width-1, height-1);
	else
	{
		cairo_move_to        (cr, 0, height-0.5);
		cairo_line_to        (cr, width, height-0.5);
	}

	murrine_set_color_rgb (cr, &dark);
	cairo_stroke          (cr);
}

static void
murrine_rgba_draw_toolbar (cairo_t *cr,
                           const MurrineColors    *colors,
                           const WidgetParameters *widget,
                           const ToolbarParameters *toolbar,
                           int x, int y, int width, int height)
{
	const MurrineRGB *fill = &colors->bg[0];
	const MurrineRGB *top  = &colors->shade[0];
	MurrineRGB dark = colors->shade[4];

	if(widget->mrn_gradient.has_border_colors)
		dark = widget->mrn_gradient.border_colors[1];

	cairo_translate (cr, x, y);
	cairo_rectangle (cr, 0, 0, width, height);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	/* Glass toolbar */
	switch (toolbar->style)
	{
		default:
		case 0:
			murrine_set_color_rgba (cr, fill, TOOLBAR_OPACITY);
			cairo_fill (cr);

			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

			/* Draw highlight */
			if (!toolbar->topmost)
			{
				murrine_set_color_rgba (cr, top, 0.5);
				cairo_move_to          (cr, 0, 0.5);
				cairo_line_to          (cr, width, 0.5);
				cairo_stroke           (cr);
			}
			break;
		case 1:
		{
			int os = (widget->glazestyle == 2) ? 1 : 0;
			murrine_draw_glaze (cr, fill,
			                    widget->glow_shade, widget->highlight_shade, widget->lightborder_shade,
			                    widget->mrn_gradient, widget, os, os, width-os*2, height-os*2,
			                    widget->roundness, widget->corners, TRUE);

			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
			break;
		}
		case 2:
		{
			cairo_pattern_t *pat;
			MurrineRGB lower;
			murrine_shade (fill, 0.95, &lower);
			pat = cairo_pattern_create_linear (0, 0, 0, height);
			murrine_pattern_add_color_stop_rgba (pat, 0.0, fill, TOOLBAR_OPACITY);
			murrine_pattern_add_color_stop_rgba (pat, 1.0, &lower, TOOLBAR_OPACITY);
			cairo_set_source (cr, pat);
			cairo_pattern_destroy (pat);
			cairo_fill (cr);

			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

			/* Draw highlight */
			if (!toolbar->topmost)
			{
				cairo_move_to         (cr, 0, 0.5);
				cairo_line_to         (cr, width, 0.5);
				murrine_set_color_rgb (cr, top);
				cairo_stroke          (cr);
			}
			break;
		}
	}

	/* Draw shadow */
	murrine_set_color_rgb (cr, &dark);
	/* Draw bottom line */
	if (toolbar->style == 1 && widget->glazestyle == 2)
		cairo_rectangle (cr, 0.5, 0.5, width-1, height-1);
	else
	{
		cairo_move_to        (cr, 0, height-0.5);
		cairo_line_to        (cr, width, height-0.5);
	}
	cairo_stroke          (cr);
}

static void
murrine_get_frame_gap_clip (int x, int y, int width, int height,
                            const FrameParameters *frame,
                            MurrineRectangle      *bevel,
                            MurrineRectangle      *border)
{
	switch (frame->gap_side)
	{
		case MRN_GAP_TOP:
			MURRINE_RECTANGLE_SET ((*bevel),  1.5+frame->gap_x, -0.5,
			                       frame->gap_width-3, 2.0);
			MURRINE_RECTANGLE_SET ((*border), 0.5+frame->gap_x, -0.5,
			                       frame->gap_width-2, 2.0);
			break;
		case MRN_GAP_BOTTOM:
			MURRINE_RECTANGLE_SET ((*bevel),  1.5+frame->gap_x, height-2.5,
			                       frame->gap_width-3, 2.0);
			MURRINE_RECTANGLE_SET ((*border), 0.5+frame->gap_x, height-1.5,
			                       frame->gap_width-2, 2.0);
			break;
		case MRN_GAP_LEFT:
			MURRINE_RECTANGLE_SET ((*bevel),  -0.5, 1.5+frame->gap_x,
			                       2.0, frame->gap_width-3);
			MURRINE_RECTANGLE_SET ((*border), -0.5, 0.5+frame->gap_x,
			                       1.0, frame->gap_width-2);
			break;
		case MRN_GAP_RIGHT:
			MURRINE_RECTANGLE_SET ((*bevel),  width-2.5, 1.5+frame->gap_x,
			                       2.0, frame->gap_width-3);
			MURRINE_RECTANGLE_SET ((*border), width-1.5, 0.5+frame->gap_x,
			                       1.0, frame->gap_width-2);
			break;
	}
}

static void
murrine_rgba_draw_frame (cairo_t *cr,
                         const MurrineColors    *colors,
                         const WidgetParameters *widget,
                         const FrameParameters  *frame,
                         int x, int y, int width, int height)
{
	MurrineRGB *border = frame->border;
	MurrineRectangle bevel_clip;
	MurrineRectangle frame_clip;
	const MurrineRGB *dark = &colors->shade[5];
	MurrineRGB highlight, shadow_color;

	murrine_shade (&colors->bg[0], 1.15, &highlight);
	murrine_shade (&colors->bg[0], 0.4, &shadow_color);

	if (frame->shadow == MRN_SHADOW_NONE)
		return;

	if (frame->gap_x != -1)
		murrine_get_frame_gap_clip (x, y, width, height,
		                            frame, &bevel_clip, &frame_clip);

	cairo_translate      (cr, x+0.5, y+0.5);

	/* save everything */
	cairo_save (cr);

	/* Set clip for the bevel */
	if (frame->gap_x != -1)
	{
		/* Set clip for gap */
		cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
		cairo_rectangle     (cr, -0.5, -0.5, width, height);
		cairo_rectangle     (cr, bevel_clip.x, bevel_clip.y, bevel_clip.width, bevel_clip.height);
		cairo_clip          (cr);
	}

	/* Draw the bevel */
	if (frame->shadow == MRN_SHADOW_ETCHED_IN || frame->shadow == MRN_SHADOW_ETCHED_OUT)
	{
		if (frame->shadow == MRN_SHADOW_ETCHED_IN)
			murrine_rounded_rectangle (cr, 1, 1, width-2, height-2, widget->roundness, widget->corners);
		else
			murrine_rounded_rectangle (cr, 0, 0, width-2, height-2, widget->roundness, widget->corners);
		murrine_set_color_rgba (cr, &highlight, 0.5);
		cairo_stroke (cr);
	}
	else if (frame->shadow != MRN_SHADOW_FLAT)
	{
		ShadowParameters shadow;
		shadow.corners = widget->corners;
		shadow.shadow  = frame->shadow;
		murrine_draw_highlight_and_shade (cr, colors, &shadow, width, height, widget->roundness-1);
	}

	/* restore the previous clip region */
	cairo_restore (cr);
	cairo_save    (cr);
	if (frame->gap_x != -1)
	{
		/* Set clip for gap */
		cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
		cairo_rectangle     (cr, -0.5, -0.5, width, height);
		cairo_rectangle     (cr, frame_clip.x, frame_clip.y, frame_clip.width, frame_clip.height);
		cairo_clip          (cr);
	}

	/* Draw frame */
	if (frame->shadow == MRN_SHADOW_ETCHED_IN || frame->shadow == MRN_SHADOW_ETCHED_OUT)
	{
		murrine_set_color_rgb (cr, dark);
		if (frame->shadow == MRN_SHADOW_ETCHED_IN)
			murrine_rounded_rectangle (cr, 0, 0, width-2, height-2, widget->roundness, widget->corners);
		else
			murrine_rounded_rectangle (cr, 1, 1, width-2, height-2, widget->roundness, widget->corners);
	}
	else
	{
		murrine_set_color_rgb (cr, border);
		murrine_rounded_rectangle (cr, 0, 0, width-1, height-1, widget->roundness, widget->corners);
	}
	cairo_stroke  (cr);
	cairo_restore (cr);
}

static void
murrine_rgba_draw_tab (cairo_t *cr,
                       const MurrineColors    *colors,
                       const WidgetParameters *widget,
                       const TabParameters    *tab,
                       int x, int y, int width, int height)
{
	const MurrineRGB *stripe_fill = &colors->spot[1];
	const MurrineRGB *stripe_border = &colors->spot[2];
	const MurrineRGB *fill = &colors->bg[widget->state_type];
	const MurrineRGB *border = &colors->shade[!widget->active ? 5 : 4];
	cairo_pattern_t  *pat;

	/* Set clip */
	cairo_rectangle (cr, x, y, width, height);
	cairo_clip      (cr);
	cairo_new_path  (cr);

	cairo_translate      (cr, x+0.5, y+0.5);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	/* Make the tabs slightly bigger than they should be, to create a gap */
	/* And calculate the strip size too, while you're at it */
	if (tab->gap_side == MRN_GAP_TOP || tab->gap_side == MRN_GAP_BOTTOM)
	{
		height += 3.0;

		if (tab->gap_side == MRN_GAP_TOP)
			cairo_translate (cr, 0.0, -3.0); /* gap at the other side */
	}
	else
	{
		width += 3.0;

		if (tab->gap_side == MRN_GAP_LEFT)
			cairo_translate (cr, -3.0, 0.0); /* gap at the other side */
	}

	/* Set tab shape */
	murrine_rounded_rectangle_closed (cr, 0, 0, width-1, height-1, widget->roundness, widget->corners);

	/* Draw fill */
	if (!widget->active)
	{
		murrine_set_color_rgba (cr, fill, NOTEBOOK_OPACITY);
		cairo_fill (cr);
	}

	if (widget->active)
	{
		MurrineRGB shade1, shade2, shade3, shade4, highlight;
		MurrineGradients mrn_gradient_new = mrn_gradient_new = murrine_get_decreased_gradient_shades (widget->mrn_gradient, 3.0);
		double highlight_shade_new = murrine_get_decreased_shade (widget->highlight_shade, 2.0);
		double lightborder_shade_new = murrine_get_decreased_shade (widget->lightborder_shade, 2.0);

		murrine_shade (fill, mrn_gradient_new.gradient_shades[0]*highlight_shade_new, &shade1);
		murrine_shade (fill, mrn_gradient_new.gradient_shades[1]*highlight_shade_new, &shade2);
		murrine_shade (fill, mrn_gradient_new.gradient_shades[2], &shade3);
		murrine_shade (fill, mrn_gradient_new.gradient_shades[3], &shade4);

		switch (tab->gap_side)
		{
			case MRN_GAP_TOP:
				pat = cairo_pattern_create_linear (0, height-2, 0, 0);
				break;
			case MRN_GAP_BOTTOM:
				pat = cairo_pattern_create_linear (0, 1, 0, height);
				break;
			case MRN_GAP_LEFT:
				pat = cairo_pattern_create_linear (width-2, 0, 1, 0);
				break;
			case MRN_GAP_RIGHT:
				pat = cairo_pattern_create_linear (1, 0, width-2, 0);
				break;
		}

		murrine_rounded_rectangle_closed (cr, 0, 0, width-1, height-1, widget->roundness, widget->corners);

		murrine_pattern_add_color_stop_rgba (pat, 0.00, &shade1, NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 0.45, &shade2, NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 0.45, &shade3, NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 1.00, &shade4, NOTEBOOK_OPACITY);
		cairo_set_source (cr, pat);
		cairo_fill (cr);
		cairo_pattern_destroy (pat);

		/* Draw lightborder */
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		murrine_shade (fill, lightborder_shade_new*highlight_shade_new, &highlight);
		murrine_shade (&highlight, mrn_gradient_new.gradient_shades[0]*highlight_shade_new, &shade1);
		murrine_shade (&highlight, mrn_gradient_new.gradient_shades[1]*highlight_shade_new, &shade2);
		murrine_shade (&highlight, mrn_gradient_new.gradient_shades[2], &shade3);
		murrine_shade (&highlight, mrn_gradient_new.gradient_shades[3], &shade4);

		switch (tab->gap_side)
		{
			case MRN_GAP_TOP:
				pat = cairo_pattern_create_linear (0, height-2, 0, 0);
				break;
			case MRN_GAP_BOTTOM:
				pat = cairo_pattern_create_linear (0, 1, 0, height);
				break;
			case MRN_GAP_LEFT:
				pat = cairo_pattern_create_linear (width-2, 0, 1, 0);
				break;
			case MRN_GAP_RIGHT:
				pat = cairo_pattern_create_linear (1, 0, width-2, 0);
				break;
		}

		murrine_rounded_rectangle_closed (cr, 1, 1, width-3, height-3, widget->roundness, widget->corners);

		murrine_pattern_add_color_stop_rgba (pat, 0.00, &shade1, 0.5*NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 0.45, &shade2, 0.5*NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 0.45, &shade3, 0.5*NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 1.00, &shade4, 0.5*NOTEBOOK_OPACITY);
		cairo_set_source (cr, pat);
		cairo_stroke (cr);
		cairo_pattern_destroy (pat);
	}
	else
	{
		MurrineRGB shade1, shade2, shade3, shade4, highlight;
		MurrineGradients mrn_gradient_new = mrn_gradient_new = murrine_get_decreased_gradient_shades (widget->mrn_gradient, 3.0);
		double highlight_shade_new = murrine_get_decreased_shade (widget->highlight_shade, 2.0);

		murrine_shade (fill, mrn_gradient_new.gradient_shades[0]*highlight_shade_new, &shade1);
		murrine_shade (fill, mrn_gradient_new.gradient_shades[1]*highlight_shade_new, &shade2);
		murrine_shade (fill, mrn_gradient_new.gradient_shades[2], &shade3);
		murrine_shade (fill, 1.0, &shade4); /* this value should change as draw_frame */

		/* Draw shade */
		switch (tab->gap_side)
		{
			case MRN_GAP_TOP:
				pat = cairo_pattern_create_linear (0, height-2, 0, 0);
				break;
			case MRN_GAP_BOTTOM:
				pat = cairo_pattern_create_linear (0, 0, 0, height);
				break;
			case MRN_GAP_LEFT:
				pat = cairo_pattern_create_linear (width-2, 0, 0, 0);
				break;
			case MRN_GAP_RIGHT:
				pat = cairo_pattern_create_linear (0, 0, width, 0);
				break;
		}

		murrine_rounded_rectangle_closed (cr, 0, 0, width-1, height-1, widget->roundness, widget->corners);

		murrine_pattern_add_color_stop_rgba (pat, 0.00, &shade1, NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 0.45, &shade2, NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 0.45, &shade3, NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 1.00, &shade4, NOTEBOOK_OPACITY);
		cairo_set_source (cr, pat);
		cairo_fill (cr);
		cairo_pattern_destroy (pat);

		/* Draw lightborder */
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		murrine_shade (fill, widget->lightborder_shade*highlight_shade_new, &highlight);
		murrine_shade (&highlight, mrn_gradient_new.gradient_shades[0]*highlight_shade_new, &shade1);
		murrine_shade (&highlight, mrn_gradient_new.gradient_shades[1]*highlight_shade_new, &shade2);
		murrine_shade (&highlight, mrn_gradient_new.gradient_shades[2], &shade3);
		murrine_shade (fill, 1.15, &shade4); /* this value should change as draw_frame */

		switch (tab->gap_side)
		{
			case MRN_GAP_TOP:
				pat = cairo_pattern_create_linear (0, height-2, 0, 0);
				break;
			case MRN_GAP_BOTTOM:
				pat = cairo_pattern_create_linear (0, 0, 0, height);
				break;
			case MRN_GAP_LEFT:
				pat = cairo_pattern_create_linear (width-2, 0, 0, 0);
				break;
			case MRN_GAP_RIGHT:
				pat = cairo_pattern_create_linear (0, 0, width, 0);
				break;
		}

		murrine_rounded_rectangle_closed (cr, 1, 1, width-3, height-3, widget->roundness, widget->corners);

		murrine_pattern_add_color_stop_rgba (pat, 0.00, &shade1, 0.5*NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 0.45, &shade2, 0.5*NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 0.45, &shade3, 0.5*NOTEBOOK_OPACITY);
		murrine_pattern_add_color_stop_rgba (pat, 1.00, &shade4, 0.5*NOTEBOOK_OPACITY);
		cairo_set_source (cr, pat);
		cairo_stroke (cr);
		cairo_pattern_destroy (pat);
	}

	murrine_rounded_rectangle (cr, 0, 0, width-1, height-1, widget->roundness, widget->corners);
	murrine_set_color_rgb  (cr, border);
	cairo_stroke (cr);
}

static void
murrine_rgba_draw_scrollbar_trough (cairo_t *cr,
                                    const MurrineColors       *colors,
                                    const WidgetParameters    *widget,
                                    const ScrollBarParameters *scrollbar,
                                    int x, int y, int width, int height)
{
	MurrineRGB border;
	MurrineRGB fill;

	murrine_shade (&widget->parentbg,
	               murrine_get_contrast (scrollbar->stepperstyle != 1 ? 0.82 : 0.75, widget->contrast),
	               &border);
	murrine_shade (&widget->parentbg, scrollbar->stepperstyle != 1 ? 0.95 : 1.065, &fill);

	if (!scrollbar->horizontal)
	{
		cairo_translate (cr, x, y);
	}
	else
	{
		int tmp = height;
		rotate_mirror_translate (cr, M_PI/2, x, y, FALSE, FALSE);
		height = width;
		width = tmp;
	}

	/* Draw fill */
	murrine_draw_trough (cr, &fill, 0, 0, width, height, widget->roundness, widget->corners, widget->mrn_gradient, 0.4, FALSE);

	if (scrollbar->stepperstyle == 3)
	{
		uint8 corners;
		MurrineRGB fill_stepper;
		MurrineRGB border_stepper;

		murrine_shade (&widget->parentbg, 1.02, &fill_stepper);
		murrine_shade (&border, (widget->mrn_gradient.trough_shades[0]+widget->mrn_gradient.trough_shades[1])/2.0, &border_stepper);

		cairo_save (cr);

		murrine_rounded_rectangle_closed (cr, 0.5, 0.5, width-1, height-1, widget->roundness, widget->corners);
		cairo_clip (cr);

		corners = MRN_CORNER_BOTTOMLEFT | MRN_CORNER_BOTTOMRIGHT;
		murrine_rounded_rectangle_inverted (cr, 0.5, 0.5, width-1, scrollbar->steppersize, widget->roundness, corners);
		murrine_set_color_rgb (cr, &fill_stepper);
		cairo_fill_preserve (cr);
		murrine_draw_trough_border_from_path (cr, &border,0.5, 0.5, width-1, scrollbar->steppersize, widget->mrn_gradient, 1.0, FALSE);

		corners = MRN_CORNER_TOPLEFT | MRN_CORNER_TOPRIGHT;
		murrine_rounded_rectangle_inverted (cr, 0.5, height-scrollbar->steppersize-0.5, width-1, scrollbar->steppersize, widget->roundness, corners);
		murrine_set_color_rgb (cr, &fill_stepper);
		cairo_fill_preserve (cr);
		murrine_draw_trough_border_from_path (cr, &border, 0.5, height-scrollbar->steppersize-0.5, width-1, scrollbar->steppersize, widget->mrn_gradient, 1.0, FALSE);

		cairo_restore (cr);
	}

	/* Draw border */
	if (!scrollbar->within_bevel)
		murrine_draw_trough_border (cr, &border, 0.5, 0.5, width-1, height-1, widget->roundness, widget->corners, widget->mrn_gradient, 0.82, FALSE);
	else
	{
		murrine_shade (&border, widget->mrn_gradient.trough_shades[0], &border);
		murrine_set_color_rgba (cr, &border, 0.82);
		cairo_move_to (cr, 0.5, 0);
		cairo_line_to (cr, 0.5, height);
		cairo_stroke (cr);
	}
}

void
murrine_rgba_draw_scrollbar_stepper (cairo_t *cr,
                                     const MurrineColors       *colors,
                                     const WidgetParameters    *widget,
                                     const ScrollBarParameters *scrollbar,
                                     int x, int y, int width, int height)
{
	MurrineGradients mrn_gradient_new = widget->mrn_gradient;
	double border_stop_mid = ((mrn_gradient_new.border_shades[0])+
	                          (mrn_gradient_new.border_shades[1]))/2.0;
	MurrineRGB border;
	MurrineRGB fill  = colors->bg[widget->state_type];

	murrine_get_fill_color (&fill, &mrn_gradient_new);
	murrine_shade (&colors->shade[7], 0.95, &border);

	mrn_gradient_new.border_shades[0] = border_stop_mid;
	mrn_gradient_new.border_shades[1] = border_stop_mid;

	if (!scrollbar->horizontal)
		murrine_exchange_axis (cr, &x, &y, &width, &height);

	/* Border color */
	murrine_mix_color (&border, &fill, 0.45, &border);

	cairo_translate (cr, x, y);

	cairo_save (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	murrine_rounded_rectangle_closed (cr, 1, 1, width-2, height-2, widget->roundness-1, widget->corners);
	cairo_clip_preserve(cr);

	murrine_draw_glaze (cr, &fill,
	                    widget->glow_shade, widget->highlight_shade, widget->lightborder_shade,
	                    widget->mrn_gradient, widget, 1, 1, width-2, height-2,
	                    widget->roundness, widget->corners, TRUE);

	cairo_restore (cr);

	murrine_draw_border (cr, &border,
	                     0.5, 0.5, width-1, height-1,
	                     widget->roundness, widget->corners,
	                     mrn_gradient_new, 1.0);
}

void
murrine_rgba_draw_scrollbar_slider (cairo_t *cr,
                                    const MurrineColors       *colors,
                                    const WidgetParameters    *widget,
                                    const ScrollBarParameters *scrollbar,
                                    int x, int y, int width, int height)
{
	MurrineGradients mrn_gradient_new = widget->mrn_gradient;
	double border_stop_mid = ((mrn_gradient_new.border_shades[0])+
	                          (mrn_gradient_new.border_shades[1]))/2.0;
	MurrineRGB fill = scrollbar->has_color ? scrollbar->color : colors->bg[widget->state_type];
	MurrineRGB border;
	uint8 corners = widget->corners;

	murrine_get_fill_color (&fill, &mrn_gradient_new);

	if (scrollbar->stepperstyle != 1 && scrollbar->stepperstyle != 3)
	{
		if (scrollbar->junction & MRN_JUNCTION_BEGIN)
		{
			if (scrollbar->horizontal)
			{
				x -= 1;
				width += 1;
			}
			else
			{
				y -= 1;
				height += 1;
			}
		}
		if (scrollbar->junction & MRN_JUNCTION_END)
		{
			if (scrollbar->horizontal)
				width += 1;
			else
				height += 1;
		}
	}

	if (scrollbar->stepperstyle == 2)
	{
		if (scrollbar->junction & MRN_JUNCTION_BEGIN)
		{
			if (scrollbar->horizontal)
				corners ^= MRN_CORNER_TOPLEFT | MRN_CORNER_BOTTOMLEFT;
			else
				corners ^= MRN_CORNER_TOPLEFT | MRN_CORNER_BOTTOMLEFT;
		}
		if (scrollbar->junction & MRN_JUNCTION_END)
		{
			if (scrollbar->horizontal)
				corners ^= MRN_CORNER_TOPRIGHT | MRN_CORNER_BOTTOMRIGHT;
			else
				corners ^= MRN_CORNER_TOPRIGHT | MRN_CORNER_BOTTOMRIGHT;
		}
	}

	murrine_shade (&colors->shade[7], 0.95, &border);

	mrn_gradient_new.border_shades[0] = border_stop_mid;
	mrn_gradient_new.border_shades[1] = border_stop_mid;

	if (widget->prelight && scrollbar->has_color)
		murrine_shade (&fill, scrollbar->prelight_shade, &fill);

	murrine_mix_color (&border, &fill, 0.5, &border);

	if (scrollbar->horizontal)
		cairo_translate (cr, x, y);
	else
	{
		int tmp = height;
		rotate_mirror_translate (cr, M_PI/2, x, y, FALSE, FALSE);
		height = width;
		width = tmp;
	}

	cairo_save (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	murrine_rounded_rectangle_closed (cr, 1, 1, width-2, height-2, widget->roundness-1, corners);
	cairo_clip_preserve (cr);

	murrine_draw_glaze (cr, &fill,
	                    widget->glow_shade, widget->highlight_shade, widget->lightborder_shade,
	                    widget->mrn_gradient, widget, 1, 1, width-2, height-2,
	                    widget->roundness, corners, TRUE);

	/* Draw the options */
	MurrineRGB style;
	if (scrollbar->style > 0)
		murrine_shade (&fill, 0.55, &style);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	switch (scrollbar->style)
	{
		case 1:
		{
			int circ_radius = 2;
			int circ_space = 5;
			int i;
			int x1 = circ_space+circ_radius;
			int y1 = height/2;
			for (i = circ_space; i < width-circ_space; i += 2*circ_radius+circ_space)
			{
				cairo_move_to (cr, i, 1);
				cairo_arc (cr, x1, y1, circ_radius, 0, M_PI*2);

				x1 += 2*circ_radius+circ_space;

				cairo_close_path (cr);
				murrine_set_color_rgba (cr, &style, 0.15);
				cairo_fill (cr);
			}
			break;
		}
		case 3:
		case 4:
		{
			int counter = -width;
			cairo_save (cr);
			cairo_rectangle (cr, 1, 1, width-2, height-2);
			cairo_clip (cr);
			cairo_new_path (cr);
			cairo_set_line_width (cr, 5.0); /* stroke width */
			murrine_set_color_rgba (cr, &style, 0.08);
			while (counter < height)
			{
				cairo_move_to (cr, width, counter);
				cairo_line_to (cr, 0, counter+width);
				cairo_stroke  (cr);
				counter += 12;
			}
			cairo_restore (cr);
			break;
		}
		case 5:
		case 6:
		{
			int stroke_width = 7;
			int stroke_space = 5;
			int i;
			murrine_set_color_rgba (cr, &style, 0.08);
			for (i = stroke_space; i < width-stroke_space; i += stroke_width+stroke_space)
			{
				cairo_move_to (cr, i, 1);
				cairo_rel_line_to (cr, 0, height-2);
				cairo_rel_line_to (cr, stroke_width, 0);
				cairo_rel_line_to (cr, 0, -(height-2));
				cairo_fill (cr);
			}
			break;
		}
	}
	/* Draw the handle */
	if (scrollbar->style > 0 && scrollbar->style % 2 == 0)
	{
		double bar_x = width/2-3.5;
		int i;

		switch (scrollbar->handlestyle)
		{
			default:
			case 0:
			{
				for (i=0; i<3; i++)
				{
					cairo_move_to (cr, bar_x, 5);
					cairo_line_to (cr, bar_x, height-5);
					murrine_set_color_rgb (cr, &border);
					cairo_stroke (cr);

					bar_x += 3;
				}
				break;
			}
			case 1:
			{
				MurrineRGB inset;
				murrine_shade (&fill, 1.08, &inset);

				for (i=0; i<3; i++)
				{
					cairo_move_to (cr, bar_x, 5);
					cairo_line_to (cr, bar_x, height-5);
					murrine_set_color_rgb (cr, &border);
					cairo_stroke (cr);

					cairo_move_to (cr, bar_x+1, 5);
					cairo_line_to (cr, bar_x+1, height-5);
					murrine_set_color_rgb (cr, &inset);
					cairo_stroke (cr);

					bar_x += 3;
				}
				break;
			}
			case 2:
			{
				MurrineRGB inset;
				murrine_shade (&fill, 1.04, &inset);

				bar_x++;

				for (i=0; i<3; i++)
				{
					cairo_move_to (cr, bar_x, 5);
					cairo_line_to (cr, bar_x, height-5);
					murrine_set_color_rgb (cr, &border);
					cairo_stroke (cr);

					cairo_move_to (cr, bar_x+1, 5);
					cairo_line_to (cr, bar_x+1, height-5);
					murrine_set_color_rgb (cr, &inset);
					cairo_stroke (cr);

					bar_x += 2;
				}
				break;
			}
		}
	}

	cairo_restore (cr);

	murrine_draw_border (cr, &border,
	                     0.5, 0.5, width-1, height-1,
	                     widget->roundness, corners,
	                     mrn_gradient_new, 1.0);
}

static void
murrine_rgba_draw_tooltip (cairo_t *cr,
                           const MurrineColors    *colors,
                           const WidgetParameters *widget,
                           int x, int y, int width, int height)
{
	MurrineRGB border;
	MurrineRGB fill = colors->bg[widget->state_type];
	MurrineGradients mrn_gradient_new = murrine_get_decreased_gradient_shades (widget->mrn_gradient, 2.0);
	double glow_shade_new = murrine_get_decreased_shade (widget->glow_shade, 2.0);
	double highlight_shade_new = murrine_get_decreased_shade (widget->highlight_shade, 2.0);

	murrine_shade (&fill, murrine_get_contrast(0.6, widget->contrast), &border);
	murrine_get_fill_color (&fill, &mrn_gradient_new);

	cairo_save (cr);

	cairo_translate (cr, x, y);

	cairo_rectangle (cr, 0, 0, width, height);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_fill (cr);

	cairo_save (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	murrine_rounded_rectangle_closed (cr, 1, 1, width-2, height-2, widget->roundness-1, widget->corners);
	cairo_clip_preserve (cr);

	murrine_draw_glaze (cr, &colors->bg[widget->state_type],
	                    glow_shade_new, highlight_shade_new, widget->lightborder_shade,
	                    mrn_gradient_new, widget, 1, 1, width-2, height-2,
	                    widget->roundness-1, widget->corners, TRUE);

	cairo_restore (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	murrine_draw_border (cr, &border,
	                     0.5, 0.5, width-1, height-1,
	                     widget->roundness, widget->corners,
	                     mrn_gradient_new, 1.0);

	cairo_restore (cr);
}

static void
murrine_rgba_draw_handle (cairo_t *cr,
                          const MurrineColors    *colors,
                          const WidgetParameters *widget,
                          const HandleParameters *handle,
                          int x, int y, int width, int height)
{
	int bar_height;
	int bar_width  = 4;
	int i, bar_y = 1;
	int num_bars, bar_spacing;
	num_bars    = 3;
	bar_spacing = 3;
	bar_height = num_bars*bar_spacing;

	if (handle->horizontal)
	{
		int tmp = height;
		rotate_mirror_translate (cr, M_PI/2, x+0.5+width/2-bar_height/2, y+height/2-bar_width/2, FALSE, FALSE);
		height = width;
		width = tmp;
	}
	else
	{
		cairo_translate (cr, x+width/2-bar_width/2, y+height/2-bar_height/2+0.5);
	}

	switch (handle->style)
	{
		default:
		case 0:
		{
			for (i=0; i<num_bars; i++)
			{
				cairo_move_to (cr, 0, bar_y);
				cairo_line_to (cr, bar_width, bar_y);
				murrine_set_color_rgb (cr, &colors->shade[5]);
				cairo_stroke (cr);

				bar_y += bar_spacing;
			}
			break;
		}
		case 1:
		{
			for (i=0; i<num_bars; i++)
			{
				cairo_move_to (cr, 0, bar_y);
				cairo_line_to (cr, bar_width, bar_y);
				murrine_set_color_rgb (cr, &colors->shade[5]);
				cairo_stroke (cr);

				cairo_move_to (cr, 0, bar_y+1);
				cairo_line_to (cr, bar_width, bar_y+1);
				murrine_set_color_rgb (cr, &colors->shade[0]);
				cairo_stroke (cr);

				bar_y += bar_spacing;
			}
			break;
		}
		case 2:
		{
			bar_y++;

			for (i=0; i<num_bars; i++)
			{
				cairo_move_to (cr, 0, bar_y);
				cairo_line_to (cr, bar_width, bar_y);
				murrine_set_color_rgb (cr, &colors->shade[5]);
				cairo_stroke (cr);

				cairo_move_to (cr, 0, bar_y+1);
				cairo_line_to (cr, bar_width, bar_y+1);
				murrine_set_color_rgb (cr, &colors->shade[0]);
				cairo_stroke (cr);

				bar_y += 2;
			}
			break;
		}
	}
}


static void
murrine_rgba_draw_radiobutton (cairo_t *cr,
                               const MurrineColors      *colors,
                               const WidgetParameters   *widget,
                               const CheckboxParameters *checkbox,
                               int x, int y, int width, int height,
                               double trans)
{
	MurrineRGB border;
	const MurrineRGB *dot;
	const MurrineRGB *bg = &colors->base[0];
	gboolean inconsistent = FALSE;
	gboolean draw_box = !checkbox->in_menu;
	gboolean draw_bullet = (checkbox->shadow_type == GTK_SHADOW_IN);
	int roundness = width+height;
	double highlight_shade_new = widget->highlight_shade;
	double lightborder_shade_new = widget->lightborder_shade;
	MurrineGradients mrn_gradient_new = widget->mrn_gradient;

	inconsistent = (checkbox->shadow_type == GTK_SHADOW_ETCHED_IN);
	draw_bullet |= inconsistent;

	if (widget->state_type == GTK_STATE_INSENSITIVE)
	{
		border = colors->shade[4];
		dot    = &colors->shade[4];
		bg     = &colors->bg[0];

		mrn_gradient_new = murrine_get_decreased_gradient_shades (widget->mrn_gradient, 3.0);
		mrn_gradient_new.border_shades[0] = murrine_get_decreased_shade (widget->mrn_gradient.border_shades[0], 3.0);
		mrn_gradient_new.border_shades[1] = murrine_get_decreased_shade (widget->mrn_gradient.border_shades[1], 3.0);
		highlight_shade_new = murrine_get_decreased_shade (widget->highlight_shade, 2.0);
		lightborder_shade_new = murrine_get_decreased_shade (widget->lightborder_shade, 2.0);
	}
	else
	{
		border = colors->shade[6];
		if (draw_bullet)
		{
			border = colors->spot[2];
			bg     = &colors->spot[1];
		}
		dot    = &colors->text[widget->state_type];
	}
	murrine_mix_color (&border, widget->state_type != GTK_STATE_INSENSITIVE ?
	                   draw_bullet ? &colors->spot[1] : &colors->bg[0] : &colors->bg[0], 0.2, &border);

	cairo_translate (cr, x, y);

	if (draw_box)
	{
		if (widget->xthickness > 2 && widget->ythickness > 2)
		{
			if (widget->reliefstyle > 1 && draw_bullet && widget->state_type != GTK_STATE_INSENSITIVE)
			{
				if (widget->reliefstyle == 5)
					murrine_draw_shadow (cr, &widget->parentbg,
					                     0.5, 0.5, width-1, height-1,
					                     roundness+1, widget->corners,
					                     widget->reliefstyle,
					                     mrn_gradient_new, 0.5);
				else
				{
					MurrineRGB shadow;
					murrine_shade (&border, 0.9, &shadow);

					murrine_draw_shadow (cr, &shadow,
					                     0.5, 0.5, width-1, height-1,
					                     roundness+1, widget->corners,
					                     widget->reliefstyle,
					                     mrn_gradient_new, 0.08);
				}
			}
			else if (widget->reliefstyle != 0)
				murrine_draw_inset (cr, &widget->parentbg, 0.5, 0.5, width-1, height-1, roundness+1, widget->corners);
		}

		cairo_save (cr);

		murrine_rounded_rectangle_closed (cr, 1.5, 1.5, width-3, height-3, roundness, widget->corners);
		cairo_clip_preserve (cr);

		if (draw_bullet)
		{
			murrine_draw_glaze (cr, bg,
				            widget->glow_shade, highlight_shade_new, lightborder_shade_new,
				            mrn_gradient_new, widget, 2, 2, width-4, height-4,
				            roundness, widget->corners, TRUE);
		}
		else
		{
			murrine_set_color_rgb (cr, bg);
			cairo_fill (cr);
		}

		cairo_restore (cr);

		if (checkbox->in_cell)
		{
			mrn_gradient_new.border_shades[0] = 1.0;
			mrn_gradient_new.border_shades[1] = 1.0;
			if (!draw_bullet)
				mrn_gradient_new.has_border_colors = FALSE;
		}
		else if (!draw_bullet)
		{
			mrn_gradient_new = murrine_get_inverted_border_shades (mrn_gradient_new);
			mrn_gradient_new.has_border_colors = FALSE;
		}

		murrine_draw_border (cr, &border,
			             1.5, 1.5, width-3, height-3,
			             roundness, widget->corners,
			             mrn_gradient_new, 1.0);
	}

	if (draw_bullet)
	{
		if (inconsistent)
		{
			cairo_save (cr);
			cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
			cairo_set_line_width (cr, 2.0);

			cairo_move_to(cr, 5, (double)height/2);
			cairo_line_to(cr, width-5, (double)height/2);

			murrine_set_color_rgba (cr, dot, trans);
			cairo_stroke (cr);
			cairo_restore (cr);
		}
		else
		{
			if (!draw_box)
			{
				cairo_arc (cr, (double)width/2, (double)height/2, (double)(width+height)/4-4, 0, G_PI*2);
			}
			else
			{
				MurrineRGB outline;
				murrine_invert_text (dot, &outline);

				cairo_arc (cr, (double)width/2, (double)height/2, (double)(width+height)/4-4, 0, G_PI*2);
				murrine_set_color_rgba (cr, &outline, 0.3*trans*(widget->state_type == GTK_STATE_INSENSITIVE ? 0.2 : 1.0));
				cairo_fill (cr);

				cairo_arc (cr, (double)width/2, (double)height/2, (double)(width+height)/4-5, 0, G_PI*2);
			}

			murrine_set_color_rgba (cr, dot, trans);
			cairo_fill (cr);
		}
	}
}

static void
murrine_rgba_draw_checkbox (cairo_t *cr,
                            const MurrineColors      *colors,
                            const WidgetParameters   *widget,
                            const CheckboxParameters *checkbox,
                            int x, int y, int width, int height,
                            double trans)
{
	MurrineRGB border;
	const MurrineRGB *dot;
	const MurrineRGB *bg = &colors->base[0];
	gboolean inconsistent = FALSE;
	gboolean draw_box = !checkbox->in_menu;
	gboolean draw_bullet = (checkbox->shadow_type == GTK_SHADOW_IN);
	int roundness = CLAMP (widget->roundness, 0, 2);
	double highlight_shade_new = widget->highlight_shade;
	double lightborder_shade_new = widget->lightborder_shade;
	MurrineGradients mrn_gradient_new = widget->mrn_gradient;

	inconsistent = (checkbox->shadow_type == GTK_SHADOW_ETCHED_IN);
	draw_bullet |= inconsistent;

	if (widget->state_type == GTK_STATE_INSENSITIVE)
	{
		border = colors->shade[4];
		dot    = &colors->shade[4];
		bg     = &colors->bg[0];

		mrn_gradient_new = murrine_get_decreased_gradient_shades (widget->mrn_gradient, 3.0);
		mrn_gradient_new.border_shades[0] = murrine_get_decreased_shade (widget->mrn_gradient.border_shades[0], 3.0);
		mrn_gradient_new.border_shades[1] = murrine_get_decreased_shade (widget->mrn_gradient.border_shades[1], 3.0);
		highlight_shade_new = murrine_get_decreased_shade (widget->highlight_shade, 2.0);
		lightborder_shade_new = murrine_get_decreased_shade (widget->lightborder_shade, 2.0);
	}
	else
	{
		border = colors->shade[6];
		if (draw_bullet)
		{
			border = colors->spot[2];
			bg     = &colors->spot[1];
		}
		dot    = &colors->text[widget->state_type];
	}
	murrine_mix_color (&border, widget->state_type != GTK_STATE_INSENSITIVE ?
	                   draw_bullet ? &colors->spot[1] : &colors->bg[0] : &colors->bg[0], 0.2, &border);

	cairo_translate (cr, x, y);

	if (draw_box)
	{
		if (widget->xthickness > 2 && widget->ythickness > 2)
		{
			if (widget->reliefstyle > 1 && draw_bullet && widget->state_type != GTK_STATE_INSENSITIVE)
			{
				if (widget->reliefstyle == 5)
					murrine_draw_shadow (cr, &widget->parentbg,
					                     0.5, 0.5, width-1, height-1,
					                     roundness+1, widget->corners,
					                     widget->reliefstyle,
					                     mrn_gradient_new, 0.5);
				else
				{
					MurrineRGB shadow;
					murrine_shade (&border, 0.9, &shadow);

					murrine_draw_shadow (cr, &shadow,
					                     0.5, 0.5, width-1, height-1,
					                     roundness+1, widget->corners,
					                     widget->reliefstyle,
					                     mrn_gradient_new, 0.08);
				}
			}
			else if (widget->reliefstyle != 0)
				murrine_draw_inset (cr, &widget->parentbg, 0.5, 0.5, width-1, height-1, roundness+1, widget->corners);
		}

		cairo_save (cr);

		murrine_rounded_rectangle_closed (cr, 1.5, 1.5, width-3, height-3, roundness, widget->corners);
		cairo_clip_preserve (cr);

		if (draw_bullet)
		{
			murrine_draw_glaze (cr, bg,
				            widget->glow_shade, highlight_shade_new, lightborder_shade_new,
				            mrn_gradient_new, widget, 2, 2, width-4, height-4,
				            roundness, widget->corners, TRUE);
		}
		else
		{
			murrine_set_color_rgb (cr, bg);
			cairo_fill (cr);
		}

		cairo_restore (cr);

		if (checkbox->in_cell)
		{
			mrn_gradient_new.border_shades[0] = 1.0;
			mrn_gradient_new.border_shades[1] = 1.0;
			if (!draw_bullet)
				mrn_gradient_new.has_border_colors = FALSE;
		}
		else if (!draw_bullet)
		{
			mrn_gradient_new = murrine_get_inverted_border_shades (mrn_gradient_new);
			mrn_gradient_new.has_border_colors = FALSE;
		}

		murrine_draw_border (cr, &border,
			             1.5, 1.5, width-3, height-3,
			             roundness, widget->corners,
			             mrn_gradient_new, 1.0);
	}

	if (draw_bullet)
	{
		if (inconsistent)
		{
			cairo_save (cr);
			cairo_set_line_width (cr, 2.0);
			cairo_move_to (cr, 3, (double)height/2);
			cairo_line_to (cr, width-3, (double)height/2);
			cairo_restore (cr);
		}
		else
		{
			if (!draw_box)
			{
				cairo_scale (cr, (double)width/18.0, (double)height/18.0);
				cairo_translate (cr, 2.0, 3.0);
			}
			else
			{
				MurrineRGB outline;
				murrine_invert_text (dot, &outline);

				cairo_scale (cr, (double)width/18.0, (double)height/18.0);

				cairo_move_to (cr, 5.0, 5.65);
				cairo_line_to (cr, 8.95, 9.57);
				cairo_line_to (cr, 16.0, 2.54);
				cairo_line_to (cr, 16.0, 8.36);
				cairo_line_to (cr, 10.6, 15.1);
				cairo_line_to (cr, 7.6, 15.1);
				cairo_line_to (cr, 2.95, 10.48);
				cairo_line_to (cr, 2.95, 7.65);
				cairo_close_path (cr);

				murrine_set_color_rgba (cr, &outline, 0.5*trans*(widget->state_type == GTK_STATE_INSENSITIVE ? 0.2 : 1.0));
				cairo_fill (cr);

				cairo_translate (cr, 4.0, 2.0);
			}
			cairo_move_to (cr, 0.0, 6.0);
			cairo_line_to (cr, 0.0, 8.0);
			cairo_line_to (cr, 4.0, 12.0);
			cairo_line_to (cr, 6.0, 12.0);
			cairo_line_to (cr, 15.0, 1.0);
			cairo_line_to (cr, 15.0, 0.0);
			cairo_line_to (cr, 14.0, 0.0);
			cairo_line_to (cr, 5.0, 9.0);
			cairo_line_to (cr, 1.0, 5.0);
			cairo_close_path (cr);
		}
		murrine_set_color_rgba (cr, dot, trans);
		cairo_fill (cr);
	}
}

static void
murrine_rgba_draw_menu_frame (cairo_t *cr,
                              const MurrineColors    *colors,
                              const WidgetParameters *widget,
                              int x, int y, int width, int height,
                              int menustyle)
{
	uint8 corners = (menustyle == 1 ? MRN_CORNER_BOTTOMRIGHT :
	                                  MRN_CORNER_BOTTOMLEFT | MRN_CORNER_BOTTOMRIGHT);

	cairo_translate       (cr, x, y);

	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	murrine_set_color_rgba (cr, &colors->bg[0], MENU_OPACITY);
	clearlooks_rounded_rectangle (cr, 0, 0, width, height, widget->roundness > 1 ? widget->roundness+1 : 0, corners);
	cairo_fill (cr);

	switch (menustyle)
	{
		case 1:
		{
			MurrineRGB *fill = (MurrineRGB*)&colors->spot[1];
			MurrineRGB border2;
			murrine_shade (fill, 0.5, &border2);

			murrine_set_color_rgb (cr, &border2);
			cairo_rectangle (cr, 0.5, 0.5, 3, height-1);
			cairo_stroke_preserve (cr);

			murrine_set_color_rgb (cr, fill);
			cairo_fill (cr);
		}
		default:
		case 0:
		{
			const MurrineRGB *border = &colors->shade[5];

			murrine_set_color_rgb (cr, border);
			murrine_rounded_rectangle (cr, 0.5, 0.5, width-1, height-1, widget->roundness, corners);
			cairo_stroke          (cr);
			break;
		}
		case 2:
		{
			const MurrineRGB *border = &colors->shade[2];
			MurrineRGB fill;
			raico_blur_t* blur = NULL;
			cairo_t *cr_surface;
			cairo_surface_t *surface;
			cairo_pattern_t *pat;
			int bradius = 30;
			int bheight = MIN (height, 300);

			murrine_shade (&colors->bg[0], 1.14, &fill);

			murrine_set_color_rgb (cr, border);
			cairo_rectangle       (cr, 0.5, 0.5, width-1, height-1);
			cairo_stroke          (cr);

			/* draw glow */
			surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, bheight);
			cr_surface = cairo_create (surface);
			blur = raico_blur_create (RAICO_BLUR_QUALITY_LOW);
			raico_blur_set_radius (blur, bradius);
			cairo_set_line_width (cr_surface, 1.0);
			cairo_rectangle (cr_surface, bradius, bradius-15, width-bradius*2, bheight-bradius*2+15);
			murrine_set_color_rgb (cr_surface, &fill);
			cairo_fill (cr_surface);
			raico_blur_apply (blur, surface);
			cairo_rectangle (cr_surface, 0, -15, width, bheight+15);
			pat = cairo_pattern_create_linear (0, -15, 0.0, bheight+15);
			murrine_pattern_add_color_stop_rgba (pat, 0.25, &colors->bg[0], 0.0);
			murrine_pattern_add_color_stop_rgba (pat, 1.0, &colors->bg[0], 1.0);
			cairo_set_source (cr_surface, pat);
			cairo_pattern_destroy (pat);
			cairo_fill (cr_surface);
			cairo_set_source_surface (cr, surface, 0, 0);
			cairo_paint (cr);
			cairo_surface_destroy (surface);
			cairo_destroy (cr_surface);
			break;
		}
		case 3:
		{
			MurrineRGB border;
			MurrineRGB fill;
			raico_blur_t* blur = NULL;
			cairo_t *cr_surface;
			cairo_surface_t *surface;
			cairo_pattern_t *pat;
			int bradius = 30;
			int bheight = MIN (height, 300);

			murrine_shade (&colors->bg[0], murrine_get_contrast(1.1, widget->contrast), &border);
			murrine_shade (&colors->bg[0], 0.96, &fill);

			murrine_set_color_rgb (cr, &border);
			cairo_rectangle       (cr, 0.5, 0.5, width-1, height-1);
			cairo_stroke          (cr);

			/* draw glow */
			surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, bheight);
			cr_surface = cairo_create (surface);
			blur = raico_blur_create (RAICO_BLUR_QUALITY_LOW);
			raico_blur_set_radius (blur, bradius);
			cairo_set_line_width (cr_surface, 1.0);
			cairo_rectangle (cr_surface, bradius, bradius-15, width-bradius*2, bheight-bradius*2+15);
			murrine_set_color_rgb (cr_surface, &fill);
			cairo_fill (cr_surface);
			raico_blur_apply (blur, surface);
			cairo_rectangle (cr_surface, 0, -15, width, bheight+15);
			pat = cairo_pattern_create_linear (0, -15, 0.0, bheight+15);
			murrine_pattern_add_color_stop_rgba (pat, 0.25, &colors->bg[0], 0.0);
			murrine_pattern_add_color_stop_rgba (pat, 1.0, &colors->bg[0], 1.0);
			cairo_set_source (cr_surface, pat);
			cairo_pattern_destroy (pat);
			cairo_fill (cr_surface);
			cairo_set_source_surface (cr, surface, 0, 0);
			cairo_paint (cr);
			cairo_surface_destroy (surface);
			cairo_destroy (cr_surface);
			break;
		}
	}
}

static void
murrine_rgba_draw_statusbar (cairo_t *cr,
                             const MurrineColors    *colors,
                             const WidgetParameters *widget,
                             int x, int y, int width, int height)
{
	const MurrineRGB *dark = &colors->shade[4];
	const MurrineRGB *highlight = &colors->shade[0];

	cairo_translate       (cr, x, y+0.5);

	murrine_set_color_rgb (cr, dark);
	cairo_move_to         (cr, 0, 0);
	cairo_line_to         (cr, width, 0);
	cairo_stroke          (cr);

	murrine_set_color_rgba (cr, highlight, 0.5);
	cairo_move_to          (cr, 0, 1);
	cairo_line_to          (cr, width, 1);
	cairo_stroke           (cr);
}

void
murrine_register_style_rgba (MurrineStyleFunctions *functions)
{
	functions->draw_button             = murrine_rgba_draw_button;
	functions->draw_entry              = murrine_rgba_draw_entry;
	functions->draw_scale_trough       = murrine_rgba_draw_scale_trough;
	functions->draw_slider             = murrine_rgba_draw_slider;
	functions->draw_spinbutton         = murrine_rgba_draw_spinbutton;
	functions->draw_progressbar_trough = murrine_rgba_draw_progressbar_trough;
	functions->draw_progressbar_fill   = murrine_rgba_draw_progressbar_fill;
	functions->draw_menubar            = murrine_rgba_draw_menubar;
	functions->draw_toolbar            = murrine_rgba_draw_toolbar;
	functions->draw_frame              = murrine_rgba_draw_frame;
	functions->draw_tab                = murrine_rgba_draw_tab;
	functions->draw_scrollbar_trough   = murrine_rgba_draw_scrollbar_trough;
	functions->draw_scrollbar_stepper  = murrine_rgba_draw_scrollbar_stepper;
	functions->draw_scrollbar_slider   = murrine_rgba_draw_scrollbar_slider;
	functions->draw_handle             = murrine_rgba_draw_handle;
	functions->draw_tooltip            = murrine_rgba_draw_tooltip;
	functions->draw_radiobutton        = murrine_rgba_draw_radiobutton;
	functions->draw_checkbox           = murrine_rgba_draw_checkbox;
	functions->draw_menu_frame         = murrine_rgba_draw_menu_frame;
	functions->draw_statusbar          = murrine_rgba_draw_statusbar;
}
