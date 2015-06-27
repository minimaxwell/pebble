/* 
 *   Copyright (C) 2015 Maxime Chevallier
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software Foundation,
 *   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 *   Hexagon drawing utilities header file
 */

#include <pebble.h>

#ifndef HEXAGON_H
#define	HEXAGON_H

#define HALF_SQRT_3 ((float)0.86602540378)

typedef struct{
    Layer *layer;
    GPoint center;
    GPoint *points;
    GPoint *border_points;
    int16_t side_width;
    TextLayer *text;
    TextLayer *legend;
} t_hexagon;

/**
 * @brief Creates an hexagon on the heap.
 * @param center the coordinates of the center of the hexagon, relative to the 
 *        parent layer
 * @param side_width the width of each side of the hexagon
 * @param color the fill color
 * @param parent_layer the parent layer of the hexagon
 * @return The newly allocated hexagon
 */
t_hexagon *create_hexagon(GPoint center, 
                          int16_t side_width, 
                          GColor color, 
                          Layer *parent_layer);

t_hexagon *create_hexagon_with_border(GPoint center, 
                                      int16_t side_width,
                                      GColor color,
                                      int16_t border_side_width,
                                      GColor border_color,
                                      int16_t border_size,
                                      Layer *parent_layer);

/**
 * Free the memory used by the hexagon
 * @param hexagon
 */
void destroy_hexagon(t_hexagon *hexagon);

/**
 * @brief Sets the color of the hexagon
 * @param color
 */
void hexagon_set_color(t_hexagon *hexagon, GColor color);

/**
 * @brief returns the current color of the hexagon
 * @param hexagon
 * @return 
 */
GColor hexagon_get_color(t_hexagon *hexagon);

/**
 * @brief initializes the text layer of an hexagon
 * @param hexagon
 * @param frame
 * @param color
 * @param init_text
 * @param font
 */
void hexagon_init_text_layer(t_hexagon *hexagon,
        GRect frame, 
        GColor color, 
        const char *init_text, 
        GFont font);

void hexagon_set_text(t_hexagon *hexagon, const char *text);
void hexagon_set_legend(t_hexagon *hexagon, const char *legend_text);

void hexagon_show_legend(t_hexagon *hexagon);
void hexagon_hide_legend(t_hexagon *hexagon);

#endif	/* HEXAGON_H */

