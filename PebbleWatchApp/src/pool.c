// pool screen code

#include <pebble.h>
#include <common.h>

// UI
static Window *window_pool;
static TextLayer *text_layer_app_name;
static TextLayer *text_layer_select;
static TextLayer *text_layer_pool;
static TextLayer *text_layer_size;
static TextLayer *text_layer_25;
static TextLayer *text_layer_50;

// Pointer variable to change the incoming parameter of the pool size by reference
static int *pool_size;

static void window_load(Window *window_pool) {

  Layer *window_layer = window_get_root_layer(window_pool);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Set the text of the application name layer
  text_layer_app_name = text_layer_create(GRect(0, 0, bounds.size.w, 20));
  text_layer_set_text(text_layer_app_name, "UbiSwim.org");
  text_layer_set_text_alignment(text_layer_app_name, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_app_name));

  // Set the text of the select text layer
  text_layer_select = text_layer_create(GRect(0, 40, 90, 30));
  text_layer_set_font(text_layer_select, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD)); 
  text_layer_set_text(text_layer_select, "Select");
  text_layer_set_text_alignment(text_layer_select, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_select));

  // Set the text of the pool text layer
  text_layer_pool = text_layer_create(GRect(0, 70, 90, 30));
  text_layer_set_font(text_layer_pool, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD)); 
  text_layer_set_text(text_layer_pool, "pool");
  text_layer_set_text_alignment(text_layer_pool, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_pool));

  // Set the text of the size text layer
  text_layer_size = text_layer_create(GRect(0, 100, 90, 40));
  text_layer_set_font(text_layer_size, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD)); 
  text_layer_set_text(text_layer_size, "size");
  text_layer_set_text_alignment(text_layer_size, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_size));

  // Set the text of the 25m text layer
  text_layer_25 = text_layer_create(GRect(90, 17, 50, 40));
  text_layer_set_font(text_layer_25, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD)); 
  text_layer_set_text(text_layer_25, ">25m");
  text_layer_set_text_alignment(text_layer_25, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_25));

  // Set the text of the Go! text layer
  text_layer_pool = text_layer_create(GRect(90, 68, 50, 30));
  text_layer_set_font(text_layer_pool, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD)); 
  text_layer_set_text(text_layer_pool, "Go!");
  text_layer_set_text_alignment(text_layer_pool, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_pool));

  // Set the text of the 50m layer
  text_layer_50 = text_layer_create(GRect(90, 115, 50, 40));
  text_layer_set_font(text_layer_50, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD)); 
  text_layer_set_text(text_layer_50, " 50m");
  text_layer_set_text_alignment(text_layer_50, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_50));
}

// Activate the 25m pool size option
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  static char s1[5];
  snprintf(s1, sizeof(s1), ">25m");
  text_layer_set_text(text_layer_25, s1);
  static char s3[5];
  snprintf(s3, sizeof(s3), " 50m");
  text_layer_set_text(text_layer_50, s3);

  *pool_size = 25;
}

// Remove the window from the stack and go to the stopwatch screen
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_remove(window_pool, false);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Disable the back button
}

// Activate the 50m pool size option
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  static char s1[5];
  snprintf(s1, sizeof(s1), " 25m");
  text_layer_set_text(text_layer_25, s1);
  static char s3[5];
  snprintf(s3, sizeof(s3), ">50m");
  text_layer_set_text(text_layer_50, s3);

  *pool_size = 50;
}

// Subscribe the click function handlers
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

// Destroy UI on window unload to free up memory
static void window_unload(Window *window_pool) {
  text_layer_destroy(text_layer_app_name);
  text_layer_destroy(text_layer_select);
  text_layer_destroy(text_layer_pool);
  text_layer_destroy(text_layer_size);
  text_layer_destroy(text_layer_25);
  text_layer_destroy(text_layer_50);
  window_destroy(window_pool);
}

// Create the pool screen UI
void show_pool(int *pool) {
  pool_size = pool;
  *pool = 25;
  window_pool = window_create();
  window_set_click_config_provider(window_pool, click_config_provider);
  window_set_window_handlers(window_pool, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = false;
  window_stack_push(window_pool, animated);
}