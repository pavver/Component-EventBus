#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <stdint.h>
#include "event.h"

typedef struct Subscriber Subscriber;
typedef EventStatus (*SubscriberCallback)(Event *event);

struct Subscriber {
    uint8_t priority;
    uint8_t event_type;
    uint8_t event_subtype;
    SubscriberCallback callback;
};

Subscriber *subscriber_create(uint8_t priority, uint8_t event_type, uint8_t event_subtype, SubscriberCallback callback);
void subscriber_destroy(Subscriber *subscriber);

#endif // SUBSCRIBER_H
