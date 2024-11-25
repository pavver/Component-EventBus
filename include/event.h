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
 * @param buffer - данні для відправки
 * @param size - розмір данних
 * @return статус відправки данних, -1 у разі помилки
 */
typedef int (*EventDataWriteFn)(void *buffer, size_t size);

enum event_status : uint8_t
{
  /**
   * @brief Очікує виклик від підписника
   */
  EventWaitInvoke,

  /// @brief Подія в обробці підписником
  EventInWork,

  // Подія виконана, але подія є запитом і очікує обробки відповіді паблішером
  EventRequestWait,

  /// @brief Подія виконана
  EventDone,
};

struct event_type
{
  uint8_t base_type;
  uint8_t subtype;
};

struct event_message_data
{
  EventDataWriteFn write_fn;
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
