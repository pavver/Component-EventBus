#include "eventbus.h"
#include <stdio.h>

#define EVENT_TYPE_WIFI 1
#define EVENT_WIFI_SUBTYPE_SCAN 1
#define EVENT_WIFI_SUBTYPE_CONNECT 2
#define EVENT_WIFI_SUBTYPE_DISCONNECTED 3
#define EVENT_WIFI_SUBTYPE_CHANGE_CONFIG 4
#define EVENT_WIFI_SUBTYPE_CHANGE_POWEROFF 5

// Простий обробник подій
event_status_t wifi_event_handler(void *context, void *data, size_t len)
{
  printf("WiFi Event: Type=%d, Subtype=%d\n", event->type, event->subtype);
  return EVENT_OK;
}

void app_main(void)
{
  eventbus_t *bus = eventbus_create();

  Subscriber *wifi_subscriber = subscriber_create(EVENT_TYPE(EVENT_TYPE_WIFI), 0, wifi_event_handler);
  eventbus_subscribe(bus, wifi_subscriber);

  const char* connect_event_json = ",testssid,password";
  Event *wifi_event = CREATE_MESSAGE(EVENT_TYPE(EVENT_TYPE_WIFI, EVENT_WIFI_SUBTYPE_CONNECT),);
  eventbus_publish(bus, wifi_event);



  event_destroy(wifi_event);
  subscriber_destroy(wifi_subscriber);
  eventbus_destroy(bus);

  return 0;
}
