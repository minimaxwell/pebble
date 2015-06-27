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
 *   Hexagon drawing utilities
 * 
 */

#include <hexagon.h>

#define HEX_HEIGHT(s) ((float)s * HALF_SQRT_3)

struct _hexagon_layer_data{
    GColor color;
    GPath *path;
    GColor border_color;
    GPath *border_path;
    int16_t border_width;
};


static struct _hexagon_layer_data *hexagon_get_layer_data(Layer *layer){
    struct _hexagon_layer_data *layer_data = layer_get_data( layer );
    return layer_data;
}

static void hexagon_update_proc( Layer *layer, GContext *context ){
    struct _hexagon_layer_data *layer_data = hexagon_get_layer_data(layer);
        
    graphics_context_set_fill_color(context, layer_data->color);
    gpath_draw_filled(context, layer_data->path);
    
    if( layer_data->border_path ){
        graphics_context_set_stroke_color(context, layer_data->border_color);
        graphics_context_set_stroke_width(context,(uint8_t)layer_data->border_width);
        gpath_draw_outline(context, layer_data->border_path);
    }
    
}

static GPoint *create_hexagonal_path(int16_t side_width){
    GPoint *points = malloc(6 * sizeof(GPoint));
    int16_t hexagon_half_height;
    
    hexagon_half_height = HEX_HEIGHT(side_width);
    /* Left */
    points[0].x = 0;
    points[0].y = hexagon_half_height;
    
    /* Up left*/
    points[1].x = (side_width / 2);
    points[1].y = 0;
    
    /* Up right*/
    points[2].x = side_width + (side_width / 2);
    points[2].y = 0;
    
    /* Right */
    points[3].x = side_width * 2;
    points[3].y = hexagon_half_height;
    
    /* Down right*/
    points[4].x = side_width + (side_width / 2);
    points[4].y = hexagon_half_height * 2;
    
     /* Down left*/
    points[5].x = (side_width / 2);
    points[5].y = hexagon_half_height * 2;
    
    return points;

}
t_hexagon *create_hexagon_with_border(GPoint center, 
                                      int16_t side_width,
                                      GColor color,
                                      int16_t border_side_width,
                                      GColor border_color,
                                      int16_t border_size,
                                      Layer *parent_layer){
    int i;
    int offset;
    t_hexagon * hexagon = create_hexagon(center, side_width, color, parent_layer);
    
    struct _hexagon_layer_data *layer_data= hexagon_get_layer_data( hexagon->layer );
    
    GPoint *points = create_hexagonal_path( border_side_width );
    
    GPathInfo border_path_info = {
        .num_points = 6,
        .points = points
    };
    
    //We offset the border to center it.
    offset = side_width - border_side_width;
    for( i=0; i < 6; ++i){
        points[i].x += offset;
        points[i].y += (offset-1);
    }
    
    hexagon->border_points = points;
    
    layer_data->border_path = gpath_create( &border_path_info );
    layer_data->border_color = border_color;
    layer_data->border_width = border_size;
    
    return hexagon;
}

t_hexagon *create_hexagon(GPoint center, 
                          int16_t side_width, 
                          GColor color, 
                          Layer *parent_layer){
    
    t_hexagon * hexagon;
    GPath *path;
    GPoint *points;  
    
    points = create_hexagonal_path(side_width);
    
    GPathInfo path_info = {
        .num_points = 6,
        .points = points
    };
    
    hexagon = malloc( sizeof( t_hexagon ) );
    memset( hexagon, 0, sizeof(t_hexagon) );
    
    hexagon->layer = layer_create_with_data(
                        GRect(center.x - side_width,
                              center.y - HEX_HEIGHT(side_width),
                              side_width * 2,
                              HEX_HEIGHT(side_width) * 2),
                        sizeof(struct _hexagon_layer_data));
    
    hexagon->center = center;
    hexagon->side_width = side_width;
    hexagon->points = points;
    
    path = gpath_create( &path_info );
    
    struct _hexagon_layer_data *layer_data = hexagon_get_layer_data( hexagon->layer );
    memset(layer_data,0,sizeof(struct _hexagon_layer_data));
    
    layer_data->path = path;
    layer_data->color = color;
    
    layer_set_update_proc( hexagon->layer, hexagon_update_proc );
    layer_add_child(parent_layer, hexagon->layer);
    
    return hexagon;
}


void destroy_hexagon(t_hexagon *hexagon){
    struct _hexagon_layer_data *layer_data = hexagon_get_layer_data( hexagon->layer );
    
    gpath_destroy( layer_data->path );
    
    if( layer_data->border_path )
        gpath_destroy( layer_data->border_path );
    
    if(hexagon->legend)
        text_layer_destroy(hexagon->legend);
    if( hexagon->text )
        text_layer_destroy(hexagon->text);
    
    layer_destroy( hexagon->layer );
    free(hexagon->points);
    free(hexagon->border_points);
    free( hexagon );
}

void hexagon_set_color(t_hexagon *hexagon, GColor color){
    struct _hexagon_layer_data *layer_data = hexagon_get_layer_data( hexagon->layer );
    layer_data->color = color;
    layer_mark_dirty(hexagon->layer);
}

GColor hexagon_get_color(t_hexagon *hexagon){
    struct _hexagon_layer_data *layer_data = hexagon_get_layer_data( hexagon->layer );
    return layer_data->color;
}

void hexagon_set_border_color(t_hexagon *hexagon, GColor color){
    struct _hexagon_layer_data *layer_data = hexagon_get_layer_data( hexagon->layer );
    layer_data->border_color = color;
}

GColor hexagon_get_border_color(t_hexagon *hexagon){
    struct _hexagon_layer_data *layer_data = hexagon_get_layer_data( hexagon->layer );
    return layer_data->border_color;
}

void hexagon_init_text_layer(t_hexagon *hexagon,
        GRect frame, 
        GColor color, 
        const char *init_text, 
        GFont font){
    
    TextLayer *layer;
    layer = text_layer_create(frame);
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_text_color(layer, color);
    text_layer_set_text(layer, init_text);

    text_layer_set_font(layer, font);
    text_layer_set_text_alignment(layer, GTextAlignmentCenter);

    layer_add_child(hexagon->layer, text_layer_get_layer(layer));
    hexagon->text = layer;
    
}

void hexagon_set_text(t_hexagon *hexagon, const char *text){
    if(hexagon->text)
        text_layer_set_text(hexagon->text, text);
}

void hexagon_set_legend(t_hexagon *hexagon, const char *legend_text){
    
    if( !hexagon->text )
        return;
    
    TextLayer *layer;
    layer = text_layer_create(
            GRect(
                0,
                HALF_SQRT_3 * 1.1 * hexagon->side_width, 
                2*hexagon->side_width, 
                HALF_SQRT_3 * 0.9 * hexagon->side_width));
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_text_color(layer, hexagon_get_border_color(hexagon)); 
    text_layer_set_text(layer, legend_text);

    text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(layer, GTextAlignmentCenter);
  //  layer_set_hidden(text_layer_get_layer(layer), true);
    layer_add_child(hexagon->layer, text_layer_get_layer(layer));
    hexagon->legend = layer;
}

void hexagon_show_legend(t_hexagon *hexagon){
    if(hexagon->legend)
        layer_set_hidden(text_layer_get_layer(hexagon->legend), false);
}
void hexagon_hide_legend(t_hexagon *hexagon){
    if(hexagon->legend)
        layer_set_hidden(text_layer_get_layer(hexagon->legend), true);
}