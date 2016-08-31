// UbiSwim pebble application

#include <pebble.h>
#include <common.h>
#include <social.h>
#include <pool.h>
#include <splash.h>
#include <score.h>

// Accelerometer tuning constants 
#define ACCEL_SAMPLING_RATE ACCEL_SAMPLING_10HZ
#define ACCEL_SAMPLES_PER_CALLBACK 1
#define ACCEL_THRESHOLD 180
#define ACCEL_DURATION 35

// Compass tuning constants
#define COMPASS_DURATION 40 // 40degreeValues / 4degreeValuesPerSec = 10sec (to detect direction change)
#define COMPASS_ALPHA 0.02 // Used in the Low Pass Filter calculation

// Persistent memory keys
#define WORKOUT_ID_PKEY 0
#define STATE_PKEY 1
#define ELAPSED_TIME_PKEY 2
#define STROKES_PKEY 3
#define LAPS_PKEY 4
#define LIKES_PKEY 5
#define SOCIAL_PKEY 6
#define SWOLF_PREV_PKEY 7

// AppMessage Keys
#define WORKOUT_ID_KEY 0
#define DURATION_KEY 1
#define STROKES_KEY 2
#define LAPS_KEY 3
#define LIKES_KEY 4
#define SOCIAL_KEY 5
#define DISTANCE_KEY 6
#define POOL_KEY 7
#define SWOLF_KEY 8
#define SSI_KEY 9

// Initialize text area on social screen
#define SOCIAL_INIT_STR "Well, there are no messages received yet. Keep going and I'' ll vibe you when something comes up even while you swim!"

// Used for stopwatch persistent memory managment
struct StopwatchState {
  double elapsed_time;
  int strokes;
  int laps;
  int swolf_avg;
  int pool;
} __attribute__((__packed__));

// Application's main screen UI (counters screen)
static Window *window;
static TextLayer *text_layer_app_name;
static TextLayer *text_layer_elapsed_time;
static TextLayer *text_layer_strokes;
static TextLayer *text_layer_laps;
static TextLayer *text_layer_distance;
static TextLayer *text_layer_msg;
static TextLayer *text_layer_m;
static TextLayer *text_layer_swolf_avg;

// Timer variables
static AppTimer* update_timer = NULL;
static double elapsed_time = 0;
static double lap_time = 0;
static double lap_start_time = 0;
static double start_time = 0;
static double pause_time = 0;
static double interval = 0;
static char elapsed_time_str[12] = "00:00:00.00";
static bool started = false;

// SWOLF variables
static char swolf_avg_str[12] = "SWOLF: 00";

// Accelerometer variables
static char strokes_str[] = "           ";
static int strokes_int = 0;
static int root_sum_of_squares = 0;
static int stroke_duration = 0;

// Compass direction change detection variables
static int degrees;            // compass degrees
static int degreesAvg = -1;    // average of all degrees captured so far, set to -1 as a workout start flag
static int degreesSum = 0;     // sum of all degrees captures during workout
static int degreesCnt = 0;     // count of degrees captures during workout
static int direction1 = 0;     // used to track direction change
static int direction2 = 0;     // used to track direction change

// Workout counters
static int lap = 0;            // workout lap counter
static int distance = 0;       // workout distance
static int strokes_of_lap = 0; // lap strokes
static int swolf = 0;          // lap SWOLF
static int swolf_avg = 0;      // average of current workout laps' SWOLF score
static int swolf_avg_prev = 0; // average of latest workout laps' SWOLF score
static int ssi = 0;            // SWOLF score percentage improvement (current vs last lap)

// Low pass filter flag
static int lowPassFilter = -1; // -1 disables the filter

// Social interaction variables
static int likes = 0;
static char social[2000];

// Workout ID
static char workout_id_str[20] = "2016-01-01 00:00:00"; // The uniquie ID of the workout. Example: 20160726205015 (date format: YYYYMMDDHHMMSS)

// Pool variables
static int pool = 0;

// Function prototypes
static void start_stopwatch();
static void stop_stopwatch();
static void start_accelerometer();
static void stop_accelerometer();
static void start_compass();
static void stop_compass();
static void timer_handler(void*);
static void start_accelerometer();
static void accelerometer_handler(void*, uint32_t);
static void compass_handler(CompassHeadingData);
static void send_data();
static void update_distance();
static void update_swolf_avg();


// Functions inplementation

static void start_stopwatch() {
  vibes_short_pulse();
  // Set a timer handler (it calls the timer_handler function in 100ms)
  update_timer = app_timer_register(100, timer_handler, NULL);
}

static void stop_stopwatch() {
  vibes_long_pulse();
  app_timer_cancel(update_timer);
}

static void start_accelerometer() {
  accel_service_set_sampling_rate( ACCEL_SAMPLING_RATE );
  accel_data_service_subscribe( ACCEL_SAMPLES_PER_CALLBACK, (AccelDataHandler) accelerometer_handler );
}

static void stop_accelerometer() {
  accel_data_service_unsubscribe();
}

static void start_compass() {
  compass_service_subscribe(compass_handler);
  compass_service_set_heading_filter(TRIG_MAX_ANGLE / 72); // 360 / 72 = 5 degrees (diff to trigger compass read)
}

static void stop_compass() {
  compass_service_unsubscribe();
}

static void update_strokes() {
  snprintf(strokes_str, 20, "strokes:%d", strokes_int);
  text_layer_set_text(text_layer_strokes, strokes_str);
}

static void update_laps() {
  static char s_buffer_lap[10];
  snprintf(s_buffer_lap, sizeof(s_buffer_lap), "laps:%d", lap);
  text_layer_set_text(text_layer_laps, s_buffer_lap);
}

static void update_distance() {
  static char s_buffer_dist[10];
  snprintf(s_buffer_dist, sizeof(s_buffer_dist), "%d", distance);
  text_layer_set_text(text_layer_distance, s_buffer_dist);
}

static void update_swolf_avg() {
  static char s_buffer_swolf_avg[10];
  snprintf(s_buffer_swolf_avg, sizeof(s_buffer_swolf_avg), "SWOLF: %d", swolf_avg);
  text_layer_set_text(text_layer_swolf_avg, s_buffer_swolf_avg);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  show_score(ssi, &swolf_avg_prev);
}

static void deinit(void) {

  // Store workout data to persistent memory on exit

  persist_write_string(WORKOUT_ID_PKEY, workout_id_str);

  struct StopwatchState state = (struct StopwatchState) {
      .elapsed_time = elapsed_time,
      .strokes = strokes_int,
      .laps = lap,
      .swolf_avg = swolf_avg,
      .pool = pool
  };

  persist_write_data(STATE_PKEY, &state, sizeof(state));
  persist_write_int(LIKES_PKEY, likes);
  persist_write_string(SOCIAL_PKEY, social);
  
  window_stack_pop_all(true);

}

// Long press the middle button to end the workout, save the data and exit the watchapp
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!started) {
    
    // Save SWOLF avg for future SSI calculation
    if (swolf_avg > 0) {
      swolf_avg_prev = swolf_avg;
      persist_write_int(SWOLF_PREV_PKEY, swolf_avg_prev);
    }

    //stop accelerometer & compass logging
    stop_accelerometer();
    stop_compass();
    stop_stopwatch();
    started = false;
    pause_time = float_time_ms();

    // Initialize counters
    strokes_int = 0;
    lap = 0;
    distance = 0;
    start_time = 0;
    lap_start_time = 0;
    elapsed_time = 0;
    lap_time = 0;
    pause_time = 0;
    likes = 0;
    swolf = 0;
    swolf_avg = 0;
    pool = 0;

    strcpy(social, SOCIAL_INIT_STR);

    update_elapsed_time(elapsed_time, elapsed_time_str);
    text_layer_set_text(text_layer_elapsed_time, elapsed_time_str);

    update_strokes();
    update_laps();  
    update_distance();
    update_swolf_avg();

    // Clear the date_time_str because this workout has been completed!
    memset(workout_id_str, 0, sizeof(workout_id_str));

    deinit();
  }  
}

// Single click the up button to start/pause the stopwatch
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!started) {
    if (start_time == 0) {
       start_time = float_time_ms();
       lap_start_time = start_time;
     } else {
        if (pause_time != 0) {
          interval = float_time_ms() - pause_time;
          start_time += interval;
          lap_start_time += interval;
        }
     }    
    start_stopwatch();
    start_accelerometer();
    start_compass();
    started = true;
  } else {
    stop_accelerometer();
    stop_compass();
    stop_stopwatch();
    started = false;
    pause_time = float_time_ms();
  }
}

// Send data over bluetooth to the smartphone mobile companion application
static void send_data() {
  text_layer_set_text(text_layer_msg, "Sending data...");

  DictionaryIterator *iter;

  // Create a message with collected workout data
  app_message_outbox_begin(&iter);

  const char * duration_str = "";
  duration_str = elapsed_time_str;


  // Add the data to the message

  dict_write_cstring(iter, WORKOUT_ID_KEY, workout_id_str);
  dict_write_cstring(iter, DURATION_KEY, duration_str);
  dict_write_int(iter, STROKES_KEY, &strokes_int, sizeof(int), true);
  dict_write_int(iter, LAPS_KEY, &lap, sizeof(int), true);
  dict_write_int(iter, LIKES_KEY, &likes, sizeof(int), true);

  // Send friends messages only if we received ones!
  if (likes > 0) {
    dict_write_cstring(iter, SOCIAL_KEY, social);
  } else {
    dict_write_cstring(iter, SOCIAL_KEY, "");
  }

  dict_write_int(iter, DISTANCE_KEY, &distance, sizeof(int), true);
  dict_write_int(iter, POOL_KEY, &pool, sizeof(int), true);
  dict_write_int(iter, SWOLF_KEY, &swolf_avg, sizeof(int), true);
  dict_write_int(iter, SSI_KEY, &ssi, sizeof(int), true);


  // Transmit the message!
  app_message_outbox_send();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Start the workout data collection and transmission process
  send_data();
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Display the social interraction screen
  show_social(likes, social);
}

// Set the buttons click handler functions
static void click_config_provider(void *context) {
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

// Set a repeated timer handler of 100ms interval to log some periods
static void timer_handler(void* data) {
  if (started) {
    double now = float_time_ms();
    elapsed_time = now - start_time;
    lap_time = now - lap_start_time;
    update_timer = app_timer_register(100, timer_handler, NULL); // Recursively calls itself after every 100ms
  }
  update_elapsed_time(elapsed_time, elapsed_time_str);
  text_layer_set_text(text_layer_elapsed_time, elapsed_time_str);
}

// Implementation of swimming strokes detection / counting algorithm
static void accelerometer_handler(void * data, uint32_t num_samples)
{
  // AppMessageResult result;
  AccelData * vector = (AccelData*) data;

  // counting strokes --> start  
  //
  // Determine if the sample occured during vibration
  bool did_vibrate = vector->did_vibrate;

  // if no, then do the math
  if (!did_vibrate) {
    
    // Calculate the acceleration of the swimmer's wrist
    root_sum_of_squares = mySqrtf(vector->x*vector->x + vector->y*vector->y + vector->z*vector->z);
    
    // Check if this acceleration if above a threshold
    if (abs(1000 - root_sum_of_squares) > ACCEL_THRESHOLD) {
      // and if yes, log for how long!
      stroke_duration++;
    }

    // OK, we have a new swimming stroke here! Log it!
    if (stroke_duration == ACCEL_DURATION) {
      strokes_int += 2; // increase total strokes by 2(hands)
      update_strokes();
      stroke_duration = 0;
      strokes_of_lap += 2; // increase strokes of current lap to calculate the SWOLF score of the lap
      // APP_LOG(APP_LOG_LEVEL_INFO, ">>stroke_duration: %d", stroke_duration);
    }

  }
  //
  // counting strokes --> end
  
}

// Lap counting detection (direction change) algorithm implementation
static void compass_handler(CompassHeadingData data) {

    int degreesAvgDiff;
    
    degrees = TRIGANGLE_TO_DEG((int)data.true_heading);
    
    if (lowPassFilter == -1) {
      lowPassFilter = degrees;
    } else {;
      lowPassFilter = lowPassFilter + (COMPASS_ALPHA * (degrees - lowPassFilter) + 0.5); // +0.5 to round up!
    }
    
    degreesCnt++;
    if (degreesAvg == -1) {
      degreesSum = degrees;
      degreesAvg = degrees;
    } else {
      degreesSum = degreesSum + degrees;
      degreesAvg = ( (degreesSum) / degreesCnt ) + 0.5; // +0.5 to round up (degreesAvg gets the int value!)
    }
    
    degreesAvg = degreesAvg - COMPASS_ALPHA * degreesAvg; // Apply low pass filter to avg (minus used here to lower the avg graph!)

    degreesAvgDiff = degreesAvg - lowPassFilter; // Calc diff to check current graph status

    if (degreesAvgDiff >= 0) { // positive values: avg graph below lowpass graph
      direction1++;
      direction2 = 0;
    } else {                   // negative values: avg graph above lowpass graph
      direction2++;
      direction1 = 0;
    }
    
    if (direction1 == COMPASS_DURATION || direction2 == COMPASS_DURATION) {
      lap++;
      distance = lap * pool;
      swolf = pool + (int)lap_time % 60;
      if (pool == 50) {
        // Dividing SWOLF score by 2, for accurate SSI calculations
        // Always doing the math on a 25m pool SWOLF score basis so as to be able to
        // correctly calculate the total SWOLF score average of the swimmer's workouts
        // and get accurate SSI metrics!
        swolf = (int)(swolf / 2);
      }
      
      if (lap == 1) {
        swolf_avg = 0;
      } else {
        if (lap == 2) {
          swolf_avg = swolf;
        } else {
          swolf_avg = (int)((swolf_avg + swolf) / 2);  
        }
      }

      if (lap > 1 && swolf_avg_prev > 0) {
        double swolf_avg_d = swolf_avg;
        double swolf_avg_prev_d = swolf_avg_prev;
        ssi = 100 - (((swolf_avg_d / swolf_avg_prev_d) * 100) + 0.5); // 0.5 to round up
        if (ssi < 0) {
          ssi = 0;
        }
      }

      // APP_LOG(APP_LOG_LEVEL_INFO, ">>lap_time:%d swolf:%d swolf_avg:%d swolf_avg_prev:%d ssi:%d", (int)lap_time % 60, swolf, swolf_avg, swolf_avg_prev, ssi);        

      strokes_of_lap = 0;
      lap_time = 0;
      lap_start_time = float_time_ms();

      // Send data to the android compation app (and from there to the web service), to track the workout in real time!
      send_data();
    }
    
    update_laps();
    update_distance();
    update_swolf_avg();

    // APP_LOG(APP_LOG_LEVEL_INFO, "d:%d c:%d s:%d l:%d a:%d df:%d d1:%d d2:%d lp:%d", degrees, degreesCnt, degreesSum, lowPassFilter, degreesAvg, degreesAvgDiff, direction1, direction2, lap); 

}

// Create main app interface
static void window_load(Window *window) {

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer_app_name = text_layer_create(GRect(0, 0, bounds.size.w, 16));
  text_layer_set_text(text_layer_app_name, "UbiSwim.org");
  text_layer_set_text_alignment(text_layer_app_name, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_app_name));

  text_layer_elapsed_time = text_layer_create(GRect(0, 20, bounds.size.w, 28));
  text_layer_set_font(text_layer_elapsed_time, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text(text_layer_elapsed_time, elapsed_time_str);
  text_layer_set_text_alignment(text_layer_elapsed_time, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_elapsed_time));

  text_layer_distance = text_layer_create(GRect(0, 50, 120, 42));
  text_layer_set_font(text_layer_distance, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD)); 
  text_layer_set_text(text_layer_distance, "0");
  text_layer_set_text_alignment(text_layer_distance, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_distance));

  text_layer_m = text_layer_create(GRect(120, 70, 140, 18));
  text_layer_set_font(text_layer_m, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)); 
  text_layer_set_text(text_layer_m, "m");
  text_layer_set_text_alignment(text_layer_m, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_m));

  text_layer_swolf_avg = text_layer_create(GRect(0, 95, 120, 28));
  text_layer_set_font(text_layer_swolf_avg, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text(text_layer_swolf_avg, swolf_avg_str);
  text_layer_set_text_alignment(text_layer_swolf_avg, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_swolf_avg));

  text_layer_strokes = text_layer_create(GRect(8, 135, 90, 16));
  // text_layer_set_font(text_layer_strokes, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD)); 
  text_layer_set_text_alignment(text_layer_strokes, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_strokes));

  text_layer_laps = text_layer_create(GRect(90, 135, 50, 16));
  // text_layer_set_font(text_layer_laps, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD)); 
  text_layer_set_text_alignment(text_layer_laps, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_laps));

  text_layer_msg = text_layer_create(GRect(0, 150, bounds.size.w, 16));
  text_layer_set_text(text_layer_msg, "U:Start M:Score D:Send");
  text_layer_set_text_alignment(text_layer_msg, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_msg));

  // Update counter values on screen
  update_strokes();
  update_laps();
  update_distance();
  update_swolf_avg();

}

// Destroy layers to free up memory
static void window_unload(Window *window) {
  text_layer_destroy(text_layer_app_name);
  text_layer_destroy(text_layer_elapsed_time);
  text_layer_destroy(text_layer_strokes);
  text_layer_destroy(text_layer_laps);
  text_layer_destroy(text_layer_msg);
}

static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
  // Succesful transmission
  text_layer_set_text(text_layer_msg, "Data succesfully sent!");
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  // Failed transmission
  text_layer_set_text(text_layer_msg, "Send failed!");
  // APP_LOG(APP_LOG_LEVEL_ERROR, "Fail reason: %d", (int)reason);
}

// Receiving message from smartphone mobile companion application
static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  // A new message has been successfully received

  // Read the friends name string
  int AppKeyFriendName = 0;
  Tuple *friendName_tuple = dict_find(iter, AppKeyFriendName);
  char *friendName = malloc(10 * sizeof(char));  
  if (friendName_tuple) {
    // This value was stored as JS String, which is stored here as a char string
     strcpy(friendName, friendName_tuple->value->cstring);
  }
 
  // Read the friends message
  int AppKeyFriendMessage = 1;
  Tuple *friendMessage_tuple = dict_find(iter, AppKeyFriendMessage);
  char *friendMessage = malloc(20 * sizeof(char));
  if (friendMessage_tuple) {
    // This value was stored as JS String, which is stored here as a char string
    strcpy(friendMessage, friendMessage_tuple->value->cstring);
  }

  // Create a temporary social_str
  char *social_str = malloc(2000 * sizeof(char));
  strcpy(social_str, "[");
  strcat(social_str, friendName);
  strcat(social_str, "]: ");
  strcat(social_str, friendMessage);
  strcat(social_str, "  ");
  
  if (likes == 0) {
    strcpy(social, social_str);  // Initialize
  } else {
    strcat(social, social_str);  // Concatenate
  }
  
  likes++;
  
  free(friendName);
  free(friendMessage);
  free(social_str);

  // Vibrate to inform the swimmer for the received "like" while working out
  vibes_double_pulse();

}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  // A message was received, but had to be dropped
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

static void init_main_ui() {
  update_elapsed_time(elapsed_time, elapsed_time_str);

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = false;
  window_stack_push(window, animated);

  // Register sent and failed appmessage handlers
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);

  // Register to be notified about appmessage inbox received and dropped events
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);

  // Open appmessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

// App initialization
static void init(void) {

  // Read workout data from persistent memory

  if (persist_exists(WORKOUT_ID_PKEY)) {
    persist_read_string(WORKOUT_ID_PKEY, workout_id_str, sizeof(workout_id_str));
    if (strlen(workout_id_str) != 19) {
      // It gets in here when a workout has been completed previously and an empty string has been saved into the WORKOUT_ID_PKEY,
      // so a new date & time string should be created for the new workout!
      createDateTimeStr(workout_id_str);  
    }
  } else {
    createDateTimeStr(workout_id_str);
  }

  struct StopwatchState state;
  if (persist_read_data(STATE_PKEY, &state, sizeof(state)) != E_DOES_NOT_EXIST) {
    elapsed_time = state.elapsed_time;
    strokes_int = state.strokes;
    lap = state.laps;
    swolf_avg = state.swolf_avg;
    pool = state.pool;
    start_time = float_time_ms() - elapsed_time;
    pause_time = float_time_ms() ;
    interval = elapsed_time;
  } else {
    elapsed_time = 0;
    strokes_int = 0;
    lap = 0;
  }

  if (persist_exists(LIKES_PKEY)) {
    likes = persist_read_int(LIKES_PKEY);
  } else {
    likes = 0;
  }

  if (persist_exists(SOCIAL_PKEY)) {
    persist_read_string(SOCIAL_PKEY, social, sizeof(social));
  } else {
    strcpy(social, SOCIAL_INIT_STR);
  }

  if (persist_exists(SWOLF_PREV_PKEY)) {
    swolf_avg_prev = persist_read_int(SWOLF_PREV_PKEY);
    // APP_LOG(APP_LOG_LEVEL_INFO, ">>persist_read(SWOLF_PREV_PKEY): %d", swolf_avg_prev);  
  } else {
    swolf_avg_prev = 0;
  }

  distance = pool * lap;

  init_main_ui();

  // It's a new workout
  if (pool == 0) {
    // so display the pool screen to select the pool size
    show_pool(&pool);
  }

  // But first display the splash screen with the UbiSwim logo :-)
  show_splash();

}

// Called on app launch
int main(void) {
  init();
  app_event_loop();
  deinit();
}