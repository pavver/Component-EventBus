#pragma once

#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include <stddef.h>

typedef enum EventStatus : uint8_t
{
  /**
   * @brief Очікує чергу на асинхронне виконання
   */
  EVENT_WAIT_QUEUE,

  /**
   * @brief Очікує виклик від підписника
   */
  EVENT_STATUS_WAIT_INVOKE,

  /**
   * @brief Подія в обробці підписником
   */
  EVENT_IN_WORK,

  /**
   * @brief Подія виконана, але подія є запитом і очікує обробки відповіді паблішером
   */
  EVENT_WAIT_REQUEST,

  /// @brief Подія виконана
  EVENT_DONE,
} EventStatus;

/**
 * @brief Функція читання данних події в підписнику.
 * @note Виводить данні в buffer, якщо buffer == NULL, то виводить розмір данних
 * @param buffer - буфер для читання данних
 * @param size - розмір буферу
 * @return розмір данних які залишились для зчитування або -1 у разі помилки
 */
typedef int (*EventDataReadFn)(void *buffer, size_t size);

/**
 * @brief Функція відправки данних події паблішеру
 * @param context - контекст паблішера
 * @param buffer - данні для відправки
 * @param size - розмір данних
 * @param event_status - стан події, якщо EVENT_WAIT_REQUEST то це не останні данні,
 * під час обробки цієї події може бути ще виклик цієї функції. якщо EVENT_DONE, то це останні данні і обробка події повністью завершена.
 * @return статус відправки данних, -1 у разі помилки
 */
typedef int (*EventDataWriteFn)(void *context, void *buffer, size_t size, EventStatus event_status);

/**
 * @brief Тип події
 */
typedef struct EventType
{
  /**
   * @brief Група події
   */
  uint8_t group;

  /**
   * @brief Тип події
   */
  uint8_t type;
} EventType;

/**
 * @brief Будь яка група подій
 */
#define EVENT_GROUP_ANY 0

/**
 * @brief Будь який тип події
 */
#define EVENT_TYPE_ANY 0

/**
 * @brief Конкретна подія з конкретної групи
 * @param group Група події
 * @param type Тип події
 */
#define EVENT_TYPE(group, type) ((EventType){.group = type, .type = type})

/**
 * @brief Будь яка подія з конкретної групи
 * @param group Група події
 */
#define EVENT_TYPE(group) ((EventType){.group = group, .type = EVENT_TYPE_ANY})

/**
 * @brief Будь яка подія
 */
#define EVENT_GROUP_ANY() ((EventType){.group = EVENT_GROUP_ANY, .type = EVENT_TYPE_ANY})

/**
 * @brief Структура яка містить дані для відправки підписнику, та/або функцію яка дозволить прочитати вхідні данні
 */
struct EventInputData
{
  EventDataReadFn read_fn;
  void *data;
  size_t data_size;
};

struct event_callback_data
{
  EventDataReadFn read_fn;
  void* additionalValue;
  void *data;
  size_t data_size;
};


bool event_is_type(Event *event, event_type type);
bool event_is_type(Event *event, uint8_t base_type, uint8_t subtype);

struct Event
{
  event_type type;
  event_message_data* message_data;
  event_callback_data* callback_data;
  EventStatus status;
};

#endif // EVENT_H
