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
 *   Hexagons watchface
 */

#include <pebble.h>

#include "hexagon.h"

#define COLOR_H  GColorFromRGBA(255,20,0,255)
#define INITIAL_COLOR GColorBlack
#define NB_HEXAGONS 18
#define HEX_DAYNUM 7
#define HEX_DAY 6
#define HEX_WEEK 8
#define HEX_YEAR 15
#define HEX_BATT 1
#define HEX_MONTH 0

/* --------------------------- Function signatures ---------------------------*/

static void init_hexagons(int16_t hexa_size, int16_t hexa_border_size, int16_t border_width, Window *window);
static void next_color_update_animation(struct Animation *animation, const uint32_t time_normalized);

void next_color_animation_started(Animation *animation, void *data);
void next_color_animation_stopped(Animation *animation, bool finished, void *data);
static void legend_update_animation(struct Animation *animation, const uint32_t time_normalized);
static void display_legend();


/* --------------- Global variables ------------------------------------------*/
static Window *g_main_window;
static t_hexagon * hexs[NB_HEXAGONS];
static TextLayer * g_hour_layer;
static TextLayer * g_minute_layer;

static GFont s_custom_font;

static Animation *g_next_color_animation = NULL;
static Animation *g_display_legend_animation = NULL;

static int16_t g_current_hex;

static const AnimationImplementation g_alert_impl = {
    .setup = NULL,
    .teardown = NULL,
    .update = (AnimationUpdateImplementation) next_color_update_animation
};

static const    AnimationImplementation legend_impl = {
        .setup = NULL,
        .teardown = NULL,
        .update = (AnimationUpdateImplementation) legend_update_animation
};

static int16_t color_index = 0;

/** ----------------------------------------------------------------------------
 * @brief returns the next color to display. Random was an option, but we want to
 * make sure that 2 following color have a sufficient contrast to make the animation
 * enjoyable
 * @return 
 */
static GColor next_color(){

    switch(color_index){
        case 0 : return GColorGreen;
        case 1 : return GColorOrange;
        case 2 : return GColorCyan;
        case 3 : return GColorShockingPink;
        case 4 : return GColorYellow;
        case 5 : return GColorRed;
    }
    return GColorBlack;

}

/** ----------------------------------------------------------------------------
 * @brief starts the animation displaying the next color
 */
static void display_next_color(){

    /* The var is global, to ensure proper cleanup, but we reuse it for each new
     * animation */
    if( g_next_color_animation )
        animation_destroy(g_next_color_animation);

    g_next_color_animation= animation_create();

    animation_set_handlers(g_next_color_animation, (AnimationHandlers) {
            .started =  next_color_animation_started,
            .stopped =  next_color_animation_stopped,
            }, NULL);

    animation_set_implementation(g_next_color_animation, &g_alert_impl);
    animation_set_duration(g_next_color_animation, 1500);
    animation_set_curve( g_next_color_animation, AnimationCurveLinear );
    animation_schedule(g_next_color_animation);
}

/** ----------------------------------------------------------------------------
 * Update all fields related to time
 */
static void update_time() {
    time_t temp = time(NULL); 
    struct tm *tick_time = localtime(&temp);

    static char hour[] = "00";
    static char minute[] = "00";
    static char month[] = "zzz";
    static char daynum[] = "99";
    static char weeknum[] = "99";
    static char year[] = "99";
    static char day[] = "zzz";
    static char s_battery_buffer[] = "999";
    
    // Write the current hours and minutes into the buffer
    if(clock_is_24h_style() == true) {
        // Use 24 hour format
        strftime(hour, 3*sizeof(char), "%H", tick_time);
    } else {
        // Use 12 hour format
        strftime(hour, 3*sizeof(char), "%I", tick_time);
    }

    strftime(minute, 3*sizeof(char), "%M", tick_time);
    strftime(month, 4*sizeof(char), "%b", tick_time );
    strftime(daynum, 3*sizeof(char), "%d", tick_time );
    strftime(weeknum, 3*sizeof(char), "%W", tick_time );
    strftime(year, 3*sizeof(char), "%y", tick_time );
    strftime(day, 4*sizeof(char), "%a", tick_time );

    BatteryChargeState charge_state = battery_state_service_peek();
    if (charge_state.is_charging) {
        snprintf(s_battery_buffer, sizeof(s_battery_buffer), "--");
    } else {
        snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d", charge_state.charge_percent);
    }

    // Display the new texts
    text_layer_set_text(g_hour_layer, hour);
    text_layer_set_text(g_minute_layer, minute);
    
    
    hexagon_set_text(hexs[HEX_MONTH], month);
    hexagon_set_text(hexs[HEX_DAYNUM], daynum);
    hexagon_set_text(hexs[HEX_WEEK], weeknum);
    hexagon_set_text(hexs[HEX_YEAR], year);
    hexagon_set_text(hexs[HEX_DAY], day);
    hexagon_set_text(hexs[HEX_BATT], s_battery_buffer);
}

/** ----------------------------------------------------------------------------
 * @brief called when time changes
 * @param tick
 * @param units_changed
 */
static void tick_handler( struct tm *tick, TimeUnits units_changed ){
    update_time();

    /*For a reason, the tick_handler function is called when the watchface loads,
     we do not want to trigger a color change right at loading, but only when time
     actually changes*/
    if( units_changed & MINUTE_UNIT )
        display_next_color();

}

/** ----------------------------------------------------------------------------
 * @brief creates a text layer
 * @param frame
 * @param color
 * @param init_text
 * @param font
 * @param parent_layer
 * @return 
 */
static TextLayer *init_text_layer( GRect frame, 
        GColor color, 
        const char *init_text, 
        GFont font, 
        Layer *parent_layer ){

    TextLayer *layer;
    layer = text_layer_create(frame);
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_text_color(layer, color);
    text_layer_set_text(layer, init_text);

    text_layer_set_font(layer, font);
    text_layer_set_text_alignment(layer, GTextAlignmentCenter);

    layer_add_child(parent_layer, text_layer_get_layer(layer));

    return layer;
}

/** ----------------------------------------------------------------------------
 * @brief callback called when the color change animation starts
 * @param animation
 * @param data
 */
void next_color_animation_started(Animation *animation, void *data) {

    g_current_hex = 0;
}

/** ----------------------------------------------------------------------------
 * @brief called when the color change animation stops
 * @param animation
 * @param finished
 * @param data
 */
void next_color_animation_stopped(Animation *animation, bool finished, void *data) {
    g_current_hex = 0;
    update_time();
    if(++color_index > 5)
        color_index = 0;
    
    /* The first tiem we load the watchface (i.e. when the legend animation is still
     NULL ), we show the legend.*/
    if( g_display_legend_animation == NULL )
        display_legend();
}

/** ----------------------------------------------------------------------------
 * @brief callback called by the Animation mechanism of the pebble API.
 * This is where we gradually change the color of the hexagons
 * @param animation
 * @param time_normalized
 */
static void next_color_update_animation(struct Animation *animation, const uint32_t time_normalized){

    uint16_t i;
    for(i = g_current_hex ; i < time_normalized / ( ANIMATION_NORMALIZED_MAX / NB_HEXAGONS ); i++){
        if( i < NB_HEXAGONS )
            hexagon_set_color(hexs[i], next_color());
    }
    if(i != g_current_hex)
        g_current_hex = i;

}
/** ----------------------------------------------------------------------------
 * @brief callback called when the legend animation starts
 * ( shows the legend text on all hexagons )
 * @param animation
 * @param data
 */
void legend_animation_started(Animation *animation, void *data) {
    int16_t i;
    for(i = 0 ; i < NB_HEXAGONS ; i++){
        hexagon_show_legend(hexs[i]);
    }
}

/** ----------------------------------------------------------------------------
 * @brief callback called when the legend animation stops
 * @param animation
 * @param finished
 * @param data
 */
void legend_animation_stopped(Animation *animation, bool finished, void *data) {
    int16_t i;
    for(i = 0 ; i < NB_HEXAGONS ; i++){
        hexagon_hide_legend(hexs[i]);
    }
}

/** ----------------------------------------------------------------------------
 * @brief ;
 * @param animation
 * @param time_normalized
 */
static void legend_update_animation(struct Animation *animation, const uint32_t time_normalized){
    ;//do nothing
}

/** ----------------------------------------------------------------------------
 * 
 */
static void display_legend(){

    g_display_legend_animation = animation_create();

    animation_set_handlers(g_display_legend_animation, (AnimationHandlers) {
            .started =  legend_animation_started,
            .stopped =  legend_animation_stopped,
            }, NULL);


    
    animation_set_implementation(g_display_legend_animation, &legend_impl);
    animation_set_duration(g_display_legend_animation, 1000);
    animation_set_curve( g_display_legend_animation, AnimationCurveLinear );
    animation_schedule(g_display_legend_animation);
}

/** ----------------------------------------------------------------------------
 * 
 * @param window
 */
static void main_window_load(Window *window){

    int16_t side = 26;
    int16_t b_width = 3;
    
    s_custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_BOLD_35));

    init_hexagons(side, side-4, b_width, window);

    g_hour_layer = init_text_layer( GRect(2, 72, 58, 50) , GColorWhite, "--", s_custom_font, window_get_root_layer(window) );
    g_minute_layer = init_text_layer( GRect(3*(144/5), 72, 58, 50) , GColorWhite, "--", s_custom_font, window_get_root_layer(window) );

    hexagon_init_text_layer(hexs[HEX_MONTH],GRect(0, 5, 52, 40),GColorBlack, "   ", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    hexagon_init_text_layer(hexs[HEX_DAY],GRect(0, 5, 52, 40), GColorBlack, "   ", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    hexagon_init_text_layer(hexs[HEX_DAYNUM],GRect(0, 5, 52, 40), GColorBlack, "   ", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    hexagon_init_text_layer(hexs[HEX_WEEK],GRect(0, 5, 52, 40), GColorBlack, "   ", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    hexagon_init_text_layer(hexs[HEX_YEAR],GRect(0, 5, 52, 40), GColorBlack, "   ", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    hexagon_init_text_layer(hexs[HEX_BATT],GRect(0, 5, 52, 40), GColorBlack, "   ", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

    hexagon_set_legend(hexs[HEX_MONTH], "mon");
    hexagon_set_legend(hexs[HEX_DAY], "day");
    hexagon_set_legend(hexs[HEX_DAYNUM], "day");
    hexagon_set_legend(hexs[HEX_WEEK], "week");
    hexagon_set_legend(hexs[HEX_YEAR], "year");
    hexagon_set_legend(hexs[HEX_BATT], "batt");

    srand(time(NULL));
    color_index = rand() % 6;
    update_time();
    display_next_color();
}

/** ----------------------------------------------------------------------------
 * 
 * @param window
 */
static void main_window_unload(Window *window){
    int i;
    for(i = 0; i < 16; i++)
        destroy_hexagon(hexs[i]);

    text_layer_destroy(g_hour_layer);
    text_layer_destroy(g_minute_layer);

    if(g_next_color_animation)
        animation_destroy( g_next_color_animation );
    
    if( g_display_legend_animation )
        animation_destroy(g_next_color_animation);
}

/** ----------------------------------------------------------------------------
 * 
 */
static void init(){

    WindowHandlers handlers = {
        .load = main_window_load,
        .unload = main_window_unload
    };

    g_main_window = window_create();

    window_set_window_handlers( g_main_window, handlers );
    window_set_background_color( g_main_window, GColorBlack );

    window_stack_push(g_main_window, true);

    // init timer
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

/** ----------------------------------------------------------------------------
 * 
 */
static void deinit(){
    window_destroy(g_main_window);
}

/** ----------------------------------------------------------------------------
 * 
 * @return 
 */
int main(void) {
    init();
    app_event_loop();
    deinit();
}

/** ----------------------------------------------------------------------------
 * @brief This huge function declares all the hexagons that are displayed on the screen.
 * Layout : 
 * 
 *     __    __
 *  __/12\__/11\__
 * /9 \__/8 \__/10\_
 * \__/7 \__/6 \__/
 * /4 \__/ 2\__/5 \_
 * \__/  \__/  \__/
 * /16\__/15\__/3 \_
 * \__/1 \__/0 \__/
 * /14\__/13\__/17\_
 * \__/  \__/  \__/
 * 
 * @param hexa_size
 * @param hexa_border_size
 * @param border_width
 * @param window
 */
static void init_hexagons(int16_t hexa_size, int16_t hexa_border_size, int16_t border_width, Window *window){

    int16_t base_interval = 144/5;

    hexs[0] = create_hexagon_with_border(
            GPoint( 4*base_interval, 168 - base_interval * HALF_SQRT_3  ), 
            hexa_size,
            INITIAL_COLOR, 
            hexa_border_size, 
            GColorBlack, 
            border_width, 
            window_get_root_layer(window));

    hexs[1] = create_hexagon_with_border(
            GPoint( base_interval +2, 168 - base_interval * HALF_SQRT_3  ),
            hexa_size, INITIAL_COLOR,hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[2] = create_hexagon_with_border(
            GPoint( 144/2 - 1, 168 - base_interval * HALF_SQRT_3 * 4 ),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[3] = create_hexagon_with_border(
            GPoint(144 + base_interval/2 - 5, 168 - base_interval * HALF_SQRT_3 * 2  ),
            hexa_size, INITIAL_COLOR,hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[4] = create_hexagon_with_border(
            GPoint(-base_interval / 2 + 3, 168 - base_interval * HALF_SQRT_3 * 4 ),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[5] = create_hexagon_with_border(
            GPoint(144 + base_interval/2 - 5, 168 - base_interval * HALF_SQRT_3 * 4 ),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[6] = create_hexagon_with_border(
            GPoint( 4*base_interval, 168 - base_interval * HALF_SQRT_3 * 5 + 1),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[7] = create_hexagon_with_border(
            GPoint( base_interval+2, 168 - base_interval * HALF_SQRT_3 * 5 + 1),
            hexa_size,INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[8] = create_hexagon_with_border(
            GPoint( 144/2 - 1, 168 - base_interval * HALF_SQRT_3 * 6 ),
            hexa_size, INITIAL_COLOR,hexa_border_size,  GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[9] = create_hexagon_with_border(
            GPoint(-base_interval / 2 + 3, 168 - base_interval * HALF_SQRT_3 * 6 ),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[10] = create_hexagon_with_border(
            GPoint(144 + base_interval/2 - 5, 168 - base_interval * HALF_SQRT_3 * 6 ),
            hexa_size,INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[11] = create_hexagon_with_border(
            GPoint( 4*base_interval, -2 ),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[12] = create_hexagon_with_border(
            GPoint( base_interval+2, -2),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[13] = create_hexagon_with_border(
            GPoint( 144/2 - 1, 167 ),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[14] = create_hexagon_with_border(
            GPoint(-base_interval / 2 + 3, 167 ),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[15] = create_hexagon_with_border(
            GPoint( 144/2 - 1, 168 - base_interval * HALF_SQRT_3 * 2 ),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));

    hexs[16] = create_hexagon_with_border(
            GPoint(-base_interval / 2 + 3, 168 - base_interval * HALF_SQRT_3 * 2 ),
            hexa_size, INITIAL_COLOR,  hexa_border_size,  GColorBlack,
            border_width, window_get_root_layer(window));
    hexs[17] = create_hexagon_with_border(
            GPoint(144 + base_interval/2 - 5, 167 ),
            hexa_size, INITIAL_COLOR, hexa_border_size, GColorBlack,
            border_width, window_get_root_layer(window));                          
}
