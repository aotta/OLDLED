#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_date_layer, *s_time_layer, *s_minutes_layer, *s_sec_layer;
static Layer *s_line_layer, *s_line_layer2;
static GFont s_custom_font_60;
static GFont s_custom_font_12;

static void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  // static char s_time_text[] = "00:00";
  static char s_hour_text[] = "00";
  static char s_min_text[] = "00";
  static char *s_sec_text = ":";
  
  static char s_date_text[] = "Xxxxxxxxx 00";
  

  strftime(s_date_text, sizeof(s_date_text), "%B %e", tick_time);
  text_layer_set_text(s_date_layer, s_date_text);
    
  char *time_format = "%I";
      
  strftime(s_hour_text, sizeof(s_hour_text), time_format, tick_time);
  
  // Handle lack of non-padded hour format string for twelve hour clock.
  if (s_hour_text[0] == '0') {
    memmove(s_hour_text, &s_hour_text[1], sizeof(s_hour_text) - 1);
  }
  
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  text_layer_set_text(s_time_layer, s_hour_text);
  
  
  // setta i minuti
  strftime(s_min_text, sizeof(s_min_text), "%M", tick_time);
  text_layer_set_text_alignment(s_minutes_layer, GTextAlignmentLeft);
  text_layer_set_text(s_minutes_layer, s_min_text);
  
  // secondi lampeggianti
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int32_t second_c = t->tm_sec;
  int32_t pari= second_c % 2;
  
  // setta i due punti (secondi)
  if (pari == 0) {
    s_sec_text = ":";  
    } else {
       s_sec_text  = " ";
    } 
  text_layer_set_text_alignment(s_sec_layer, GTextAlignmentCenter);
  text_layer_set_text(s_sec_layer, s_sec_text);
  
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  // Imposta il layer per la data
  s_date_layer = text_layer_create(GRect(8, 38, 136, 30));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  
   // custom fonts!
  s_custom_font_12 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LCD_12));
    
  text_layer_set_font(s_date_layer, s_custom_font_12);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  // imposta il layer per l'ora
  s_time_layer = text_layer_create(GRect(1, 54, 66, 110));
  text_layer_set_text_color(s_time_layer, GColorWhite);
 
  text_layer_set_background_color(s_time_layer, GColorClear);
  
  s_custom_font_60 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LED_REAL_60));
    
  text_layer_set_font(s_time_layer, s_custom_font_60);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // imposta il layer per i due punti (secondi)
  s_sec_layer = text_layer_create(GRect(67, 54, 14, 110));
  
  text_layer_set_text_color(s_sec_layer, GColorWhite);
  text_layer_set_background_color(s_sec_layer, GColorClear);
  text_layer_set_font(s_sec_layer, s_custom_font_60);
  layer_add_child(window_layer, text_layer_get_layer(s_sec_layer));

  // imposta il layer per i minuti 
  s_minutes_layer = text_layer_create(GRect(80, 54, 66, 110));
  
  text_layer_set_text_color(s_minutes_layer, GColorWhite);
  text_layer_set_background_color(s_minutes_layer, GColorClear);
  text_layer_set_font(s_minutes_layer, s_custom_font_60);
  layer_add_child(window_layer, text_layer_get_layer(s_minutes_layer));


  // imposta il layer per la linea di separazione alta
  GRect line_frame = GRect(8, 30, 139, 2);
  s_line_layer = layer_create(line_frame);
  layer_set_update_proc(s_line_layer, line_layer_update_callback);
  layer_add_child(window_layer, s_line_layer);
  
   // imposta il layer per la linea di separazione bassa
  GRect line_frame2 = GRect(8, 130, 139, 2);
  s_line_layer2 = layer_create(line_frame2);
  layer_set_update_proc(s_line_layer2, line_layer_update_callback);
  layer_add_child(window_layer, s_line_layer2);
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_minutes_layer);
  text_layer_destroy(s_sec_layer);
  
  fonts_unload_custom_font(s_custom_font_12);
  fonts_unload_custom_font(s_custom_font_60);
  layer_destroy(s_line_layer);
  layer_destroy(s_line_layer2);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  
  // Prevent starting blank
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_second_tick(t, SECOND_UNIT);
}

static void deinit() {
  window_destroy(s_main_window);

  tick_timer_service_unsubscribe();
}

int main() {
  init();
  app_event_loop();
  deinit();
}
