#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include <stddef.h>

typedef struct Event Event;
typedef int (*EventDataReadFn)(void *buffer, size_t size);

typedef enum {
    EVENT_OK,
    EVENT_FAIL,
    EVENT_TIMEOUT
} EventStatus;

struct Event {
    uint8_t type;
    uint8_t subtype;
    void *data;
    size_t data_size;
    EventDataReadFn read_fn;
    void (*free_fn)(void *data);
    EventStatus status;
};

Event *event_create(uint8_t type, uint8_t subtype, void *data, size_t data_size);
Event *event_create_with_fn(uint8_t type, uint8_t subtype, EventDataReadFn read_fn, size_t data_size);
void event_destroy(Event *event);

#endif // EVENT_H
