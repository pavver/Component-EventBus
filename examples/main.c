#include "eventbus.h"
#include <stdio.h>

// Простий обробник подій
EventStatus wifi_event_handler(Event *event)
{
  printf("WiFi Event: Type=%d, Subtype=%d\n", event->type, event->subtype);
  return EVENT_OK;
}

int main(void)
{
  EventBus *bus = eventbus_create();

  Subscriber *wifi_subscriber = subscriber_create(0, 1, 0, wifi_event_handler);
  eventbus_register(bus, wifi_subscriber);

  Event *wifi_event = event_create(1, 1, NULL, 0);
  eventbus_publish(bus, wifi_event);

  event_destroy(wifi_event);
  subscriber_destroy(wifi_subscriber);
  eventbus_destroy(bus);

  return 0;
}
