// score screen code

#include <pebble.h>
#include <common.h>

// UI
static Window *window_score;
static TextLayer *text_layer_app_name;
static TextLayer *text_layer_ssi;
static TextLayer *text_layer_msg;
static TextLayer *text_layer_info;

// SWOLF Score Improvement (SSI) Avatar
static BitmapLayer *ssi_bitmap_layer;
static GBitmap *ssi_bitmap;

// SWOLF Score variables
static int ssi = 0;          // SSI: SWOLF Score Improvement
static int *swolf_avg_prev;  // SWOLF score average of the last workout

static void window_load(Window *window_score) {

  Layer *window_layer = window_get_root_layer(window_score);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Set the text of the application name layer
  text_layer_app_name = text_layer_create(GRect(0, 0, bounds.size.w, 20));
  text_layer_set_text(text_layer_app_name, "UbiSwim.org");
  text_layer_set_text_alignment(text_layer_app_name, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_app_name));

  // Set the text of the SSI layer
  text_layer_ssi = text_layer_create(GRect(0, 20, bounds.size.w, 30));
  text_layer_set_font(text_layer_ssi, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD)); 
  static char s_buffer_ssi[10];
  snprintf(s_buffer_ssi, sizeof(s_buffer_ssi), "SSI: %d%%", ssi);
  text_layer_set_text(text_layer_ssi, s_buffer_ssi);
  text_layer_set_text_alignment(text_layer_ssi, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_ssi));

  // Set the text of the motivational message layer
  text_layer_msg = text_layer_create(GRect(0, 50, bounds.size.w, 40));
  text_layer_set_font(text_layer_msg, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD)); 
  if (ssi > 0) {
    text_layer_set_text(text_layer_msg, "Hooray!");
  } else {
    text_layer_set_text(text_layer_msg, "Keep it going!");
  }
  text_layer_set_text_alignment(text_layer_msg, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_msg));

  // Show smiley
  bounds = GRect(0, 58, bounds.size.w, 120);

  // Load the image
  if (ssi > 0) {
    ssi_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SMILEY_HAPPY); // If there is some SWOLF score improvement
  } else {
    ssi_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SMILEY); // If there is no SWOLF score improvement
  }

  // Create a BitmapLayer
  ssi_bitmap_layer = bitmap_layer_create(bounds);

  // Set the bitmap and compositing mode
  bitmap_layer_set_bitmap(ssi_bitmap_layer, ssi_bitmap);
  bitmap_layer_set_compositing_mode(ssi_bitmap_layer, GCompOpSet);

  // Add to the Window
  layer_add_child(window_layer, bitmap_layer_get_layer(ssi_bitmap_layer));

  // Display info message
  text_layer_info = text_layer_create(GRect(0, 150, bounds.size.w, 16));
  text_layer_set_text(text_layer_info, "M:Hold to reset history");
  text_layer_set_text_alignment(text_layer_info, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_info));

}

// Set the long middle click to reset the SWOLF score average history
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  *swolf_avg_prev = 0;
  vibes_long_pulse();
  window_stack_remove(window_score, false); // Return to the stopwatch screen
}

// Subscribe the click function handlers
static void click_config_provider(void *context) {
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_long_click_handler, NULL);
}

// Destroy UI on window unload to free up memory
static void window_unload(Window *window_score) {
  text_layer_destroy(text_layer_app_name);
  text_layer_destroy(text_layer_ssi);
  text_layer_destroy(text_layer_msg);
  window_destroy(window_score);
}

// Create the score screen UI
void show_score(int ssi_in, int *swolf_avg_prev_in) {
  ssi = ssi_in;
  swolf_avg_prev = swolf_avg_prev_in;
  window_score = window_create();
  window_set_click_config_provider(window_score, click_config_provider);
  window_set_window_handlers(window_score, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = false;
  window_stack_push(window_score, animated);
}