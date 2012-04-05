/*
 * Copyright 2011-2012 Samy Al Bahra.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ck_rwlock.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../common.h"

#ifndef STEPS 
#define STEPS 1000000
#endif

static int barrier;
static int threads;
static unsigned int flag CK_CC_CACHELINE;
static ck_rwlock_t rwlock = CK_RWLOCK_INITIALIZER;
static struct affinity affinity;

static void *
thread_rwlock(void *pun)
{
	uint64_t s_b, e_b, a, i;
	uint64_t *value = pun;

	if (aff_iterate(&affinity) != 0) {
		perror("ERROR: Could not affine thread");
		exit(EXIT_FAILURE);
	}

	ck_pr_inc_int(&barrier);
	while (ck_pr_load_int(&barrier) != threads)
		ck_pr_stall();

	for (i = 1, a = 0;; i++) {
		s_b = rdtsc();
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		ck_rwlock_read_lock(&rwlock);
		ck_rwlock_read_unlock(&rwlock);
		e_b = rdtsc();

		a += (e_b - s_b) >> 4;

		if (ck_pr_load_uint(&flag) == 1)
			break;
	}

	ck_pr_inc_int(&barrier);
	while (ck_pr_load_int(&barrier) != threads * 2)
		ck_pr_stall();

	*value = (a / i);
	return NULL;
}

int
main(int argc, char *argv[])
{
	int t;
	pthread_t *p;
	uint64_t *latency;

	if (argc != 3) {
		fprintf(stderr, "Usage: throughput <delta> <threads>\n");
		exit(EXIT_FAILURE);
	}

	threads = atoi(argv[2]);
	if (threads <= 0) {
		fprintf(stderr, "ERROR: Threads must be a value > 0.\n");
		exit(EXIT_FAILURE);
	}

	p = malloc(sizeof(pthread_t) * threads);
	if (p == NULL) {
		fprintf(stderr, "ERROR: Failed to initialize thread.\n");
		exit(EXIT_FAILURE);
	}

	latency = malloc(sizeof(uint64_t) * threads);
	if (latency == NULL) {
		fprintf(stderr, "ERROR: Failed to create latency buffer.\n");
		exit(EXIT_FAILURE);
	}

	affinity.delta = atoi(argv[1]);
	affinity.request = 0;

	fprintf(stderr, "Creating threads (rwlock)...");
	for (t = 0; t < threads; t++) {
		if (pthread_create(&p[t], NULL, thread_rwlock, latency + t) != 0) {
			fprintf(stderr, "ERROR: Could not create thread %d\n", t);
			exit(EXIT_FAILURE);
		}
	}
	fprintf(stderr, "done\n");

	sleep(10);
	ck_pr_store_uint(&flag, 1);

	fprintf(stderr, "Waiting for threads to finish acquisition regression...");
	for (t = 0; t < threads; t++)
		pthread_join(p[t], NULL);
	fprintf(stderr, "done\n\n");

	for (t = 1; t <= threads; t++)
		printf("%10u %20" PRIu64 "\n", t, latency[t - 1]);

	return (0);
}

