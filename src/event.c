#include "event.h"

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