/*
 *    Copyright (c) 2026 Project CHIP Authors
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <string.h>

#include <poski/osal/os_event.h>
#include <poski/osal/os_event_timer.h>
#include <poski/osal/os_eventq.h>
#include <poski/osal/osal.h>

#include "test_util.h"

/* Ticks elapsed since `start` (wrap-safe). */
#define ELAPSED(start) ((pos_time_t) (pos_time_get() - (start)))

struct event_test_ctx
{
    struct pos_eventq * evq;
    int order[8];
    int count;
    int repost_target;
};

static void record_event_cb(struct pos_event * ev)
{
    struct event_test_ctx * ctx = (struct event_test_ctx *) pos_event_arg_get(ev);
    VerifyOrQuit(ctx != NULL, "event: NULL context arg");
    VerifyOrQuit(!pos_event_is_queued(ev), "event: queued flag must be false during callback");
    if (ctx->count < 8)
    {
        ctx->order[ctx->count] = ctx->count + 1;
    }
    ctx->count++;
}

static void self_repost_cb(struct pos_event * ev)
{
    struct event_test_ctx * ctx = (struct event_test_ctx *) pos_event_arg_get(ev);
    VerifyOrQuit(ctx != NULL, "repost: NULL context");
    VerifyOrQuit(!pos_event_is_queued(ev), "repost: event still marked queued inside cb");
    ctx->count++;
    if (ctx->count < ctx->repost_target)
    {
        /* Self-reposting from inside callback must succeed because queued was cleared on get */
        pos_error_t err = pos_eventq_put(ctx->evq, ev);
        VerifyOrQuit(err == POS_OK, "repost: failed to re-enqueue event");
        VerifyOrQuit(pos_event_is_queued(ev), "repost: event not marked queued after re-put");
    }
}

static void test_basic_eventq(void)
{
    struct pos_eventq evq;
    struct pos_event ev1, ev2, ev3;
    struct event_test_ctx ctx = { 0 };

    VerifyOrQuit(pos_eventq_init(&evq) == POS_OK, "eventq: init failed");
    VerifyOrQuit(pos_eventq_inited(&evq) != 0, "eventq: inited returned false");
    VerifyOrQuit(pos_eventq_is_empty(&evq), "eventq: expected empty initially");

    ctx.evq = &evq;
    pos_event_init(&ev1, record_event_cb, &ctx);
    pos_event_init(&ev2, record_event_cb, &ctx);
    pos_event_init(&ev3, record_event_cb, &ctx);

    /* Post ev1 twice to verify idempotent coalescing */
    VerifyOrQuit(pos_eventq_put(&evq, &ev1) == POS_OK, "eventq: put ev1 failed");
    VerifyOrQuit(pos_event_is_queued(&ev1), "event: ev1 should be queued");
    VerifyOrQuit(pos_eventq_put(&evq, &ev1) == POS_OK, "eventq: duplicate put ev1 failed");
    VerifyOrQuit(pos_eventq_put(&evq, &ev2) == POS_OK, "eventq: put ev2 failed");
    VerifyOrQuit(pos_eventq_put(&evq, &ev3) == POS_OK, "eventq: put ev3 failed");

    /* Remove middle event (ev2) before processing */
    VerifyOrQuit(pos_eventq_remove(&evq, &ev2) == POS_OK, "eventq: remove ev2 failed");
    VerifyOrQuit(!pos_event_is_queued(&ev2), "event: ev2 should not be queued after remove");
    VerifyOrQuit(pos_eventq_remove(&evq, &ev2) == POS_OK, "eventq: removing an unqueued event should be a no-op");

    /* Dequeue and run remaining events (ev1, then ev3) */
    struct pos_event * out1 = pos_eventq_get_no_wait(&evq);
    VerifyOrQuit(out1 == &ev1, "eventq: expected ev1 first");
    pos_event_run(out1);

    struct pos_event * out2 = pos_eventq_get_no_wait(&evq);
    VerifyOrQuit(out2 == &ev3, "eventq: expected ev3 second");
    pos_event_run(out2);

    VerifyOrQuit(pos_eventq_get_no_wait(&evq) == NULL, "eventq: expected NULL when empty");
    VerifyOrQuit(ctx.count == 2, "eventq: expected exactly 2 callbacks executed");

    /* Test self-reposting inside callback */
    ctx.count         = 0;
    ctx.repost_target = 3;
    pos_event_init(&ev1, self_repost_cb, &ctx);
    VerifyOrQuit(pos_eventq_put(&evq, &ev1) == POS_OK, "eventq: put self-repost failed");

    while (!pos_eventq_is_empty(&evq))
    {
        VerifyOrQuit(pos_eventq_poll(&evq, POS_TIME_NO_WAIT) == POS_OK, "eventq: poll failed");
    }
    VerifyOrQuit(ctx.count == 3, "eventq: expected 3 self-reposted executions");

    VerifyOrQuit(pos_eventq_deinit(&evq) == POS_OK, "eventq: deinit failed");
    VerifyOrQuit(pos_eventq_inited(&evq) == 0, "eventq: inited should be false after deinit");
}

/* A statically initialized event needs no pos_event_init() call. */
static struct event_test_ctx s_static_ctx;
static struct pos_event s_static_ev = POS_EVENT_INITIALIZER(record_event_cb, &s_static_ctx);

static void test_event_static_init(void)
{
    struct pos_eventq evq;

    VerifyOrQuit(!pos_event_is_queued(&s_static_ev), "static event: should not be queued initially");
    VerifyOrQuit(pos_event_arg_get(&s_static_ev) == &s_static_ctx, "static event: wrong arg");

    VerifyOrQuit(pos_eventq_init(&evq) == POS_OK, "static event: eventq init failed");
    VerifyOrQuit(pos_eventq_put(&evq, &s_static_ev) == POS_OK, "static event: put failed");
    VerifyOrQuit(pos_eventq_poll(&evq, POS_TIME_NO_WAIT) == POS_OK, "static event: poll failed");
    VerifyOrQuit(s_static_ctx.count == 1, "static event: callback not run");
    VerifyOrQuit(pos_eventq_deinit(&evq) == POS_OK, "static event: eventq deinit failed");
}

static void timer_event_cb(struct pos_event * ev);

static struct pos_eventq s_blocking_evq;
static struct pos_event s_blocking_ev;
static struct pos_event_timer s_blocking_timer;
static struct pos_task s_producer_task;
static struct pos_task s_timer_task;

static void * producer_task_run(void * arg)
{
    (void) arg;

    pos_task_sleep_ms(20);
    SuccessOrQuit(pos_eventq_put(&s_blocking_evq, &s_blocking_ev), "producer: put failed");

    return NULL;
}

static void * timer_task_run(void * arg)
{
    (void) arg;

    pos_task_sleep_ms(20);
    SuccessOrQuit(pos_event_timer_start_ms(&s_blocking_timer, 10), "timer task: start failed");

    return NULL;
}

static void test_eventq_blocking_get(void)
{
    pos_time_t start;

    VerifyOrQuit(pos_eventq_init(&s_blocking_evq) == POS_OK, "blocking: evq init failed");
    pos_event_init(&s_blocking_ev, NULL, NULL);
    VerifyOrQuit(pos_event_timer_init(&s_blocking_timer, &s_blocking_evq, timer_event_cb, NULL) == POS_OK,
                 "blocking: timer init failed");

    /* A timed get on an empty queue waits for the full timeout, then gives up. */
    start = pos_time_get();
    VerifyOrQuit(pos_eventq_get(&s_blocking_evq, pos_time_ms_to_ticks(30)) == NULL, "blocking: expected timeout");
    VerifyOrQuit(ELAPSED(start) >= pos_time_ms_to_ticks(30), "blocking: timed get returned early");

    /* A blocking get is woken by a put from another task. */
    SuccessOrQuit(pos_task_init(&s_producer_task, "producer", producer_task_run, NULL, 1, 1028),
                  "blocking: producer task init failed");
    VerifyOrQuit(pos_eventq_get(&s_blocking_evq, POS_TIME_FOREVER) == &s_blocking_ev, "blocking: expected producer event");
    VerifyOrQuit(!pos_event_is_queued(&s_blocking_ev), "blocking: event should not be queued after get");

    /* A blocking get is woken by the expiry of a timer that another task
     * started 20ms from now, for 10ms. */
    start = pos_time_get();
    SuccessOrQuit(pos_task_init(&s_timer_task, "timer", timer_task_run, NULL, 1, 1028), "blocking: timer task init failed");
    VerifyOrQuit(pos_eventq_get(&s_blocking_evq, POS_TIME_FOREVER) == pos_event_timer_event_get(&s_blocking_timer),
                 "blocking: expected timer event");
    VerifyOrQuit(ELAPSED(start) >= pos_time_ms_to_ticks(30), "blocking: timer event arrived early");

    VerifyOrQuit(pos_event_timer_deinit(&s_blocking_timer) == POS_OK, "blocking: timer deinit failed");
    VerifyOrQuit(pos_eventq_deinit(&s_blocking_evq) == POS_OK, "blocking: evq deinit failed");
}

#define MC_CONSUMERS 2

static struct pos_eventq s_mc_evq;
static struct pos_event s_mc_ev[MC_CONSUMERS];
static struct pos_task s_mc_task[MC_CONSUMERS];
static struct pos_sem s_mc_done;

static void * mc_consumer_run(void * arg)
{
    (void) arg;

    /* Blocks forever: a lost wakeup makes the join in the test time out. */
    VerifyOrQuit(pos_eventq_get(&s_mc_evq, POS_TIME_FOREVER) != NULL, "multi: consumer got no event");
    SuccessOrQuit(pos_sem_give(&s_mc_done), "multi: done give failed");

    return NULL;
}

static void test_eventq_multi_consumer(void)
{
    int i;

    VerifyOrQuit(pos_eventq_init(&s_mc_evq) == POS_OK, "multi: evq init failed");
    SuccessOrQuit(pos_sem_init(&s_mc_done, 0), "multi: sem init failed");
    for (i = 0; i < MC_CONSUMERS; i++)
    {
        pos_event_init(&s_mc_ev[i], NULL, NULL);
        SuccessOrQuit(pos_task_init(&s_mc_task[i], "consumer", mc_consumer_run, NULL, 1, 1028),
                      "multi: consumer task init failed");
    }

    /* Let every consumer block in get, then post back-to-back: each event must
     * wake a different consumer. */
    pos_task_sleep_ms(20);
    for (i = 0; i < MC_CONSUMERS; i++)
    {
        VerifyOrQuit(pos_eventq_put(&s_mc_evq, &s_mc_ev[i]) == POS_OK, "multi: put failed");
    }
    for (i = 0; i < MC_CONSUMERS; i++)
    {
        SuccessOrQuit(pos_sem_take(&s_mc_done, pos_time_ms_to_ticks(1000)), "multi: a consumer was not woken");
    }

    VerifyOrQuit(pos_eventq_is_empty(&s_mc_evq), "multi: queue should be drained");
    VerifyOrQuit(pos_eventq_deinit(&s_mc_evq) == POS_OK, "multi: evq deinit failed");
}

static void timer_event_cb(struct pos_event * ev)
{
    int * fired = (int *) pos_event_arg_get(ev);
    if (fired != NULL)
    {
        (*fired)++;
    }
}

static void test_event_timer(void)
{
    struct pos_eventq evq;
    struct pos_event_timer et;
    int fired = 0;

    VerifyOrQuit(pos_eventq_init(&evq) == POS_OK, "event_timer: evq init failed");
    VerifyOrQuit(pos_event_timer_init(&et, &evq, timer_event_cb, &fired) == POS_OK, "event_timer: init failed");
    VerifyOrQuit(pos_event_timer_inited(&et) == POS_OK, "event_timer: should be inited");
    VerifyOrQuit(!pos_event_timer_is_active(&et), "event_timer: should be inactive after init");
    VerifyOrQuit(pos_event_timer_arg_get(&et) == &fired, "event_timer: wrong arg");
    VerifyOrQuit(pos_event_arg_get(pos_event_timer_event_get(&et)) == &fired, "event_timer: wrong embedded event");

    /* Start a timer for 50ms and wait for its event to arrive on evq */
    pos_time_t start = pos_time_get();
    VerifyOrQuit(pos_event_timer_start_ms(&et, 50) == POS_OK, "event_timer: start_ms failed");
    VerifyOrQuit(pos_event_timer_is_active(&et), "event_timer: should be active");

    pos_time_t remaining = pos_event_timer_remaining_ticks(&et, pos_time_get());
    VerifyOrQuit(remaining > 0 && remaining <= pos_time_ms_to_ticks(50), "event_timer: unexpected remaining ticks");

    pos_error_t err = pos_eventq_poll(&evq, pos_time_ms_to_ticks(500));
    VerifyOrQuit(err == POS_OK, "event_timer: timed out waiting for event on queue");
    VerifyOrQuit(ELAPSED(start) >= pos_time_ms_to_ticks(50), "event_timer: fired early");
    VerifyOrQuit(fired == 1, "event_timer: callback did not fire");
    VerifyOrQuit(!pos_event_timer_is_active(&et), "event_timer: should be inactive after expiry");
    VerifyOrQuit(pos_event_timer_remaining_ticks(&et, pos_time_get()) == 0, "event_timer: remaining should be 0");

    /* Start and immediately stop before expiry */
    VerifyOrQuit(pos_event_timer_start_ms(&et, 200) == POS_OK, "event_timer: restart failed");
    VerifyOrQuit(pos_event_timer_stop(&et) == POS_OK, "event_timer: stop failed");
    VerifyOrQuit(!pos_event_timer_is_active(&et), "event_timer: should be inactive after stop");
    VerifyOrQuit(pos_eventq_poll(&evq, pos_time_ms_to_ticks(50)) == POS_TIMEOUT,
                 "event_timer: stopped timer should not post event");
    VerifyOrQuit(fired == 1, "event_timer: stopped timer should not increment count");

    /* Stop after expiry, before the consumer dequeues the event, removes it */
    VerifyOrQuit(pos_event_timer_start(&et, 1) == POS_OK, "event_timer: short start failed");
    pos_task_sleep_ms(20);
    VerifyOrQuit(!pos_eventq_is_empty(&evq), "event_timer: expired event should be pending");
    VerifyOrQuit(pos_event_timer_stop(&et) == POS_OK, "event_timer: stop after expiry failed");
    VerifyOrQuit(pos_eventq_is_empty(&evq), "event_timer: stop should remove the pending event");
    VerifyOrQuit(fired == 1, "event_timer: stopped event should not run");

    VerifyOrQuit(pos_event_timer_deinit(&et) == POS_OK, "event_timer: deinit failed");
    VerifyOrQuit(pos_event_timer_inited(&et) == POS_EINVAL, "event_timer: should not be inited after deinit");
    VerifyOrQuit(pos_event_timer_deinit(&et) == POS_OK, "event_timer: second deinit should be a no-op");
    VerifyOrQuit(pos_eventq_deinit(&evq) == POS_OK, "event_timer: evq deinit failed");
}

static void test_event_timer_restart(void)
{
    struct pos_eventq evq;
    struct pos_event_timer et;
    int fired = 0;

    VerifyOrQuit(pos_eventq_init(&evq) == POS_OK, "restart: evq init failed");
    VerifyOrQuit(pos_event_timer_init(&et, &evq, timer_event_cb, &fired) == POS_OK, "restart: init failed");

    /* Let the timer expire so that its event is pending in the queue */
    VerifyOrQuit(pos_event_timer_start(&et, 1) == POS_OK, "restart: start failed");
    pos_task_sleep_ms(20);
    VerifyOrQuit(!pos_eventq_is_empty(&evq), "restart: expired event should be pending");
    VerifyOrQuit(pos_event_is_queued(pos_event_timer_event_get(&et)), "restart: event should be queued");

    /* Restarting removes the stale pending event and delivers exactly once */
    pos_time_t start = pos_time_get();
    VerifyOrQuit(pos_event_timer_start_ms(&et, 50) == POS_OK, "restart: restart failed");
    VerifyOrQuit(!pos_event_is_queued(pos_event_timer_event_get(&et)), "restart: stale event should be removed");
    VerifyOrQuit(pos_eventq_is_empty(&evq), "restart: queue should be empty after restart");
    VerifyOrQuit(pos_event_timer_is_active(&et), "restart: should be active after restart");

    VerifyOrQuit(pos_eventq_poll(&evq, pos_time_ms_to_ticks(500)) == POS_OK, "restart: restarted timer did not fire");
    VerifyOrQuit(ELAPSED(start) >= pos_time_ms_to_ticks(50), "restart: restarted timer fired early");
    VerifyOrQuit(fired == 1, "restart: expected exactly one delivery");
    VerifyOrQuit(pos_eventq_poll(&evq, pos_time_ms_to_ticks(80)) == POS_TIMEOUT, "restart: unexpected second delivery");

    VerifyOrQuit(pos_event_timer_deinit(&et) == POS_OK, "restart: deinit failed");
    VerifyOrQuit(pos_eventq_deinit(&evq) == POS_OK, "restart: evq deinit failed");
}

static void test_event_timer_order(void)
{
    struct pos_eventq evq;
    struct pos_event_timer late, early;
    struct pos_event plain;

    VerifyOrQuit(pos_eventq_init(&evq) == POS_OK, "order: evq init failed");
    VerifyOrQuit(pos_event_timer_init(&late, &evq, timer_event_cb, NULL) == POS_OK, "order: late init failed");
    VerifyOrQuit(pos_event_timer_init(&early, &evq, timer_event_cb, NULL) == POS_OK, "order: early init failed");
    pos_event_init(&plain, NULL, NULL);

    /* Timers are delivered in expiry order, not start order */
    pos_time_t start = pos_time_get();
    VerifyOrQuit(pos_event_timer_start_ms(&late, 60) == POS_OK, "order: start late failed");
    VerifyOrQuit(pos_event_timer_start_ms(&early, 20) == POS_OK, "order: start early failed");
    VerifyOrQuit(pos_eventq_get(&evq, pos_time_ms_to_ticks(500)) == pos_event_timer_event_get(&early),
                 "order: expected early timer first");
    VerifyOrQuit(ELAPSED(start) >= pos_time_ms_to_ticks(20), "order: early timer fired early");
    VerifyOrQuit(pos_eventq_get(&evq, pos_time_ms_to_ticks(500)) == pos_event_timer_event_get(&late),
                 "order: expected late timer second");
    VerifyOrQuit(ELAPSED(start) >= pos_time_ms_to_ticks(60), "order: late timer fired early");

    /* A timer that expired before an event was posted is delivered first */
    VerifyOrQuit(pos_event_timer_start(&early, 1) == POS_OK, "order: restart early failed");
    pos_task_sleep_ms(20);
    VerifyOrQuit(pos_eventq_put(&evq, &plain) == POS_OK, "order: put failed");
    VerifyOrQuit(pos_eventq_get_no_wait(&evq) == pos_event_timer_event_get(&early), "order: expected expired timer first");
    VerifyOrQuit(pos_eventq_get_no_wait(&evq) == &plain, "order: expected posted event second");

    VerifyOrQuit(pos_event_timer_deinit(&late) == POS_OK, "order: deinit late failed");
    VerifyOrQuit(pos_event_timer_deinit(&early) == POS_OK, "order: deinit early failed");
    VerifyOrQuit(pos_eventq_deinit(&evq) == POS_OK, "order: evq deinit failed");
}

static void test_event_timer_invalid(void)
{
    struct pos_eventq evq;
    struct pos_event_timer et;

    memset(&et, 0, sizeof(et));
    VerifyOrQuit(pos_event_timer_inited(&et) == POS_EINVAL, "invalid: zeroed timer should not be inited");
    VerifyOrQuit(pos_event_timer_inited(NULL) == POS_EINVAL, "invalid: NULL timer should not be inited");
    VerifyOrQuit(pos_event_timer_start(&et, 1) == POS_INVALID_PARAM, "invalid: start should fail");
    VerifyOrQuit(pos_event_timer_stop(&et) == POS_INVALID_PARAM, "invalid: stop should fail");
    VerifyOrQuit(!pos_event_timer_is_active(&et), "invalid: should not be active");
    VerifyOrQuit(pos_event_timer_deinit(&et) == POS_OK, "invalid: deinit of zeroed timer should be a no-op");

    VerifyOrQuit(pos_eventq_init(&evq) == POS_OK, "invalid: evq init failed");
    VerifyOrQuit(pos_event_timer_init(&et, &evq, NULL, NULL) == POS_INVALID_PARAM, "invalid: NULL fn should fail");
    VerifyOrQuit(pos_event_timer_inited(&et) == POS_EINVAL, "invalid: rejected timer should not be inited");
    VerifyOrQuit(pos_event_timer_init(&et, NULL, timer_event_cb, NULL) == POS_INVALID_PARAM,
                 "invalid: NULL evq should fail");
    VerifyOrQuit(pos_eventq_put(&evq, NULL) == POS_INVALID_PARAM, "invalid: put NULL event should fail");

    /* Delays beyond half the tick range would wrap, so they are rejected */
    VerifyOrQuit(pos_event_timer_init(&et, &evq, timer_event_cb, NULL) == POS_OK, "invalid: init failed");
    VerifyOrQuit(pos_event_timer_start(&et, POS_EVENT_TIMER_MAX_TICKS + 1) == POS_INVALID_PARAM,
                 "invalid: out-of-range ticks should fail");
    VerifyOrQuit(!pos_event_timer_is_active(&et), "invalid: rejected start should leave timer inactive");
    VerifyOrQuit(pos_event_timer_start(&et, POS_EVENT_TIMER_MAX_TICKS) == POS_OK, "invalid: max ticks should be accepted");
    VerifyOrQuit(pos_event_timer_remaining_ticks(&et, pos_time_get()) > POS_EVENT_TIMER_MAX_TICKS / 2,
                 "invalid: max ticks timer should be far from expiry");
    VerifyOrQuit(pos_event_timer_deinit(&et) == POS_OK, "invalid: deinit failed");
    VerifyOrQuit(pos_eventq_is_empty(&evq), "invalid: deinit should leave nothing queued");
    VerifyOrQuit(pos_eventq_deinit(&evq) == POS_OK, "invalid: evq deinit failed");
}

int main(void)
{
    test_basic_eventq();
    test_event_static_init();
    test_eventq_blocking_get();
    test_eventq_multi_consumer();
    test_event_timer();
    test_event_timer_restart();
    test_event_timer_order();
    test_event_timer_invalid();
    printf("All event tests passed\n");
    return PASS;
}
