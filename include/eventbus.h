#ifndef EVENTBUS_H
#define EVENTBUS_H

#include "event.h"
#include "subscriber.h"

typedef struct EventBus EventBus;

// Створення та знищення EventBus
EventBus *eventbus_create(void);
void eventbus_destroy(EventBus *bus);

// Реєстрація/відписка підписників
int eventbus_register(EventBus *bus, Subscriber *subscriber);
int eventbus_unregister(EventBus *bus, Subscriber *subscriber);

// Відправлення події
int eventbus_publish(EventBus *bus, Event *event);
int eventbus_request(EventBus *bus, Event *request, void *response, size_t response_size);

#endif // EVENTBUS_H