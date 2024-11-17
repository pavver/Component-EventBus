#pragma once

#ifndef EVENTBUS_H
#define EVENTBUS_H

#include "event.h"
#include "subscriber.h"

typedef struct EventBus EventBus;

// Створення EventBus
EventBus *eventbus_create(void);

// Знищення EventBus
void eventbus_destroy(EventBus *bus);

// Реєстрація підписників
int eventbus_register(EventBus *bus, Subscriber *subscriber);

int eventbus_unregister(EventBus *bus, Subscriber *subscriber);

// Відправлення події
int eventbus_publish(EventBus *bus, Event *event);
int eventbus_request(EventBus *bus, Event *request, void *response, size_t response_size);

#endif // EVENTBUS_H