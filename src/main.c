#include <pebble.h>

Window *my_window;
int battery_level;

TextLayer *time_layer;
TextLayer *date_layer;
BitmapLayer *bitmap_layer;
Layer *battery_layer;
GFont eurocaps_font_big;
GFont eurocaps_font_small;
GBitmap *bg_bitmap;

void battery_layer_update(Layer *layer, GContext *ctx) {
  //GPoint p0 = GPoint(0, 0);
  //GPoint p1 = GPoint(100, 100);
  //GRect gr = GRect(50,50,100,100);
  GRect gr = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, GColorBlue);
  graphics_context_set_stroke_width(ctx, 4);
  graphics_draw_arc(ctx, gr, GOvalScaleModeFitCircle, 0, (battery_level*0.01)*TRIG_MAX_ANGLE );
  //graphics_draw_arc(ctx, gr, GOvalScaleModeFitCircle, 0, TRIG_MAX_ANGLE );
  //graphics_draw_circle(ctx, GPoint(35, 35), 30);
}

void my_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  int bitmap_y = 0;
  int time_y = bounds.size.h-60;
  int date_y = 0;
  
  GColor text_colour = GColorOrange;
  
  #if defined(PBL_BW)
  text_colour = GColorWhite;
  #endif
  
  #if defined(PBL_ROUND)
  bitmap_y = -4;
  time_y = bounds.size.h-70;
  date_y = 12;
  #endif
  
  bitmap_layer = bitmap_layer_create(GRect(0, bitmap_y, bounds.size.w, bounds.size.h-bitmap_y));
  time_layer = text_layer_create(GRect(-2, time_y, bounds.size.w, 56));
  date_layer = text_layer_create(GRect(0, date_y, bounds.size.w, 24));
  battery_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(battery_layer, battery_layer_update);
      
  // Improve the layout to be more like a watchface
  bitmap_layer_set_background_color(bitmap_layer, GColorBlack);
  bitmap_layer_set_bitmap(bitmap_layer, bg_bitmap);
  
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, text_colour);
  text_layer_set_text(time_layer, "00:00");
  text_layer_set_font(time_layer, eurocaps_font_big);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);

  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, text_colour);
  text_layer_set_text(date_layer, "");
  text_layer_set_font(date_layer, eurocaps_font_small);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));
  layer_add_child(window_layer, text_layer_get_layer(time_layer));
  layer_add_child(window_layer, text_layer_get_layer(date_layer));
  layer_add_child(window_layer, battery_layer);
}

void my_window_unload(Window *window) {
  layer_destroy(battery_layer);
  bitmap_layer_destroy(bitmap_layer);
  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
}

void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char time_buffer[10];
  strftime(time_buffer, sizeof(time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
 
  // Write the current date into a buffer
  static char date_buffer[20];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);

  // Display this time and date on TextLayers
  text_layer_set_text(time_layer, time_buffer);
  text_layer_set_text(date_layer, date_buffer);
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

void battery_handler(BatteryChargeState state) {
  battery_level = state.charge_percent;
  layer_mark_dirty(battery_layer);
}

void bluetooth_handler(bool connected) {

}

void handle_init(void) {
  eurocaps_font_big = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EUROCAPS_48));
  eurocaps_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EUROCAPS_24));
  bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ELITEDANGEROUS);

  my_window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = my_window_load,
    .unload = my_window_unload
  });

  window_stack_push(my_window, true);
  update_time();
  battery_handler(battery_state_service_peek());
  bluetooth_handler(connection_service_peek_pebble_app_connection());
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // Register for battery level updates
  battery_state_service_subscribe(battery_handler);
  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_handler
  });
}

void handle_deinit(void) {
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(my_window);
  fonts_unload_custom_font(eurocaps_font_big);
  fonts_unload_custom_font(eurocaps_font_small);
  gbitmap_destroy(bg_bitmap);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
