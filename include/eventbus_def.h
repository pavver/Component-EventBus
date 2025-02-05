
#ifndef EVENTBUS_DEF_H
#define EVENTBUS_DEF_H

#if defined(CONFIG_IDF_TARGET)
  // esp-idf specific

typedef SemaphoreHandle_t eventbus_mutex_t;
#define EVENTBUS_MUTEX_INIT(m) m = xSemaphoreCreateMutex()
#define EVENTBUS_MUTEX_LOCK(m) xSemaphoreTake(m, portMAX_DELAY)
#define EVENTBUS_MUTEX_UNLOCK(m) xSemaphoreGive(m)

typedef TaskHandle_t eventbus_thread_t;
#define TASK_DELAY(x) vTaskDelay(pdMS_TO_TICKS(x))
typedef TickType_t TimeType;
#define THREAD_RETURN_TYPE void
#define THREAD_ARG_TYPE void *
#define THREAD_RETURN

#elif defined(_WIN32)
  // Windows-specific

#include <windows.h>
typedef CRITICAL_SECTION eventbus_mutex_t;
#define EVENTBUS_MUTEX_INIT(m) InitializeCriticalSection(m)
#define EVENTBUS_MUTEX_LOCK(m) EnterCriticalSection(m)
#define EVENTBUS_MUTEX_UNLOCK(m) LeaveCriticalSection(m)

typedef HANDLE eventbus_thread_t;
#define TASK_DELAY(x) Sleep(x)
typedef DWORD TimeType;
#define THREAD_RETURN_TYPE DWORD WINAPI
#define THREAD_ARG_TYPE LPVOID
#define THREAD_RETURN 0

#else
  // Unix-specific

#include <pthread.h>
typedef pthread_mutex_t eventbus_mutex_t;
#define EVENTBUS_MUTEX_INIT(m) pthread_mutex_init(m, NULL)
#define EVENTBUS_MUTEX_LOCK(m) pthread_mutex_lock(m)
#define EVENTBUS_MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
typedef pthread_t eventbus_thread_t;

#endif


#endif
