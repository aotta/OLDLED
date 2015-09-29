#include <pebble.h>

  // keys for app message and storage
enum Settings { setting_screen = 1, 
							 setting_date, 
							 setting_vibrate, 
							 setting_battery, 
							 setting_riserva, 
							 setting_ledcolorF, 
							 setting_ledcolorB
						   };

static Window *s_main_window;
static TextLayer *s_date_layer, *s_time_layer, *s_minutes_layer, *s_sec_layer;
static TextLayer *s_battery_layer;
static Layer *s_line_layer, *s_line_layer2;
static GFont s_custom_font_60;
static GFont s_custom_font_12;
// static InverterLayer *inverter_layer;
static bool visible;
static bool lineon;
static int battery_percent;
static char battery_text[] = "FU";
static int low_is;
static GColor Colore;
static GColor bkColore;
static GColor lineColor;

static enum SettingRiserva { low_is_30 = 0, low_is_20, riserva_count } riserva;
static enum SettingScreen { screen_black = 0, screen_white, screen_count } screen;
static enum SettingDate { date_month_day = 0, date_day_month, date_count } date;
static enum SettingVibrate { vibrate_none = 0, vibrate_hourly, vibrate_count } vibrate;
static enum SettingBattery { battery_none = 0, battery_show, battery_count } battery;
static enum SettingLedcolorF { lcf, ledf_count } ledcolorF;
static enum SettingLedcolorB { lcb, ledb_count } ledcolorB;



static AppSync app;
static uint8_t buffer[256];
  
  
static void line_layer_update_callback(Layer *layer, GContext* ctx) {
  if (screen == screen_black) {
		lineColor = Colore;
		} else {
		lineColor = bkColore;
	}
	graphics_context_set_fill_color(ctx, lineColor);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void app_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "app error %d", app_message_error);
}

static void handle_battery(BatteryChargeState charge_state) {
  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "CH");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
    battery_percent=charge_state.charge_percent;
  }
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "battery text: %s", battery_text);
  // text_layer_set_text(s_battery_layer, battery_text);
}


static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
// Need to be static because they're used by the system later.
// static char s_time_text[] = "00:00";
  static char s_hour_text[] = "00";
  static char s_min_text[] = "00";
  static char *s_sec_text = ":";
 
  static char s_date_text[] = "Xxxxxxxxx 00";
  
  if (date == 0) {
    strftime(s_date_text, sizeof(s_date_text), "%B %e", tick_time);
  } else {
    strftime(s_date_text, sizeof(s_date_text), "%e %B", tick_time);}
  
  text_layer_set_text(s_date_layer, s_date_text);
    
  char *time_format;

   if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I";
  }
  
  strftime(s_hour_text, sizeof(s_hour_text), time_format, tick_time);
  
  
  // Handle lack of non-padded hour format string for twelve hour clock.
  if (!clock_is_24h_style() && (s_hour_text[0] == '0')) {
      memmove(s_hour_text, &s_hour_text[1], sizeof(s_hour_text) - 1);
  }
  
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  text_layer_set_text(s_time_layer, s_hour_text);
  
  // imposta i colori
	window_set_background_color(s_main_window, bkColore);
	text_layer_set_text_color(s_date_layer, Colore);
  text_layer_set_background_color(s_date_layer, bkColore);
	text_layer_set_text_color(s_time_layer, Colore);
  text_layer_set_background_color(s_time_layer, bkColore);
	text_layer_set_text_color(s_minutes_layer, Colore);
  text_layer_set_background_color(s_minutes_layer, bkColore);
	text_layer_set_text_color(s_sec_layer, Colore);
  text_layer_set_background_color(s_sec_layer, bkColore);
	text_layer_set_text_color(s_battery_layer, Colore);
  text_layer_set_background_color(s_battery_layer, bkColore);

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
    visible = -1;
    } else {
       s_sec_text  = " ";  
       visible = 0;
    } 
  text_layer_set_text_alignment(s_sec_layer, GTextAlignmentCenter);
  text_layer_set_text(s_sec_layer, s_sec_text);
 
//  check for hourly vibration notification

  if ((vibrate == vibrate_hourly) && (units_changed & HOUR_UNIT)) {
    vibes_short_pulse();  
  }

  //  check for battery shorting, and in case print "lo" warning
  
  if ((battery == battery_show) && (battery_percent <= low_is)) {
   text_layer_set_text(s_battery_layer, "LO");
  } else {
    text_layer_set_text(s_battery_layer, "");
  }
}

static void setdefcol() {
  
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting colori default");
  Colore = GColorFromRGB(255,0, 0);
	bkColore = GColorFromRGB(0, 0, 0);	
}

static void tuple_changed_callback(const uint32_t key, const Tuple* tuple_new, const Tuple* tuple_old, void* context) {
  //  we know these values are uint8 format
  int value = tuple_new->value->uint8;
	// APP_LOG(APP_LOG_LEVEL_DEBUG, "KEY: %lu  Value: %i",key,value);
  switch (key) {
    case setting_screen:
	    APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting_screen");
      if ((value >= 0) && (value < screen_count)) {
       //  update value
        screen = value;
			 //	layer_set_hidden(inverter_layer_get_layer(inverter_layer), (screen == screen_black));
				if (screen_black == screen) {
					lineon = -1;
				}
      }       
      break;
    
    case setting_date:
       APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting_date");
       if ((value >= 0) && (value < date_count)) {
      	 //  redraw date
         date = value;
         //handle_second_tick(NULL,0);
       }
       break;
    
    case setting_vibrate:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting_vibrate");
      if ((value >= 0) && (value < vibrate_count)) {
        //  update value
      vibrate = value;
	    }
      break;
		
		 case setting_battery:
	    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting_battery");
      if ((value >= 0) && (value < battery_count)) {
 				//  update value
        battery = value;
      }
      break;
		
    case setting_riserva:
		  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting_riserva-value: %i",value);
      if ((value == 0) && (value < riserva_count)) {
        riserva=value;
				low_is = 20;
      } else {
        riserva=value;
				low_is = 30;
      }
	    break;
	 
	  case setting_ledcolorF:
		  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting_ledcolorF: %i", value);
			ledcolorF = value;
			Colore.argb =  ((uint8_t)(0xC0|ledcolorF));
			break;
		
		case setting_ledcolorB:
    	//APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting_ledcolorB: %i", value);
		  ledcolorB = value;
			bkColore.argb = ((uint8_t)(0xC0|ledcolorB));
		  if (gcolor_equal(Colore, bkColore)) {
				  Colore = GColorFromRGB(255,0, 0);
					bkColore = GColorFromRGB(0, 0, 0);	
			}
			break;
	}
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  // Imposta il layer per la data
  s_date_layer = text_layer_create(GRect(8, 38, 136, 30));
  text_layer_set_text_color(s_date_layer, Colore);
  text_layer_set_background_color(s_date_layer, bkColore);
  
   // custom fonts!
  s_custom_font_12 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LCD_12));
    
  text_layer_set_font(s_date_layer, s_custom_font_12);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  // imposta il layer per l'ora
  s_time_layer = text_layer_create(GRect(1, 54, 64, 110));
	
  text_layer_set_text_color(s_time_layer, Colore);
 
  text_layer_set_background_color(s_time_layer, bkColore);
  
  s_custom_font_60 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LED_REAL_60));
    
  text_layer_set_font(s_time_layer, s_custom_font_60);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // imposta il layer per i due punti (secondi)
  s_sec_layer = text_layer_create(GRect(65, 54, 14, 110));
  
  text_layer_set_text_color(s_sec_layer, Colore);
  text_layer_set_background_color(s_sec_layer, bkColore);
  text_layer_set_font(s_sec_layer, s_custom_font_60);
  layer_add_child(window_layer, text_layer_get_layer(s_sec_layer));

  // imposta il layer per i minuti 
  s_minutes_layer = text_layer_create(GRect(80, 54, 64, 110));
  text_layer_set_text_color(s_minutes_layer, Colore);
  text_layer_set_background_color(s_minutes_layer, bkColore);
  text_layer_set_font(s_minutes_layer, s_custom_font_60);
  layer_add_child(window_layer, text_layer_get_layer(s_minutes_layer));


  // imposta il layer per la linea di separazione alta
  	GRect line_frame = GRect(8, 30, 130, 2);
  	s_line_layer = layer_create(line_frame);
  	layer_set_update_proc(s_line_layer, line_layer_update_callback);
  	layer_add_child(window_layer, s_line_layer);
  
   // imposta il layer per la linea di separazione bassa
  	GRect line_frame2 = GRect(8, 136, 130, 2);
  	s_line_layer2 = layer_create(line_frame2);
  	layer_set_update_proc(s_line_layer2, line_layer_update_callback);
  	layer_add_child(window_layer, s_line_layer2);
	
   // imposta il layer per l'indicazione low battery
  s_battery_layer = text_layer_create(GRect(120, 8, 30, 12));
  text_layer_set_text_color(s_battery_layer, Colore);
  text_layer_set_background_color(s_battery_layer, bkColore);
  text_layer_set_font(s_battery_layer, s_custom_font_12);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer)); 
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_minutes_layer);
  text_layer_destroy(s_sec_layer);
  text_layer_destroy(s_battery_layer);
  
  fonts_unload_custom_font(s_custom_font_12);
  fonts_unload_custom_font(s_custom_font_60);
  layer_destroy(s_line_layer);
  layer_destroy(s_line_layer2);
}

static void init() {
  
  // default setting
  
  screen = screen_black;
  date = date_day_month;
  vibrate = vibrate_hourly;
  battery = battery_show;
	low_is = 30;
	setdefcol();
  s_main_window = window_create();
  window_set_background_color(s_main_window, bkColore);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  //  inverter
//  inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
//  layer_add_child(window_get_root_layer(s_main_window), inverter_layer_get_layer(inverter_layer));
  
  //  app communication
  Tuplet tuples[] = {
    TupletInteger(setting_screen, screen),
    TupletInteger(setting_date, date),
    TupletInteger(setting_vibrate, vibrate),
    TupletInteger(setting_battery, battery),
    TupletInteger(setting_riserva, riserva),
		TupletInteger(setting_ledcolorF, ledcolorF),
		TupletInteger(setting_ledcolorB, ledcolorB)
  };
  app_message_open(160, 160);
  app_sync_init(&app, buffer, sizeof(buffer), tuples, ARRAY_LENGTH(tuples),
                tuple_changed_callback, app_error_callback, NULL);
	
  //  display time (immediately before first tick event)
  // handle_second_tick(t, SECOND_UNIT);
  
  //  tick service
    tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  
  handle_battery(battery_state_service_peek());
  // battery service
  battery_state_service_subscribe(handle_battery);

  
  // Prevent starting blank
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_second_tick(t, SECOND_UNIT);
  battery_state_service_peek();
  
}

static void deinit() {
  
  window_destroy(s_main_window);

    //  tick unsubscribe
  tick_timer_service_unsubscribe();
  // battery unsubscribe
  battery_state_service_unsubscribe();
  //  inverter
//  inverter_layer_destroy(inverter_layer);
  //  app unsync
  app_sync_deinit(&app);
  
}

int main() {
  init();
  app_event_loop();
  deinit();
}
