#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "..\..\hiredis.h"
#include "..\..\async.h"
#include "..\..\adapters\ae.h"

/* Put event loop in the global scope, so it can be explicitly stopped */
static aeEventLoop *loop;

/* ------------------------------------------------------------------------- */
/* Sync sample                                                             */
/* ------------------------------------------------------------------------- */

void SyncSample() {
    printf("--- SYNC Sample Start ---\n\n");

    unsigned int j;
    redisContext *c;
    redisReply *reply;

    c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        return;
    }

    /* PING server */
    reply = redisCommand(c, "PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key */
    reply = redisCommand(c, "SET %s %s", "foo", "Hello World Sync");
    printf("SET: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key using binary safe API */
    reply = redisCommand(c, "SET %b %b", "bar", (size_t) 3, "Hello", (size_t) 5);
    printf("SET (binary API): %s\n", reply->str);
    freeReplyObject(reply);

    /* Try a GET and two INCR */
    reply = redisCommand(c, "GET foo");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply);

    reply = redisCommand(c, "INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);
    /* again ... */
    reply = redisCommand(c, "INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);

    /* Create a list of numbers, from 0 to 9 */
    reply = redisCommand(c, "DEL mylist");
    freeReplyObject(reply);
    for (j = 0; j < 10; j++) {
        char buf[64];

        _snprintf(buf, 64, "%d", j);
        reply = redisCommand(c, "LPUSH mylist element-%s", buf);
        freeReplyObject(reply);
    }

    /* Let's check what we have inside the list */
    reply = redisCommand(c, "LRANGE mylist 0 -1");
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            printf("%u) %s\n", j, reply->element[j]->str);
        }
    }
    freeReplyObject(reply);

    /* Disconnects and frees the context */
    redisFree(c);

    printf("\n--- SYNC Sample Complete ---\n");
}

/* ------------------------------------------------------------------------- */
/* Get Async sample                                                          */
/* ------------------------------------------------------------------------- */

static int getCallbackCalls = 0;
void getCallbackContinue(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = r;
    if (reply == NULL)
        return;

    getCallbackCalls++;
    printf("callback invoked [%d] - key:%s - value:%s\n", getCallbackCalls, (char*) privdata, reply->str);
}

void getCallbackEnd(redisAsyncContext *c, void *r, void *privdata) {
    getCallbackContinue(c, r, privdata);
    redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected.\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    aeStop(loop);
    printf("Disconnected.\n");
}

void AsyncSample() {
    printf("--- ASYNC Sample Start ---\n\n");
    /* The event loop must be created before the async connect */
    loop = aeCreateEventLoop(1024 * 10);

    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisAsyncFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        return;
    }

    redisAeAttach(loop, c);
    redisAsyncSetConnectCallback(c, connectCallback);
    redisAsyncSetDisconnectCallback(c, disconnectCallback);
    char key_value[30] = "Hello World Async";
    redisAsyncCommand(c, NULL, NULL, "SET key %s", key_value, strlen(key_value));

    int counter = 10;
    printf("Get will be called %i times.\n", counter);
    for (int i = 1; i < counter; i++) {
        redisAsyncCommand(c, getCallbackContinue, "0", "GET key");
    }
    redisAsyncCommand(c, getCallbackEnd, "0", "GET key");

    aeMain(loop);

    printf("\n--- ASYNC Sample Complete ---\n");
}

/* ------------------------------------------------------------------------- */
/* PUBSUB sample                                                             */
/* ------------------------------------------------------------------------- */

void PubSubCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = r;
    if (reply == NULL) return;

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (int j = 0; j < reply->elements; j++) {
            if (reply->element[j]->type == REDIS_REPLY_STRING) {
                printf("%u) %s\n", j, reply->element[j]->str);
                if (strcmp(reply->element[j]->str, "stop") == 0) {
                    aeStop(loop);
                    printf("Disconnected.\n");
                }
            } else if (reply->element[j]->type == REDIS_REPLY_INTEGER) {
                printf("%u) %d\n", j, reply->element[j]->integer);
                if (reply->element[j]->integer == 0) {
                }
            }
        }
    }
}

void PubSubSample() {
    printf("--- PUBSUB Sample Start ---\n\n");
    printf("Run redis-cli.exe and execute the command 'publish foo testvalue'.\n");
    printf("You can repeat the command multiple time with different test value.\n");
    printf("To exit execute 'publish foo stop'.\n\n");

    /* The event loop must be created before the async connect */
    loop = aeCreateEventLoop(1024 * 10);

    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisAsyncFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        return;
    }

    redisAeAttach(loop, c);
    redisAsyncSetConnectCallback(c, connectCallback);
    redisAsyncSetDisconnectCallback(c, disconnectCallback);

    redisAsyncCommand(c, PubSubCallback, NULL, "subscribe foo");

    aeMain(loop);

    printf("\n--- PUBSUB Sample Complete ---\n");
}

/* ------------------------------------------------------------------------- */

int main(int argc, char **argv) {
    SyncSample();
    AsyncSample();
    PubSubSample();
    return 0;
}

