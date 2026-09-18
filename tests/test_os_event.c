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

#include <poski/osal/osal.h>

#include "test_util.h"

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
    VerifyOrQuit(pos_event_timer_init(&et, &evq, timer_event_cb, &fired) == POS_OK,
                 "event_timer: init failed");

    /* Start a timer for 50ms and wait for its event to arrive on evq */
    VerifyOrQuit(pos_event_timer_start_ms(&et, 50) == POS_OK, "event_timer: start_ms failed");
    VerifyOrQuit(pos_event_timer_is_active(&et), "event_timer: should be active");

    pos_error_t err = pos_eventq_poll(&evq, pos_time_ms_to_ticks(500));
    VerifyOrQuit(err == POS_OK, "event_timer: timed out waiting for event on queue");
    VerifyOrQuit(fired == 1, "event_timer: callback did not fire");

    /* Start and immediately stop before expiry */
    VerifyOrQuit(pos_event_timer_start_ms(&et, 200) == POS_OK, "event_timer: restart failed");
    VerifyOrQuit(pos_event_timer_stop(&et) == POS_OK, "event_timer: stop failed");
    VerifyOrQuit(!pos_event_timer_is_active(&et), "event_timer: should be inactive after stop");
    VerifyOrQuit(pos_eventq_poll(&evq, pos_time_ms_to_ticks(50)) == POS_TIMEOUT,
                 "event_timer: stopped timer should not post event");
    VerifyOrQuit(fired == 1, "event_timer: stopped timer should not increment count");

    VerifyOrQuit(pos_event_timer_deinit(&et) == POS_OK, "event_timer: deinit failed");
    VerifyOrQuit(pos_eventq_deinit(&evq) == POS_OK, "event_timer: evq deinit failed");
}

int main(void)
{
    test_basic_eventq();
    test_event_timer();
    printf("All event tests passed\n");
    return PASS;
}
