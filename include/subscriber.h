#pragma once

#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <stdint.h>
#include "event.h"

typedef struct Subscriber Subscriber;
typedef EventStatus (*SubscriberCallback)(Event *event);

struct Subscriber
{
  EventType event_type;
  uint8_t priority;
  SubscriberCallback callback;
};

Subscriber *subscriber_create(EventType event_type, uint8_t priority, SubscriberCallback callback = NULL);
void subscriber_destroy(Subscriber *subscriber);

#endif // SUBSCRIBER_H
