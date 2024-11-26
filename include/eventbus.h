#pragma once

#ifndef EVENTBUS_H
#define EVENTBUS_H

#include "event.h"
#include "subscriber.h"

typedef struct EventBus EventBus;

typedef struct eventbus_config_t
{
  uint8_t max_subscribers;
  uint8_t events_queue_len;
} eventbus_config_t;

#define EVENTBUS_DEFAULT_CONFIG ((struct eventbus_configure_t){ \
    .max_subscribers = 20,                                      \
    .events_queue_len = 20,                                     \
})

EventBus *eventbus_create();
EventBus *eventbus_create(eventbus_config_t config);

// Знищення EventBus
void eventbus_destroy(EventBus *bus);

// Реєстрація підписників
int eventbus_subscribe(EventBus *bus, Subscriber *subscriber);

int eventbus_unsubscribe(EventBus *bus, Subscriber *subscriber);

Event *eventbus_nextEvent(EventBus *bus, Subscriber *subscriber);

// Відправлення події
int eventbus_publish_message(EventBus *bus, EventType event_type);
int eventbus_publish_message(EventBus *bus, EventType event_type, EventInputData *input_data);

int eventbus_publish_request(EventBus *bus, EventType event_type);
int eventbus_publish_request(EventBus *bus, EventType event_type, EventInputData *input_data);

bool eventbus_request_existResponseData(EventBus *bus, Subscriber *subscriber, Event *event);
EventData *eventbus_awaitData(EventBus *bus, Subscriber *subscriber, Event *event);

#endif // EVENTBUS_H