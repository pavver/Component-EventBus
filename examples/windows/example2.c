
#include "eventbus.h"
#include <stdio.h>
#include <stdlib.h>

#define EVENT_GROUP_A 1
#define EVENT_GROUP_A_TYPE_A 1
#define EVENT_GROUP_A_TYPE_B 2

#define EVENT_GROUP_B 2
#define EVENT_GROUP_B_TYPE_A 1
#define EVENT_GROUP_B_TYPE_B 2

const char *format_event_type_name(Event *event)
{
  if (event->type.category == EVENT_GROUP_A)
  {
    if (event->type.id == EVENT_GROUP_A_TYPE_A)
      return "EVENT_GROUP_A: EVENT_GROUP_A_TYPE_B";
    else if (event->type.id == EVENT_GROUP_A_TYPE_B)
      return "EVENT_GROUP_A: EVENT_GROUP_A_TYPE_B";
  }
  else if (event->type.category == EVENT_GROUP_B)
  {
    if (event->type.id == EVENT_GROUP_B_TYPE_A)
      return "EVENT_GROUP_B: EVENT_GROUP_B_TYPE_A";
    else if (event->type.id == EVENT_GROUP_B_TYPE_B)
      return "EVENT_GROUP_B: EVENT_GROUP_B_TYPE_B";
  }
  return "Unknown";
}

const char* text = "1234567890";


static int publish_callbach(void *context, void *buffer, size_t size)
{
}

void subscriber_handler_A(Event *event, void *context)
{
  if (event->input.direct_data)
    printf("handler_A. Data: %s\n", (char *)(event->input.direct_data));

  EventBus *bus = (EventBus *)context;

  EventInputData eventData = create_event_input_str(text);

  EventResultData readData;
  readData.context = NULL;
  readData.write_fn = publish_callbach;

  eventbus_publish(bus, event_type(EVENT_GROUP_B, EVENT_GROUP_B_TYPE_B), eventData, readData);
}

void subscriber_handler_B(Event *event, void *context)
{
  if (event->input.direct_data)
    printf("handler_B. Data: %s\n", (char *)(event->input.direct_data));
}

void subscriber_handler_BB(Event *event, void *context)
{
  if (event->input.direct_data)
    printf("handler_BB. Data: %s\n", (char *)(event->input.direct_data));
}

void subscriber_handler_BB1(Event *event, void *context)
{
  if (event->input.direct_data)
    printf("handler_BA. Data: %s\n", (char *)(event->input.direct_data));
}

void subscriber_handler_logger(Event *event, void *context)
{
  if (event->input.direct_data)
    printf("Event: %s. Data: %s\n", format_event_type_name(event), (char *)(event->input.direct_data));
}

int main(void)
{
  EventBus *bus = eventbus_create(eventbus_default_config());

  EventSubscriber *subscriber_A =
      eventbus_subscribe(bus, event_type(EVENT_GROUP_A, 0), 10, bus, subscriber_handler_A);

  EventSubscriber *subscriber_logger =
      eventbus_subscribe(bus, event_type(0, 0), 0, bus, subscriber_handler_logger);

  EventSubscriber *subscriber_BB =
      eventbus_subscribe(bus, event_type(EVENT_GROUP_B, EVENT_GROUP_B_TYPE_B), 50, bus, subscriber_handler_BB);

  EventSubscriber *subscriber_BB1 =
      eventbus_subscribe(bus, event_type(EVENT_GROUP_B, EVENT_GROUP_B_TYPE_A), 50, bus, subscriber_handler_BB1);

  EventSubscriber *subscriber_B =
      eventbus_subscribe(bus, event_type(EVENT_GROUP_B, 0), 10, bus, subscriber_handler_B);

  EventResultData readData;
  readData.context = NULL;
  readData.write_fn = publish_callbach;

  for (size_t i = 0; i < 1000000; i++)
  {
    int num = i % 5;

    if (num == 0)
    {
      int rnd = rand() % 2;
      EventInputData eventData = create_event_input_str(text);
      eventbus_publish(bus, event_type(EVENT_GROUP_A, rnd ? EVENT_GROUP_A_TYPE_A : EVENT_GROUP_A_TYPE_B), eventData, readData);
    }

    if (num == 1)
    {
      int rnd = rand() % 2;
      EventInputData eventData = create_event_input_str(text);
      eventbus_publish(bus, event_type(EVENT_GROUP_B, rnd ? EVENT_GROUP_B_TYPE_A : EVENT_GROUP_B_TYPE_B), eventData, readData);
    }

    if (num == 3)
    {
      EventInputData eventData = create_event_input_str(text);
      eventbus_publish(bus, event_type(EVENT_GROUP_B, EVENT_GROUP_B_TYPE_B), eventData, readData);
    }

    Sleep(5);
  }

  eventbus_stop(bus);

  return 0;
}