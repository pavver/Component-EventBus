/**
 * @file eventbus.c
 * @brief Реалізація бібліотеки EventBus.
 */

#include "eventbus.h"
#include <stdio.h>

EventBusConfig eventbus_default_config(void)
{

  EventBusConfig config;
  config.subs_array_size = 20;
  config.queue_size = 10;

#if defined(CONFIG_IDF_TARGET)
  // esp-idf specific

  config.task_name = strdup("EventBus");
  config.task_priority = 5;
  config.task_stackSize = 4096;
  config.task_xCoreId = 0;

#elif defined(_WIN32)
  // Windows-specific

  config.task_stackSize = 4096;

#else
  // Unix-specific

#endif

  return config;
}

EventInputData create_event_input_str(const char *data)
{
  return create_event_input_data(strdup(data), strlen(data) + 1);
}

EventInputData create_event_input_data(void *data, size_t data_size)
{
  EventInputData data_ptr;
  data_ptr.read_fn = NULL;
  data_ptr.size_fn = NULL;
  data_ptr.direct_data = data;
  data_ptr.data_size = data_size;
  return data_ptr;
}

EventInputData create_event_input_callback(EventDataReadFn read_fn, EventDataSizeFn size_fn)
{
  EventInputData data_ptr;
  data_ptr.read_fn = read_fn;
  data_ptr.size_fn = size_fn;
  data_ptr.direct_data = NULL;
  data_ptr.data_size = 0;
  return data_ptr;
}

static int create_event_result_devnull_callback(void *context, void *buffer, size_t size)
{
}

EventResultData create_event_result_devnull()
{
  EventResultData readData;
  readData.context = NULL;
  readData.write_fn = create_event_result_devnull_callback;
  return readData;
}

// ==================== Робота з чергою подій ====================

/**
 * @brief Додає подію до циклічного буфера.
 *
 * @param bus Вказівник на EventBus.
 * @param evt Вказівник на подію.
 * @return 0 при успіху, -1 якщо черга переповнена.
 */
static int queue_push(EventBus *bus, Event *evt)
{
  EVENTBUS_MUTEX_LOCK(&bus->queue_mutex);
  size_t next = (bus->tail + 1) % bus->config.queue_size;
  if (next == bus->head)
  {
    EVENTBUS_MUTEX_UNLOCK(&bus->queue_mutex);
    return -1; // черга переповнена
  }
  bus->queue[bus->tail] = *evt;
  bus->tail = next;
  EVENTBUS_MUTEX_UNLOCK(&bus->queue_mutex);
  return 0;
}

/**
 * @brief Видаляє подію з циклічного буфера.
 *
 * @param bus Вказівник на EventBus.
 * @param evt Вказівник, куди буде скопійована подія.
 * @return 0 при успіху, -1 якщо черга порожня.
 */
static int queue_pop(EventBus *bus, Event *evt)
{
  EVENTBUS_MUTEX_LOCK(&bus->queue_mutex);
  if (bus->head == bus->tail)
  {
    EVENTBUS_MUTEX_UNLOCK(&bus->queue_mutex);
    return -1; // черга порожня
  }
  *evt = bus->queue[bus->head];
  bus->head = (bus->head + 1) % bus->config.queue_size;
  EVENTBUS_MUTEX_UNLOCK(&bus->queue_mutex);
  return 0;
}

// ==================== Обробка подій ====================

static int sub_next(EventBus *bus, int id, EventType type)
{
  if (id == -1)
    return -1;
  if (id == -2)
    id = bus->sub_head;
  else
  {
    bus->subs[id].status = sub_slot_used;
    id = bus->subs[id].next;
  }
  while (id != -1)
  {
    // Блокуємо subs_mutex для зчитування інформації про поточного підписника та його next.
    EventSubscriber *sub = &bus->subs[id];
    // Перевіряємо, чи відповідає тип події (з wildcard-правилами)
    if (sub->type.category == 0)
      break;
    else if (sub->type.category == type.category)
    {
      if (sub->type.id == type.id || sub->type.id == 0)
        break;
    }
    id = sub->next;
  }
  if (id != -1)
    bus->subs[id].status = sub_slot_inWork;
  return id;
}

/**
 * @brief Обробляє подію, послідовно обходячи зв'язаний список підписників.
 *
 * Виконання починається з першого підписника (bus->sub_head). Для кожного підписника,
 * якщо тип події відповідає (з урахуванням wildcard‑правил), викликається його callback.
 * При виборі наступного підписника м’ютекс subs_mutex блокується для зчитування значення поля next,
 * після чого розблокується перед викликом callback.
 *
 * @param bus Вказівник на EventBus.
 * @param evt Вказівник на подію.
 */
static void process_event(EventBus *bus, Event *evt)
{
  EVENTBUS_MUTEX_LOCK(&bus->subs_mutex);
  int id = sub_next(bus, -2, evt->type);
  EVENTBUS_MUTEX_UNLOCK(&bus->subs_mutex);

  while (id != -1)
  {
    EventSubscriber *sub = &bus->subs[id];
    sub->callback(evt, sub->context);

    if (bus->status == bus_thread_stopping)
      break;

    EVENTBUS_MUTEX_LOCK(&bus->subs_mutex);
    id = sub_next(bus, id, evt->type);
    EVENTBUS_MUTEX_UNLOCK(&bus->subs_mutex);
  }
  if (evt->input.direct_data != NULL)
  {
    free(evt->input.direct_data);
    evt->input.direct_data = NULL;
    evt->input.data_size = 0;
  }
}

// ==================== Потік обробки подій ====================
/**
 * @brief Функція потоку обробки подій.
 *
 * Потік чекає появи подій у черзі та обробляє їх.
 *
 * @param arg Вказівник на EventBus.
 * @return NULL.
 */
static THREAD_RETURN_TYPE eventbus_thread_func(THREAD_ARG_TYPE arg)
{
  EventBus *bus = (EventBus *)arg;
  bus->status = bus_thread_working;
  while (true)
  {
    Event evt;
    if (bus->status == bus_thread_stopping)
    {
      while (queue_pop(bus, &evt) == 0)
      {
        if (evt.input.direct_data == NULL)
          continue;

        free(evt.input.direct_data);
        evt.input.direct_data = NULL;
        evt.input.data_size = 0;
      }
      break;
    }
    if (queue_pop(bus, &evt) != 0)
    {
      TASK_DELAY(1);
      continue;
    }
    process_event(bus, &evt);
  }

  bus->status = bus_thread_stoped;
  return THREAD_RETURN;
}

// ==================== Ініціалізація EventBus ====================

/**
 * @brief Ініціалізує EventBus згідно з переданою конфігурацією.
 *
 * Виділяє пам’ять для черги подій та масиву підписників, ініціалізує м’ютекси,
 * умовну змінну та створює потік обробки подій.
 *
 * @param bus Вказівник на EventBus.
 * @param cfg Вказівник на конфігурацію EventBus.
 * @return 0 при успіху, -1 при помилці.
 */
int eventbus_init(EventBus *bus, EventBusConfig *cfg)
{
  bus->config = *cfg;
  bus->status = bus_thread_noStarted;
  bus->head = bus->tail = 0;
  bus->sub_head = -1;
  bus->queue = (Event *)malloc(sizeof(Event) * bus->config.queue_size);
  if (!bus->queue)
    return -1;
  bus->subs = (EventSubscriber *)malloc(sizeof(EventSubscriber) * bus->config.subs_array_size);
  if (!bus->subs)
  {
    free(bus->queue);
    return -1;
  }
  for (size_t i = 0; i < bus->config.queue_size; i++)
  {
    bus->queue[i].input.direct_data = NULL;
    bus->queue[i].input.data_size = 0;
  }
  for (size_t i = 0; i < bus->config.subs_array_size; i++)
  {
    bus->subs[i].status = sub_slot_free;
    bus->subs[i].next = -1;
    bus->subs[i].prev = -1;
  }
  EVENTBUS_MUTEX_INIT(&bus->queue_mutex);
  EVENTBUS_MUTEX_INIT(&bus->subs_mutex);

#if defined(CONFIG_IDF_TARGET)
  // esp-idf specific

  xTaskCreatePinnedToCore(eventbus_thread_func,
                          bus->config.task_name,
                          bus->config.task_stackSize,
                          bus,
                          bus->config.task_priority,
                          &(bus->thread), bus->config.task_xCoreId);

#elif defined(_WIN32)
  // Windows-specific

  bus->thread = CreateThread(
      NULL,                       // default security attributes
      bus->config.task_stackSize, // use default stack size
      eventbus_thread_func,       // thread function name
      bus,                        // argument to thread function
      0,                          // use default creation flags
      &(bus->dwThreadId));        // returns the thread identifier
#else
  // Unix-specific

#endif

  return 0;
}

EventBus *eventbus_create(EventBusConfig cfg)
{
  EventBus *bus = (EventBus *)malloc(sizeof(EventBus));
  if (eventbus_init(bus, &cfg) != 0)
  {
    free(bus);
    return NULL;
  }
  return bus;
}

/**
 * @brief Зупиняє роботу EventBus.
 *
 * Встановлює прапорець завершення, сигналізує потоку та чекає його завершення,
 * після чого звільняє всі виділені ресурси.
 *
 * @param bus Вказівник на EventBus.
 */
void eventbus_stop(EventBus *bus)
{
  bus->status = bus_thread_stopping;

  while (bus->status != bus_thread_stoped)
    TASK_DELAY(1);

  free(bus->queue);
  free(bus->subs);

#if defined(CONFIG_IDF_TARGET)
  // esp-idf specific

  free(config.task_name);

#elif defined(_WIN32)
  // Windows-specific

#else
  // Unix-specific

#endif
}

// ==================== Робота з підписниками ====================

static void remove_subscriber(EventBus *bus, int idx)
{
  // Видаляємо елемент із зв’язного списку
  int prev = bus->subs[idx].prev;
  int next = bus->subs[idx].next;
  if (prev != -1)
    bus->subs[prev].next = next;
  else
    bus->sub_head = next;
  if (next != -1)
    bus->subs[next].prev = prev;
  bus->subs[idx].next = -1;
  bus->subs[idx].prev = -1;
  bus->subs[idx].status = sub_slot_free;
}

/**
 * @brief Вставляє підписника у двосторонній зв’язаний список за зростанням priority.
 *
 * Якщо priority співпадають, новий елемент вставляється після існуючих з таким же значенням.
 *
 * @param bus Вказівник на EventBus.
 * @param idx Індекс нового підписника в масиві bus->subs.
 */
static void insert_subscriber(EventBus *bus, int idx)
{
  // Якщо список порожній, новий елемент стає першим.
  if (bus->sub_head == -1)
  {
    bus->sub_head = idx;
    bus->subs[idx].next = -1;
    bus->subs[idx].prev = -1;
    return;
  }

  int cur = bus->sub_head;
  int prev = -1;
  while (cur != -1 && bus->subs[cur].priority <= bus->subs[idx].priority)
  {
    prev = cur;
    cur = bus->subs[cur].next;
  }
  if (prev == -1)
  {
    // Вставка на початок списку
    bus->subs[idx].next = bus->sub_head;
    bus->subs[idx].prev = -1;
    bus->subs[bus->sub_head].prev = idx;
    bus->sub_head = idx;
  }
  else
  {
    // Вставка між prev та cur або в кінець
    bus->subs[idx].next = cur;
    bus->subs[idx].prev = prev;
    bus->subs[prev].next = idx;
    if (cur != -1)
      bus->subs[cur].prev = idx;
  }
}

static int sub_finde_free_slot(EventBus *bus)
{
  for (size_t i = 0; i < bus->config.subs_array_size; i++)
  {
    if (bus->subs[i].status == sub_slot_free)
    {
      return i;
    }
  }
  return -1;
}

static EventSubscriber *sub_add(EventBus *bus, EventType type, uint8_t priority, void *context, EventCallback callback)
{
  int free_slot = sub_finde_free_slot(bus);
  if (free_slot == -1)
  {
    return NULL; // немає вільного слоту
  }
  bus->subs[free_slot].status = sub_slot_used;
  bus->subs[free_slot].type = type;
  bus->subs[free_slot].priority = priority;
  bus->subs[free_slot].context = context;
  bus->subs[free_slot].callback = callback;
  bus->subs[free_slot].next = -1;
  bus->subs[free_slot].prev = -1;
  // Вставка підписника у зв’язаний список
  insert_subscriber(bus, free_slot);
  return &bus->subs[free_slot];
}

/**
 * @brief Додає нового підписника до EventBus.
 *
 * Шукає вільний слот у масиві підписників, заповнює його інформацією та вставляє у зв’язаний список.
 *
 * @param bus Вказівник на EventBus.
 * @param type Тип події для підписки.
 * @param priority Пріоритет підписника.
 * @param context Контекст підписника.
 * @param callback Callback для обробки події.
 * @return Вказівник на структуру EventSubscriber при успіху, або NULL при помилці.
 */
EventSubscriber *eventbus_subscribe(EventBus *bus, EventType type, uint8_t priority, void *context, EventCallback callback)
{
  EventSubscriber *ret = NULL;

  EVENTBUS_MUTEX_LOCK(&bus->subs_mutex);

  ret = sub_add(bus, type, priority, context, callback);

  EVENTBUS_MUTEX_UNLOCK(&bus->subs_mutex);

  return ret;
}

/**
 * @brief Видаляє підписника з EventBus.
 *
 * Видаляє елемент зі зв’язного списку та маркує слот як вільний.
 *
 * @param bus Вказівник на EventBus.
 * @param subscriber Вказівник на підписника, який потрібно видалити.
 * @return 0 при успіху, -1 якщо subscriber == NULL.
 */
int eventbus_unsubscribe(EventBus *bus, EventSubscriber *subscriber)
{
  if (!subscriber)
    return -1;
  int idx = (int)(subscriber - bus->subs); // розраховуємо індекс елемента
  if (bus->subs[idx].status == sub_slot_free)
  {
    return -1;
  }

  while (bus->subs[idx].status == sub_slot_inWork)
  {
    TASK_DELAY(1);
  }

  EVENTBUS_MUTEX_LOCK(&bus->subs_mutex);

  remove_subscriber(bus, idx);

  EVENTBUS_MUTEX_UNLOCK(&bus->subs_mutex);
  return 0;
}

/**
 * @brief Публікує подію.
 *
 * Події з type.category==0 або type.id==0 заборонені.
 *
 * @param bus Вказівник на EventBus.
 * @param type Тип події.
 * @param input Вхідні дані події.
 * @param result Дані для повернення результату.
 * @return 0 при успішній публікації, -1 при помилці.
 */
int eventbus_publish(EventBus *bus, EventType type, EventInputData input, EventResultData result)
{
  if (type.category == 0 || type.id == 0)
    return -1;

  Event evt;
  evt.type = type;
  evt.input = input;
  evt.result = result;
  return queue_push(bus, &evt);
}
