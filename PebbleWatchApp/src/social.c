// social screen code

#include <pebble.h>
#include <common.h>

// UI
static Window *window;
static TextLayer *text_layer_app_name;
static TextLayer *text_layer_likes;
static TextLayer *text_layer_msg;

// Scroll layer for displaying social "likes"
static ScrollLayer *s_scroll_layer;

// Text layer to scroll in the scroll layer
static TextLayer *s_text_layer;
static int likes_int = 0;
static char likes_str[20] = "";

// Create a 2000 characters long empty string
static char s_scroll_text[] = "                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                ";

// Function prototypes
static void click_config_provider_updown(void *context);
static void update_likes(void);
static void update_social(void);

static void window_load(Window *window) {

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GRect max_text_bounds = GRect(0, 0, bounds.size.w, 2000);

  // Initialize the scroll layer
  s_scroll_layer = scroll_layer_create(GRect(0, 55, bounds.size.w, 98));

  // This binds the scroll layer to the window so that up and down map to scrolling
  // You may use scroll_layer_set_callbacks to add or override interactivity
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  
  // Initialize the social messages scrolling text layer
  s_text_layer = text_layer_create(max_text_bounds);
  text_layer_set_text(s_text_layer, s_scroll_text);
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

  // Trim text layer and scroll content to fit text box
  GSize max_size = text_layer_get_content_size(s_text_layer);
  text_layer_set_size(s_text_layer, max_size);
  scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, max_size.h + 4));

  // Add the layers for display
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));

  // Set the text of the application name layer
  text_layer_app_name = text_layer_create(GRect(0, 0, bounds.size.w, 20));
  text_layer_set_text(text_layer_app_name, "UbiSwim.org");
  text_layer_set_text_alignment(text_layer_app_name, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_app_name));

  // Set the text of the social likes layer
  text_layer_likes = text_layer_create(GRect(0, 20, bounds.size.w, 40));
  text_layer_set_font(text_layer_likes, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD)); 
  text_layer_set_text(text_layer_likes, "Likes:");
  text_layer_set_text_alignment(text_layer_likes, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_likes));

  // Set the text of the info layer
  text_layer_msg = text_layer_create(GRect(0, 150, bounds.size.w, 30));
  text_layer_set_text(text_layer_msg, "U:Prev M:Like D:Next");
  text_layer_set_text_alignment(text_layer_msg, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_msg));

  // Set the click config provider for the scrolling text layer
  scroll_layer_set_callbacks(s_scroll_layer, (ScrollLayerCallbacks) {
        .click_config_provider = &click_config_provider_updown
      });

  // 1st update of likes and social text layers
  update_likes();
  update_social();
 }

// Select BT click handler
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Select BT");
}

// Select BT click config provider
static void click_config_provider_updown(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

// Update the likes displayed counter
static void update_likes() {
  snprintf(likes_str, 20, "Likes:%d", likes_int);
  text_layer_set_text(text_layer_likes, likes_str);
}

// Update the displayed social messages
static void update_social() {
  text_layer_set_text(s_text_layer, s_scroll_text);
}

// Destroy UI on window unload (when the user clicks the back button on Pebble) to free up memory
static void window_unload(Window *window) {
  text_layer_destroy(text_layer_app_name);
  text_layer_destroy(text_layer_likes);
  text_layer_destroy(text_layer_msg);
  text_layer_destroy(s_text_layer);
  scroll_layer_destroy(s_scroll_layer);
  window_destroy(window);
}

// Create the social screen UI
void show_social(int likes, char * social) {
  likes_int = likes;
  strcpy(s_scroll_text, social);
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = false;
  window_stack_push(window, animated);
}