// splash screen code

#include <pebble.h>
#include <common.h>

// UI
static Window *window_splash;
static BitmapLayer *s_bitmap_layer;
static GBitmap *s_bitmap;

static AppTimer* timer = NULL;

// Remove splash screen window from the window stack
static void timer_handler(void* data) {
  window_stack_pop(true);
}

// Display splash screem on window load
static void window_load(Window *window_splash) {

  Layer *window_layer = window_get_root_layer(window_splash);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Load the UbiSwim logo image
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LOGO);

  // Create a BitmapLayer
  s_bitmap_layer = bitmap_layer_create(bounds);

  // Set the bitmap and compositing mode
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet); // Required for transparent png images(!)

  // Add to the Window
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));

  // Register a timer so the splash screen hides after 5 seconds
  timer = app_timer_register(5000, timer_handler, NULL);

}

// Destroy UI on window unload to free up memory
static void window_unload(Window *window_splash) {
  bitmap_layer_destroy(s_bitmap_layer);
  gbitmap_destroy(s_bitmap);
  window_destroy(window_splash);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Disable the back button to avoid splash screen removal by the user
}

// Subscribe the click function handlers
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

// Create the splash screen UI
void show_splash() {
  window_splash = window_create();
  window_set_click_config_provider(window_splash, click_config_provider);
  window_set_window_handlers(window_splash, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = false;
  window_stack_push(window_splash, animated);
}