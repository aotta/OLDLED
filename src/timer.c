#include <pebble.h>

  // keys for app message and storage
#define BATTERY_MODE   1
#define DATE_MODE      2
#define BLUETOOTH_MODE 3
#define GRAPHICS_MODE  4
#define CONNLOST_MODE  5
#define REQUEST_CONFIG 100

#define BATTERY_MODE_NEVER    0
#define BATTERY_MODE_IF_LOW   1
#define BATTERY_MODE_ALWAYS   2
#define DATE_MODE_OFF         0
#define DATE_MODE_FIRST       1
#define DATE_MODE_EN          1
#define DATE_MODE_DE          2
#define DATE_MODE_ES          3
#define DATE_MODE_FR          4
#define DATE_MODE_IT          5
#define DATE_MODE_SE          6
#define DATE_MODE_LAST        6
#define BLUETOOTH_MODE_NEVER  0
#define BLUETOOTH_MODE_IFOFF  1
#define BLUETOOTH_MODE_ALWAYS 2
#define GRAPHICS_MODE_NORMAL  0
#define GRAPHICS_MODE_INVERT  1
#define CONNLOST_MODE_IGNORE  0
#define CONNLOST_MODE_WARN    1

static int battery_mode   = BATTERY_MODE_IF_LOW;
static int date_mode      = DATE_MODE_EN;
static int bluetooth_mode = BLUETOOTH_MODE_ALWAYS;
static int graphics_mode  = GRAPHICS_MODE_NORMAL;
static int connlost_mode  = CONNLOST_MODE_IGNORE;
static bool has_config = false;
  
static Window *s_main_window;
static TextLayer *s_date_layer, *s_time_layer, *s_minutes_layer, *s_sec_layer;
static Layer *s_line_layer, *s_line_layer2;
static GFont s_custom_font_60;
static GFont s_custom_font_12;


static InverterLayer *inverter_layer;

static void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}


void handle_inverter() {
  if (layer_get_hidden(inverter_layer_get_layer(inverter_layer)) != (graphics_mode == GRAPHICS_MODE_NORMAL))
    layer_set_hidden(inverter_layer_get_layer(inverter_layer), graphics_mode == GRAPHICS_MODE_NORMAL);
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

void handle_appmessage_receive(DictionaryIterator *received, void *context) {
  Tuple *tuple = dict_read_first(received);
  while (tuple) {
    switch (tuple->key) {
      case BATTERY_MODE:
        battery_mode = tuple->value->int32;
        break;
      case DATE_MODE:
        date_mode = tuple->value->int32;
        break;
      case BLUETOOTH_MODE:
        bluetooth_mode = tuple->value->int32;
        break;
      case GRAPHICS_MODE:
        graphics_mode = tuple->value->int32;
        break;
      case CONNLOST_MODE:
        connlost_mode = tuple->value->int32;
        break;
    }
    tuple = dict_read_next(received);
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received config");
  has_config = true;
//  handle_battery(battery_state_service_peek());
//  handle_bluetooth(bluetooth_connection_service_peek());
  handle_inverter();
//  layer_mark_dirty(hands_layer);
//  layer_mark_dirty(date_layer);
}

void request_config(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Requesting config");
  Tuplet request_tuple = TupletInteger(REQUEST_CONFIG, 1);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (!iter) return;
  dict_write_tuplet(iter, &request_tuple);
  dict_write_end(iter);
  app_message_outbox_send();
}


static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
  layer_add_child(window_get_root_layer(s_main_window), inverter_layer_get_layer(inverter_layer));

  
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  
  // Prevent starting blank
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_second_tick(t, SECOND_UNIT);
}

static void deinit() {
  app_message_deregister_callbacks();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  if (has_config) {
    persist_write_int(BATTERY_MODE, battery_mode);
    persist_write_int(DATE_MODE, date_mode);
    persist_write_int(BLUETOOTH_MODE, bluetooth_mode);
    persist_write_int(GRAPHICS_MODE, graphics_mode);
    persist_write_int(CONNLOST_MODE, connlost_mode);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Wrote config");
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Did not write config");
  }
  
  window_destroy(s_main_window);

  tick_timer_service_unsubscribe();
  inverter_layer_destroy(inverter_layer);
  
}

int main() {
  init();
  app_event_loop();
  deinit();
}
