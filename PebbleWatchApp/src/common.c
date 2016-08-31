// common functions

#include <pebble.h>

// update the epapsed time string
void update_elapsed_time(double elapsed_time, char* elapsed_time_str) {
  int hundredths = (int)(elapsed_time * 100) % 100;
  int seconds = (int)elapsed_time % 60;
  int minutes = (int)elapsed_time / 60 % 60;
  int hours = (int)elapsed_time / 3600;

  snprintf(elapsed_time_str, 12, "%02d:%02d:%02d.%02d", hours, minutes, seconds, hundredths);
}

// Create a current date & time string
void createDateTimeStr(char *date_time_str) {
  // Clear the date_time_str char array parameter, in case it isn't already empty!
  // If you don't clear the arr and it's not empty, the new date & time string will be concatenated right
  // after the old one, resulting in a 39 characters string.
  memset(date_time_str, 0, sizeof(date_time_str));
  //
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  //
  // Get date
  static char date_buffer[12];
  strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d ", tick_time);
  //
  // Get time
  static char time_buffer[9];
  strftime(time_buffer, sizeof(time_buffer), clock_is_24h_style() ? "%H:%M:%S" : "%I:%M%S", tick_time);
  //
  // Concat date & time
  static char date_time_buffer[20];
  strncat(date_time_buffer, date_buffer, 11);
  strncat(date_time_buffer, time_buffer, 8);
  //
  // Return the newly created date & time string by reference
  strcat(date_time_str, date_time_buffer);
}

// return current time
double float_time_ms() {
	time_t seconds;
	uint16_t milliseconds;
	time_ms(&seconds, &milliseconds);

	return (double)seconds + ((double)milliseconds / 1000.0);
}

// return the square root of a float
float mySqrtf(const float x) {
  const float xhalf = 0.5f*x;
  union {
    float x;
    int i;
  } u;
  u.x = x;
  u.i = 0x5f3759df - (u.i >> 1);

  return x*u.x*(1.5f - xhalf*u.x*u.x) + 1;
}