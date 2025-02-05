#include "eventbus.h"
#include <stdio.h>
#include <string.h>

// Callback для отримання розміру даних
int my_data_size(void *context)
{
  const char *data = "Hello, EventBus!";
  return (int)strlen(data);
}

// Callback для зчитування даних у буфер
int my_data_read(void *context, void *buffer, size_t size)
{
  const char *data = "Hello, EventBus!";
  int len = (int)strlen(data);
  int to_copy = (len < (int)size) ? len : (int)size;
  memcpy(buffer, data, to_copy);
  return to_copy;
}

// Callback підписника
void my_event_callback(Event *evt, void *ctx)
{
  // todo попрацювати над зручнішим інтерфейсом зчитування данних
  char buf[64];
  int total_size = evt->input.size_fn ? evt->input.size_fn(evt->input.context) : 0;
  int n = 0;
  if (evt->input.read_fn)
    n = evt->input.read_fn(evt->input.context, buf, sizeof(buf) - 1);
  else if (evt->input.direct_data)
    n = (evt->input.data_size < sizeof(buf) - 1) ? evt->input.data_size : sizeof(buf) - 1;
  buf[n] = '\0';
  printf("Subscriber [%s] received an event: %s (total size: %d)\n", (char *)ctx, buf, total_size);
}

int main()
{
  EventBus *bus = eventbus_create(eventbus_default_config());

  // Підписка на події типу (1,1)
  EventSubscriber *sub = eventbus_subscribe(bus, event_type(1, 1), 0, "Subscribe A", my_event_callback);
  if (!sub)
  {
    printf("Помилка підписки\n");
    eventbus_stop(bus);
    return -1;
  }
  // Формуємо вхідні дані з callback для даних
  EventInputData input1 = create_event_input_callback(my_data_read, my_data_size);
  // Формуємо вхідні дані з стрічки для даних
  EventInputData input2 = create_event_input_str("Test text");
  // Формуємо функцію яка ігнорує результати обробки
  EventResultData result = create_event_result_devnull();
  // Публікуємо події типу (1,1)
  eventbus_publish(bus, event_type(1, 1), input1, result);
  eventbus_publish(bus, event_type(1, 1), input2, result);

  TASK_DELAY(10000); // Чекаємо для обробки події
  // Відписка від події
  eventbus_unsubscribe(bus, sub);
  eventbus_stop(bus);
  return 0;
}