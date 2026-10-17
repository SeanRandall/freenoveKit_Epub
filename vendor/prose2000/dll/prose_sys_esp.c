#include "prose_sys.h"

#include <stdlib.h>

struct prose_esp_thread {
    TaskHandle_t task;
    SemaphoreHandle_t done;
    void (*fn)(void *);
    void *arg;
};

void sys_mutex_init(sys_mutex *m) { *m = xSemaphoreCreateRecursiveMutex(); }
void sys_mutex_destroy(sys_mutex *m) { if (*m) vSemaphoreDelete(*m); }
void sys_lock(sys_mutex *m) { xSemaphoreTakeRecursive(*m, portMAX_DELAY); }
void sys_unlock(sys_mutex *m) { xSemaphoreGiveRecursive(*m); }
void sys_cond_init(sys_cond *c) { *c = xSemaphoreCreateBinary(); }
void sys_cond_destroy(sys_cond *c) { if (*c) vSemaphoreDelete(*c); }

void sys_cond_wait(sys_cond *c, sys_mutex *m)
{
    sys_unlock(m);
    xSemaphoreTake(*c, portMAX_DELAY);
    sys_lock(m);
}

void sys_cond_timed_wait(sys_cond *c, sys_mutex *m, int ms)
{
    sys_unlock(m);
    xSemaphoreTake(*c, pdMS_TO_TICKS(ms > 0 ? ms : 1));
    sys_lock(m);
}

void sys_cond_broadcast(sys_cond *c) { xSemaphoreGive(*c); }
sys_thread_id sys_thread_self(void) { return xTaskGetCurrentTaskHandle(); }
int sys_thread_equal(sys_thread_id a, sys_thread_id b) { return a == b; }
void sys_sleep_ms(int ms)
{
    TickType_t ticks = pdMS_TO_TICKS(ms > 0 ? ms : 1);
    vTaskDelay(ticks ? ticks : 1);
}

static void thread_main(void *arg)
{
    struct prose_esp_thread *thread = arg;
    thread->fn(thread->arg);
    xSemaphoreGive(thread->done);
    vTaskDelete(NULL);
}

int sys_thread_start(sys_thread *out, void (*fn)(void *), void *arg)
{
    struct prose_esp_thread *thread = calloc(1, sizeof(*thread));
    if (!thread) return -1;
    thread->done = xSemaphoreCreateBinary();
    thread->fn = fn;
    thread->arg = arg;
    if (!thread->done || xTaskCreatePinnedToCore(thread_main, "prose", 8192,
                                                thread, 4, &thread->task, 0) != pdPASS) {
        if (thread->done) vSemaphoreDelete(thread->done);
        free(thread);
        return -1;
    }
    *out = thread;
    return 0;
}

void sys_thread_join(sys_thread thread)
{
    if (!thread) return;
    xSemaphoreTake(thread->done, portMAX_DELAY);
    vSemaphoreDelete(thread->done);
    free(thread);
}

static sys_mutex global_mutex;
static StaticSemaphore_t global_mutex_storage;
static portMUX_TYPE global_guard = portMUX_INITIALIZER_UNLOCKED;

sys_mutex *sys_global_mutex(void)
{
    if (!global_mutex) {
        taskENTER_CRITICAL(&global_guard);
        if (!global_mutex)
            global_mutex = xSemaphoreCreateRecursiveMutexStatic(&global_mutex_storage);
        taskEXIT_CRITICAL(&global_guard);
    }
    return &global_mutex;
}

struct sys_audio { int unused; };
sys_audio *sys_audio_open(void) { return NULL; }
int sys_audio_write(sys_audio *a, const int16_t *pcm, size_t count)
{ (void)a; (void)pcm; (void)count; return -1; }
uint32_t sys_audio_played(sys_audio *a) { (void)a; return 0; }
uint32_t sys_audio_queued(sys_audio *a) { (void)a; return 0; }
void sys_audio_pause(sys_audio *a, int on) { (void)a; (void)on; }
void sys_audio_drop(sys_audio *a) { (void)a; }
void sys_audio_close(sys_audio *a) { (void)a; }
