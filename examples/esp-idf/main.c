#include "eventbus.h"
#include <stdio.h>

#define EVENT_TYPE_WIFI 1
#define EVENT_WIFI_SUBTYPE_SCAN 1
#define EVENT_WIFI_SUBTYPE_CONNECT 2
#define EVENT_WIFI_SUBTYPE_DISCONNECTED 3
#define EVENT_WIFI_SUBTYPE_CHANGE_CONFIG 4

// Простий обробник подій
event_status_t wifi_event_handler(void *context, void *data, size_t len)
{
  printf("WiFi Event: Type=%d, Subtype=%d\n", event->type, event->subtype);
  return EVENT_OK;
}

void app_main(void)
{
  eventbus_configure_t config = {
      .max_subscribers = 10,
      .max_events = 10,
  };

  eventbus_t *bus = eventbus_create(config);

  eventbus_subscriber_config_t subscriber_config = {
      base_type = 0,
      subtype = 0,
      callback_context_data = NULL,
      callback_function = wifi_event_handler,
  }

  Subscriber *wifi_subscriber = subscriber_create(EVENT_TYPE_WIFI, EVENT_SUBTYPE_ANY, 0, wifi_event_handler);
  eventbus_register(bus, wifi_subscriber);

  Event *wifi_event = event_create(1, 1, NULL, 0);
  eventbus_publish(bus, wifi_event);

  event_destroy(wifi_event);
  subscriber_destroy(wifi_subscriber);
  eventbus_destroy(bus);

  return 0;
}
