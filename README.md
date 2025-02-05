# EventBus

## Опис

**EventBus** – це бібліотека для асинхронної обробки подій, орієнтована на системи з обмеженими ресурсами (ESP-IDF, FreeRTOS) але підтримує і Wandows (поки що не підтримує Posix). Вона дозволяє публікувати події, обробляти їх в окремому потоці та викликати callback‑функції підписників згідно з заданим пріоритетом.

Подія публікуєтся в EventBus, в середині EventBus`а працює свій Thread який викликає callback‑функції підписників (за їхнім пріоритетом) які підписані на тип цієї події. При цьому callback‑функції підписників також можуть повертати данні за допомогою інших callback‑функцій. Наприклад в підписник як callback передаємо функцію читання тіла http запиту а як callback функцію "відповіді" передаємо функцію відправки http відповіді, і відповідно підписник зможе прочитати данні запряму з http запиту і напряму відправити http відповідь.

Бібліотека створена для того щоб розвязати прямі залежності між компонентами.

Основні можливості:
- **Асинхронна обробка подій.** Публікація подій не блокує основний потік – події обробляються окремим потоком.
- **Система підписників.** Підписники реєструються на певні типи подій із зазначенням пріоритету. Всі підписники зберігаються у двосторонньому зв’язаному списку, де перший елемент (sub_head) завжди має найвищий пріоритет.
- **Wildcard-підписка.** Якщо підписник реєструється з типом (category==0) або (id==0), він отримує всі події певної категорії або всі події.
- **Передача даних через callback.** Для подій можна передавати дані за допомогою двох callback‑функцій:
  - `size_fn`: повертає загальний розмір даних,
  - `read_fn`: зчитує дані у буфер.
- **Відписка.** При підписці повертається вказівник на структуру підписника, який використовується для відписки через `eventbus_unsubscribe`.

## Як це працює

1. **Ініціалізація EventBus.**  
   Використовуйте структуру `EventBusConfig` для задання розміру черги подій та максимальної кількості підписників. Функція `eventbus_init` виділяє необхідну пам’ять, ініціалізує м’ютекси та створює потік обробки подій.

2. **Підписка на події.**  
   Функція `eventbus_subscribe` додає нового підписника у двосторонній зв’язаний список, впорядкований за пріоритетом. Повертається вказівник на структуру `EventSubscriber`, який використовується для подальшої відписки. Підписатись можна на конкретну подію, на будь які події конкретної групи, або на абсолютно всі події.

3. **Обробка подій.**  
   Подія публікується за допомогою `eventbus_publish` і додається до черги. Потік обробки подій бере подію з черги та обходить список підписників, починаючи з `sub_head` (найвищий пріоритет), викликаючи callback‑функції для тих, хто відповідає типу події (з урахуванням wildcard‑правил).

4. **Відписка від подій.**  
   Використовуйте функцію `eventbus_unsubscribe`, передаючи вказівник на підписника, щоб видалити його зі списку (слот буде позначено як вільний).

## Приклад використання

```c
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
  EventResultData result = create_event_result();
  // Публікуємо події типу (1,1)
  eventbus_publish(bus, event_type(1, 1), input1, result);
  eventbus_publish(bus, event_type(1, 1), input2, result);

  TASK_DELAY(1); // Чекаємо для обробки події
  // Відписка від події
  eventbus_unsubscribe(bus, sub);
  eventbus_stop(bus);
  return 0;
}
