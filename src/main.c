#include "mgos.h"
#include "mgos_mqtt.h"

/*
 * Global variables
 */
static bool mqtt_connected = false;
static mgos_timer_id health_check_timer = 0;

/**
 * Callback for led looping
 */
static void led_timer_cb(__attribute__((unused)) void *arg) {
  mgos_gpio_toggle(mgos_sys_config_get_app_pins_led());
}

static void start_blinking() {
  if (health_check_timer != 0) {
    return;
  }

  health_check_timer = mgos_set_timer(300, MGOS_TIMER_REPEAT, led_timer_cb, NULL);
}

static void stop_blinking() {
  mgos_clear_timer(health_check_timer);
  mgos_gpio_write(mgos_sys_config_get_app_pins_led(), true);
}

/**
 * Callback for button press
 */
static void button_cb(__attribute__((unused)) int pin, __attribute__((unused)) void *arg) {
  char topic[100];
  char *message = "{\"event\":\"button_press\"}";

  snprintf(topic, sizeof(topic), "devices/%s/events/toggl-button",
           mgos_sys_config_get_device_id());

  // Publishes the event to MQTT
  bool res = mgos_mqtt_pub(topic, message, strlen(message), 1, false);

  // Logs the result
  LOG(LL_INFO, ("Button pressed - Published: %s", res ? "yes" : "no"));
}

/**
 * Get a formatted uptime string
 */
static void get_formatted_uptime(char *string, size_t size) {
  int seconds = (int) mgos_uptime();
  int days = seconds / 86400;
  seconds = seconds % 86400;
  int hours = seconds / 3600;
  seconds = seconds % 3600;
  int minutes = seconds / 60;
  seconds = seconds % 60;

  snprintf(string, size, "%d days, %.2d:%.2d:%.2d",
           days, hours, minutes, seconds);
}

/**
 * Health check function
 *
 * This function will check for the vital signs of the system and if something
 * is not right, it will make the led blink
 */
static void health_check(__attribute__((unused)) void *arg) {
  bool wifi_connected = mgos_wifi_get_status() == MGOS_WIFI_IP_ACQUIRED;
  char uptime[50];

  get_formatted_uptime(uptime, sizeof(uptime));

  LOG(LL_INFO, ("Uptime: %s | Wi-Fi: %s | MQTT: %s | RAM: %lu, %lu free", 
        uptime,
        wifi_connected ? "Yes" : "No", 
        mqtt_connected ? "Yes" : "No",
        (unsigned long) mgos_get_heap_size(),
        (unsigned long) mgos_get_free_heap_size()));

  if (!wifi_connected || !mqtt_connected) {
    // Start blinking the LED to indicate that something is wrong
    start_blinking();
  } else {
    stop_blinking();
  }
}

/**
 * Callback run every time a MQTT event happen
 */
static void mqtt_event_cb(__attribute__((unused)) struct mg_connection *conn, 
    __attribute__((unused)) int ev, __attribute__((unused)) void *ev_data, 
    __attribute__((unused)) void *user_data) {
  if (ev == MG_EV_CLOSE) {
    mqtt_connected = false;
    health_check(NULL);
  } else if (ev == MG_EV_MQTT_CONNACK) {
    mqtt_connected = true;
    health_check(NULL);
  }
}

/**
 * Callback run every time a Wi-Fi event happen
 */
static void wifi_event_cb(__attribute__((unused)) enum mgos_wifi_status ws, __attribute__((unused)) void *arg) {
  health_check(NULL);
}

/**
 * Init (main) function
 */
enum mgos_app_init_result mgos_app_init(void) {
  // Set the onboard LED pin to output mode
  mgos_gpio_set_mode(mgos_sys_config_get_app_pins_led(), MGOS_GPIO_MODE_OUTPUT);

  // Set a timer to health check function
  mgos_set_timer(1000, MGOS_TIMER_REPEAT, health_check, NULL);

  // Set a callback for MQTT events
  mgos_mqtt_add_global_handler(mqtt_event_cb, NULL);

  // Set a callback for WIFI events
  mgos_wifi_add_on_change_cb(wifi_event_cb, NULL);

  // Publish to MQTT on button press
  mgos_gpio_set_button_handler(mgos_sys_config_get_app_pins_button(),
                               MGOS_GPIO_PULL_UP, MGOS_GPIO_INT_EDGE_NEG, 200,
                               button_cb, NULL);

  return MGOS_APP_INIT_SUCCESS;
}
