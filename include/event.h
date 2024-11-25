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
typedef int (*EventDataReadFn)(void *context, void *buffer, size_t size);

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
#define EVENT_TYPE() ((EventType){.group = EVENT_GROUP_ANY, .type = EVENT_TYPE_ANY})

typedef struct EventInputCallbackData
{
  EventDataReadFn read_fn;
  void *context;
} EventInputCallbackData;

typedef struct EventData
{
  void *data;

  /**
   * @brief розмір data в байтах
   */
  size_t data_size;
} EventData;

/**
 * @brief Структура яка містить дані для відправки підписнику, та/або функцію яка дозволить прочитати вхідні данні
 */
typedef struct EventInputData
{
  EventInputCallbackData *callback_data;
  EventData *return_data;
} EventInputData;

EventInputData *create_event_input_callback(EventDataReadFn fn, void *context)
{
  EventInputCallbackData *callback_data_ptr = (EventInputCallbackData *)malloc(sizeof(EventInputCallbackData));
  callback_data_ptr->read_fn = fn;
  callback_data_ptr->context = context;

  EventInputData *data_ptr = (EventInputData *)malloc(sizeof(EventInputData));
  data_ptr->callback_data = callback_data_ptr;
  data_ptr->return_data = NULL;

  return EventInputData;
}

EventInputData *create_event_input_data(char *data)
{
  return create_event_input_data(data, strlen(data) + 1);
}

EventInputData *create_event_input_data(void *data, size_t data_size)
{
  EventData *ret_data_ptr = (EventData *)malloc(sizeof(EventData));
  ret_data_ptr->data = data;
  ret_data_ptr->data_size = data_size;

  EventInputData *data_ptr = (EventInputData *)malloc(sizeof(EventInputData));
  data_ptr->callback_data = NULL;
  data_ptr->return_data = data_ptr;

  return data_ptr;
}

typedef struct EventOutputCallbackData
{
  EventDataWriteFn write_fn;
  void *context;
} EventOutputCallbackData;

/**
 * @brief Структура яка містить данні які мають бути повернені пабліщеру (якщо це Request)
 */
typedef struct EventOutputData
{
  EventOutputCallbackData *callback_data;
  EventData *return_data;
} EventOutputData;

/**
 * @brief Структура яка містить данні події
 */
typedef struct Event
{
  EventType event_type;
  EventInputData *input_data;
  EventOutputData *output_data;
  EventStatus status;
} Event;

void event_destroy(Event *event)
{
  if (event.input_data != NULL)
  {
    if (event->input_data.return_data != NULL)
    {
      if (event->input_data->return_data.data != NULL)
        free(event->input_data->return_data.data);
      free(event->input_data.return_data);
    }
    if (event->input_data.callback_data != NULL)
      free(event->input_data.callback_data);
    free(event.input_data);
  }

  if (event.output_data != NULL)
  {
    if (event->output_data.return_data != NULL)
    {
      if (event->output_data->return_data.data != NULL)
        free(event->output_data->return_data.data);
      free(event->output_data.return_data);
    }
    if (event->output_data.callback_data != NULL)
      free(event->output_data.callback_data);
    free(event.output_data);
  }
}

#endif // EVENT_H
