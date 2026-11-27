// File: Notifier.c

#include "common.h"
#include "notifier.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <btstack.h>
#include <pico/sync.h>
#include <pico/util/queue.h>

#include "BLECore.h"
#include "error.h"


PUBLIC uint32_t Last_MTU = 0;


// -----------------------------------------------------------------------------
// Notification queue + state
// -----------------------------------------------------------------------------

#define NOTIFICATION_CHUNK_SIZE (MAX_MTU_SIZE - 3)
#define MAX_NOTIFICATIONS_QUEUE 32

typedef struct
{
    uint16_t handle;

    uint16_t length;
    uint16_t offset;

    uint8_t buffer[NOTIFICATION_CHUNK_SIZE];

} notification_buff_t;


typedef struct notification
{
    queue_t queue;

    notification_buff_t current;     // Only touched from the BTstack thread.

} notification_t;


PRIVATE notification_t notification = {0};


typedef struct notifier_context
{
    critical_section_t lock;
    bool pending;                    // A run-loop callback is registered and has not yet run.

} notifier_context_t;

PRIVATE notifier_context_t notifier_context = {0};

PRIVATE bool notifier_initialized = false;

// Count of chunks/messages dropped because the queue was full. Incremented by
// producers (any context), reported from the BTstack thread where printf is safe.
PRIVATE volatile uint32_t notifier_dropped = 0;


// -----------------------------------------------------------------------------
// BTstack run-loop task registration
// -----------------------------------------------------------------------------

PRIVATE btstack_context_callback_registration_t notifier_task;


// Safe from any context. Registers notifier_task at most once until it has run.
PRIVATE void notifier_schedule(void)
{
    bool schedule = false;

    critical_section_enter_blocking(&notifier_context.lock);

    if (!notifier_context.pending)
    {
        notifier_context.pending = true;
        schedule = true;
    }

    critical_section_exit(&notifier_context.lock);

    if (schedule)
    {
        btstack_run_loop_execute_on_main_thread(&notifier_task);
    }
}


// This runs INSIDE BTstack's main loop (safe)
PRIVATE void notifier_task_handler(void *context)
{
    notifier_context_t *notifier = context;

    if (notifier)
    {
        // Clear pending BEFORE requesting CSN so a post arriving now re-registers.
        critical_section_enter_blocking(&notifier->lock);
        notifier->pending = false;
        critical_section_exit(&notifier->lock);

        if (connection_handle != HCI_CON_HANDLE_INVALID)
        {
            att_server_request_can_send_now_event(connection_handle);
        }
    }
}


PUBLIC void Notifier_Initialize(void)
{
    if (notifier_initialized)    // Re-init: don't leak the queue, just flush it.
    {
        Notifier_Reset();
        return;
    }

    queue_init(&notification.queue, sizeof(notification_buff_t), MAX_NOTIFICATIONS_QUEUE);

    critical_section_init(&notifier_context.lock);

    notifier_task.callback = notifier_task_handler;
    notifier_task.context  = &notifier_context;

    notifier_initialized = true;
}


// Call from the BTstack thread on disconnect (HCI_EVENT_DISCONNECTION_COMPLETE).
// Deliberately does NOT touch notifier_context.pending: if a task is already
// registered, the handler will clear it. Clearing it here could cause the same
// registration to be queued twice.
PUBLIC void Notifier_Reset(void)
{
    static notification_buff_t drain;     // Avoid a big stack object; BTstack thread only.

    notification.current.length = 0;
    notification.current.offset = 0;

    while (queue_try_remove(&notification.queue, &drain)) { continue; }   // Clear the queue.
}


// -----------------------------------------------------------------------------
// Queue a buffer for notification. Splits into NOTIFICATION_CHUNK_SIZE pieces.
// The data is copied, so the caller's buffer need not remain valid.
// IRQ-safe (queue_t is spinlock protected; no BTstack calls here).
//
// Returns false if nothing was queued or the message was truncated.
//
PUBLIC bool Notifier_Post_buffer
(
    uint16_t handle,
    const char *buffer,
    uint16_t buffer_size
)
{
    if ( !handle
      || !buffer
      || !buffer_size
      || (connection_handle == HCI_CON_HANDLE_INVALID))
    {
        return false;
    }

    // Reject the whole message up front if it clearly can't fit, so the receiver
    // doesn't get a truncated message. (Racy against other producers, so the
    // per-chunk check below is still required.)
    uint32_t chunks = ((uint32_t)buffer_size + NOTIFICATION_CHUNK_SIZE - 1) / NOTIFICATION_CHUNK_SIZE;

    if ((queue_get_level(&notification.queue) + chunks) > MAX_NOTIFICATIONS_QUEUE)
    {
        ++notifier_dropped;
        return false;
    }

    bool result = true;
    bool queued = false;

    while (buffer_size)
    {
        notification_buff_t note;          // Not zero-initialised: only 'length' bytes are used.

        note.handle = handle;
        note.offset = 0;
        note.length = (buffer_size > NOTIFICATION_CHUNK_SIZE) ? NOTIFICATION_CHUNK_SIZE : buffer_size;

        memcpy(note.buffer, buffer, note.length);

        if (!queue_try_add(&notification.queue, &note))
        {
            ++notifier_dropped;
            result = false;
            break;
        }

        queued = true;

        buffer      += note.length;
        buffer_size -= note.length;
    }

    // Kick the pipeline whenever ANYTHING was queued, even on a partial post,
    // otherwise the queued chunks sit until some later post wakes it up.
    if (queued) { notifier_schedule(); }

    return result;
}


// -----------------------------------------------------------------------------
//
PUBLIC void Notifier_Post_String(uint16_t handle, const char* str)
{
    if (str && *str)
    {
        size_t len = strlen(str);

        if (len > UINT16_MAX) { len = UINT16_MAX; }

        Notifier_Post_buffer(handle, str, (uint16_t)len);
    }
}


// -----------------------------------------------------------------------------
// Called from the BTstack thread on ATT_EVENT_CAN_SEND_NOW.
//
PUBLIC void Notifier_Process_Can_Send_Now_Event(void)
{
    notification_buff_t *note = &notification.current;

    // Report drops here, where printf is safe.
    if (notifier_dropped)
    {
        uint32_t dropped = notifier_dropped;
        notifier_dropped = 0;
        POST_ERROR("%lu notification(s) dropped (queue full).\n", (unsigned long)dropped);
    }

    // No active connection: don't load (and thereby strand) data for a dead link.
    if (connection_handle == HCI_CON_HANDLE_INVALID) { return; }

    // Get the next notification if necessary.
    if (!note->length) { if (!queue_try_remove(&notification.queue, note)) { return; } }

    Last_MTU = att_server_get_mtu(connection_handle);

    if (Last_MTU <= 3)
    {
        POST_ERROR("Notifier: Invalid MTU=%lu", (unsigned long)Last_MTU);
        note->length = 0;                // Drop it rather than stall the pipeline.
        note->offset = 0;
        return;
    }

    uint16_t payload   = (uint16_t)(Last_MTU - 3);
    uint16_t remaining = note->length - note->offset;
    uint16_t send_size = (remaining > payload) ? payload : remaining;

    int err = att_server_notify(connection_handle, note->handle, note->buffer + note->offset, send_size);

    if (err)
    {
        if ((err == BTSTACK_ACL_BUFFERS_FULL) || (err == ERROR_CODE_COMMAND_DISALLOWED))
        {
            // Transient: keep 'current' and ask again.
            att_server_request_can_send_now_event(connection_handle);
            return;
        }

        // Anything else (e.g. notifications not enabled on the CCCD) won't fix
        // itself by retrying. Drop this chunk so the rest of the queue isn't blocked.
        POST_ERROR("Notifier: notify=%d", err);
        note->length = 0;
        note->offset = 0;
    }
    else
    {
        note->offset += send_size;

        // Notification completely transmitted?
        if (note->offset >= note->length)
        {
            note->length = 0;
            note->offset = 0;
        }
    }

    // More data waiting?
    bool need_more = (note->length != 0) || !queue_is_empty(&notification.queue);

    if (need_more) { att_server_request_can_send_now_event(connection_handle); }
}


// End of file: Notifier.c
