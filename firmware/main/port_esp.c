#include <stdint.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "esp_timer.h"

#include "evv_port.h"
#include "evv_arena.h"

#define EVV_EVENT_BIT BIT0
#define EVV_TASK_PRIORITY 5
#define EVV_MIN_STACK 4096

struct evv_sem { SemaphoreHandle_t handle; };
struct evv_event { EventGroupHandle_t handle; };
struct evv_task { TaskHandle_t handle; };

struct evv_start {
    void (*entry)(void *);
    void *arg;
};

static TickType_t evv_timeout_ticks(int ms)
{
    if (ms < 0)
        return portMAX_DELAY;
    return pdMS_TO_TICKS((uint32_t)ms);
}

void evv_port_start(void) { }
void evv_port_finish(void) { }

evv_sem *evv_sem_create(int initial, int most)
{
    evv_sem *s;
    UBaseType_t limit = most > 0 ? (UBaseType_t)most : (UBaseType_t)0x7fffffff;

    if (initial < 0 || (UBaseType_t)initial > limit)
        return NULL;
    s = malloc(sizeof *s);
    if (s == NULL)
        return NULL;
    s->handle = xSemaphoreCreateCounting(limit, (UBaseType_t)initial);
    if (s->handle == NULL) {
        free(s);
        return NULL;
    }
    return s;
}

void evv_sem_destroy(evv_sem *s)
{
    if (s != NULL) {
        vSemaphoreDelete(s->handle);
        free(s);
    }
}

int evv_sem_wait(evv_sem *s, int ms)
{
    if (s == NULL)
        return EVV_WAIT_FAILED;
    if (xSemaphoreTake(s->handle, evv_timeout_ticks(ms)) == pdTRUE)
        return EVV_WAIT_OK;
    return ms >= 0 ? EVV_WAIT_TIMEOUT : EVV_WAIT_FAILED;
}

int evv_sem_post(evv_sem *s, int n)
{
    int i;
    if (s == NULL || n < 0)
        return EVV_WAIT_FAILED;
    for (i = 0; i < n; ++i) {
        if (xSemaphoreGive(s->handle) != pdTRUE)
            return EVV_WAIT_FAILED;
    }
    return EVV_WAIT_OK;
}

evv_event *evv_event_create(int signalled)
{
    evv_event *e = malloc(sizeof *e);
    if (e == NULL)
        return NULL;
    e->handle = xEventGroupCreate();
    if (e->handle == NULL) {
        free(e);
        return NULL;
    }
    if (signalled)
        xEventGroupSetBits(e->handle, EVV_EVENT_BIT);
    return e;
}

void evv_event_destroy(evv_event *e)
{
    if (e != NULL) {
        vEventGroupDelete(e->handle);
        free(e);
    }
}

int evv_event_wait(evv_event *e, int ms)
{
    EventBits_t bits;
    if (e == NULL)
        return EVV_WAIT_FAILED;
    bits = xEventGroupWaitBits(e->handle, EVV_EVENT_BIT, pdFALSE, pdTRUE,
                               evv_timeout_ticks(ms));
    if (bits & EVV_EVENT_BIT)
        return EVV_WAIT_OK;
    return ms >= 0 ? EVV_WAIT_TIMEOUT : EVV_WAIT_FAILED;
}

void evv_event_signal(evv_event *e)
{
    if (e != NULL)
        xEventGroupSetBits(e->handle, EVV_EVENT_BIT);
}

void evv_event_unsignal(evv_event *e)
{
    if (e != NULL)
        xEventGroupClearBits(e->handle, EVV_EVENT_BIT);
}

void evv_event_pulse(evv_event *e)
{
    if (e != NULL) {
        xEventGroupSetBits(e->handle, EVV_EVENT_BIT);
        taskYIELD();
        xEventGroupClearBits(e->handle, EVV_EVENT_BIT);
    }
}

static void evv_trampoline(void *arg)
{
    struct evv_start *start = arg;
    void (*entry)(void *) = start->entry;
    void *entry_arg = start->arg;

    free(start);
    entry(entry_arg);
    evv_frame_done();
    vTaskDeleteWithCaps(NULL);
}

evv_task *evv_task_start(void (*entry)(void *), void *arg, int stack_bytes)
{
    evv_task *task;
    struct evv_start *start;
    uint32_t stack = stack_bytes > EVV_MIN_STACK ? (uint32_t)stack_bytes : EVV_MIN_STACK;

    if (entry == NULL)
        return NULL;
    task = malloc(sizeof *task);
    start = malloc(sizeof *start);
    if (task == NULL || start == NULL) {
        free(task);
        free(start);
        return NULL;
    }
    start->entry = entry;
    start->arg = arg;
    if (xTaskCreateWithCaps(evv_trampoline, "evv", stack, start,
                            EVV_TASK_PRIORITY, &task->handle,
                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) != pdPASS) {
        free(start);
        free(task);
        return NULL;
    }
    return task;
}

void evv_task_stop(evv_task *task) { (void)task; }

int evv_task_priority_get(evv_task *task, int *out)
{
    if (task == NULL)
        return -1;
    if (out != NULL)
        *out = (int)uxTaskPriorityGet(task->handle);
    return 0;
}

int evv_task_priority_set(evv_task *task, int priority)
{
    if (task == NULL || priority < 0 || priority >= configMAX_PRIORITIES)
        return -1;
    vTaskPrioritySet(task->handle, (UBaseType_t)priority);
    return 0;
}

unsigned evv_task_self(void)
{
    return (unsigned)(uintptr_t)xTaskGetCurrentTaskHandle();
}

void evv_sleep_ms(int ms)
{
    if (ms > 0)
        vTaskDelay(pdMS_TO_TICKS((uint32_t)ms));
    else
        taskYIELD();
}

unsigned evv_ticks_ms(void)
{
    return (unsigned)(esp_timer_get_time() / 1000);
}
