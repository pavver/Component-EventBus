/**
 * @file eventbus.h
 * @brief Заголовочний файл для бібліотеки EventBus.
 *
 * Бібліотека реалізує асинхронну обробку подій з використанням окремого потоку,
 * двох м’ютексів для синхронізації роботи з чергою подій та списком підписників.
 */

#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "eventbus_def.h"

/**
 * @brief Конфігурація EventBus.
 */
typedef struct
{
  uint16_t queue_size;      /**< Розмір черги подій */
  uint16_t subs_array_size; /**< Максимальна кількість підписників */
  uint32_t task_stackSize;  /**< Розмір стеку для потоку */

#if defined(CONFIG_IDF_TARGET)
  // esp-idf specific

  char *task_name;
  UBaseType_t task_priority;
  BaseType_t task_xCoreId;

#elif defined(_WIN32)
  // Windows-specific

#else
  // Unix-specific

#endif

} EventBusConfig;

EventBusConfig eventbus_default_config(void);

/**
 * @brief Функція, що повертає розмір даних.
 *
 * @param context Контекст виклику.
 * @return Розмір даних.
 */
typedef int (*EventDataSizeFn)(void *context);

/**
 * @brief Функція для зчитування даних у буфер.
 *
 * @param context Контекст виклику.
 * @param buffer Буфер для зчитування.
 * @param size Максимальний розмір для зчитування.
 * @return Кількість зчитаних байтів.
 */
typedef int (*EventDataReadFn)(void *context, void *buffer, size_t size);

/**
 * @brief Функція для запису даних (результат).
 *
 * @param context Контекст виклику.
 * @param buffer Буфер з даними.
 * @param size Розмір даних.
 * @return Результат операції.
 */
typedef int (*EventDataWriteFn)(void *context, void *buffer, size_t size);
typedef int (*EventDataWriteDoneFn)(void *context);

/**
 * @brief Структура для введення даних події.
 *
 * Можна передавати дані через callback‑функції:
 * - size_fn: повертає загальний розмір даних,
 * - read_fn: зчитує дані у буфер,
 * або напряму через direct_data.
 */
typedef struct
{
  EventDataReadFn read_fn; /**< Callback для зчитування даних */
  EventDataSizeFn size_fn; /**< Callback для отримання розміру даних */
  void *context;           /**< Контекст для callback‑функцій */
  void *direct_data;       /**< Прямий вказівник на дані */
  size_t data_size;        /**< Розмір даних у direct_data */
} EventInputData;

/**
 * @brief Структура для виведення результату події.
 */
typedef struct
{
  EventDataWriteFn write_fn;    /**< Callback для запису даних (результат) */
  EventDataWriteDoneFn done_fn; /**< Callback для запису даних (результат) Викликаєтся коли всі підписники уже відпрацювали */
  void *context;                /**< Контекст для write_fn */
} EventResultData;

/**
 * @brief Тип події, що складається з категорії та id.
 *
 * Значення 0 зарезервовано для wildcard-підписників.
 */
typedef struct
{
  uint8_t category, id;
} EventType;

/**
 * @brief Функція для створення типу події.
 *
 * @param cat Категорія події.
 * @param id Ідентифікатор події.
 * @return Створений тип події.
 */
static inline EventType event_type(uint8_t cat, uint8_t id)
{
  EventType t = {cat, id};
  return t;
}

EventInputData create_event_input_str(const char *data);

EventInputData create_event_input_data(void *data, size_t data_size);

EventInputData create_event_input_callback(EventDataReadFn read_fn, EventDataSizeFn size_fn);

EventResultData create_event_result();

/**
 * @brief Структура події.
 */
typedef struct
{
  EventType type;         /**< Тип події */
  EventInputData input;   /**< Вхідні дані події */
  EventResultData result; /**< Дані для повернення результату */
} Event;

/**
 * @brief Прототип callback‑функції підписника.
 *
 * @param evt Вказівник на подію.
 * @param subscriber_context Контекст підписника.
 */
typedef void (*EventCallback)(Event *evt, void *subscriber_context);

enum SubSlotStatus
{
  sub_slot_free,
  sub_slot_used,
  sub_slot_inWork
};
typedef uint8_t SubSlotStatus;

/**
 * @brief Структура підписника.
 *
 * Поле active вказує, чи слот використовується.
 * Двосторонній зв’язаний список забезпечується полями next та prev.
 */
typedef struct
{
  SubSlotStatus status;   /**< true, якщо підписник активний, інакше false */
  EventType type;         /**< Тип події, на яку підписаний */
  uint8_t priority;       /**< Пріоритет (менше значення – вищий пріоритет) */
  void *context;          /**< Контекст для callback */
  EventCallback callback; /**< Callback для обробки події */
  int next;               /**< Індекс наступного підписника в списку, -1 якщо кінець */
  int prev;               /**< Індекс попереднього підписника, -1 якщо початок */
} EventSubscriber;

enum EventBusThreadStatus
{
  bus_thread_noStarted,
  bus_thread_working,
  bus_thread_stopping,
  bus_thread_stoped,
};
typedef uint8_t EventBusThreadStatus;

/**
 * @brief Основна структура EventBus.
 *
 * Бібліотека використовує окремий потік для обробки подій,
 * два м’ютекса для синхронізації роботи з чергою подій та списком підписників.
 */
typedef struct
{
  EventBusConfig config; /**< Налаштування EventBus */

  Event *queue;      /**< Черга подій (динамічно виділена) */
  size_t head, tail; /**< Індекси для циклічного буфера подій */

  eventbus_mutex_t queue_mutex; /**< М’ютекс для роботи з чергою подій */
  eventbus_mutex_t subs_mutex;  /**< М’ютекс для роботи зі списком підписників */

  EventSubscriber *subs;       /**< Масив підписників (динамічно виділений) */
  int sub_head;                /**< Індекс першого підписника (найвищий пріоритет) */
  EventBusThreadStatus status; /**< Прапорець роботи потоку обробки подій */

  eventbus_thread_t thread; /**< Потік обробки подій */

#if defined(CONFIG_IDF_TARGET)
  // esp-idf specific
#elif defined(_WIN32)
  // Windows-specific
  DWORD dwThreadId; /**< Умовна змінна для Windows */
#else
  pthread_cond_t cond; /**< Умовна змінна для POSIX */
#endif

} EventBus;

/**
 * @brief Ініціалізує EventBus згідно з переданою конфігурацією.
 *
 * Виділяє пам’ять для черги подій та масиву підписників, ініціалізує м’ютекси,
 * умовну змінну та створює потік обробки подій.
 *
 * @param bus Вказівник на EventBus.
 * @param cfg Вказівник на конфігурацію EventBus.
 * @return 0 при успішній ініціалізації, -1 при помилці.
 */
int eventbus_init(EventBus *bus, EventBusConfig *cfg);

EventBus *eventbus_create(EventBusConfig cfg);

/**
 * @brief Зупиняє роботу EventBus та звільняє всі ресурси.
 *
 * @param bus Вказівник на EventBus.
 */
void eventbus_stop(EventBus *bus);

/**
 * @brief Додає нового підписника до EventBus.
 *
 * Функція повертає вказівник на структуру EventSubscriber, яка використовується для подальшої відписки.
 * Підписник вставляється у двосторонній зв’язаний список за зростанням priority (якщо priority співпадають, новий елемент вставляється після існуючих).
 *
 * @param bus Вказівник на EventBus.
 * @param type Тип події для підписки.
 * @param priority Пріоритет підписника.
 * @param context Контекст підписника.
 * @param callback Callback для обробки події.
 * @return Вказівник на EventSubscriber при успіху, або NULL при помилці.
 */
EventSubscriber *eventbus_subscribe(EventBus *bus, EventType type, uint8_t priority, void *context, EventCallback callback);

/**
 * @brief Видаляє підписника з EventBus.
 *
 * @param bus Вказівник на EventBus.
 * @param subscriber Вказівник на підписника, який потрібно видалити.
 * @return 0 при успіху, -1 якщо підписника не знайдено.
 */
int eventbus_unsubscribe(EventBus *bus, EventSubscriber *subscriber);

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
int eventbus_publish(EventBus *bus, EventType type, EventInputData input, EventResultData result);

#endif
