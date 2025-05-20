/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef POSIX_THREADING_H
#define POSIX_THREADING_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include <sys/mutex.h>
#include <sys/sem.h>
#include <sys/thread.h>

#define THREAD_ENTRY void
typedef THREAD_ENTRY (*ThreadEntry)(void*);
#define THREAD_EXIT(RES) return

typedef sys_ppu_thread_t Thread;
typedef sys_lwmutex_t Mutex;
typedef struct {
	Mutex mutex;
	sys_sem_t semaphore;
	int waiting;
} Condition;

static inline int MutexInit(Mutex* mutex) {
	sys_lwmutex_attr_t attr  = { 0 };
	attr.attr_protocol = SYS_MUTEX_PROTOCOL_FIFO;
	attr.attr_recursive = SYS_MUTEX_ATTR_NOT_RECURSIVE;

	return sysLwMutexCreate(mutex, &attr);
}

static inline int MutexDeinit(Mutex* mutex) {
	return sysLwMutexDestroy(mutex);
}

static inline int MutexLock(Mutex* mutex) {
	return sysLwMutexLock(mutex, 0);
}

static inline int MutexTryLock(Mutex* mutex) {
	return sysLwMutexTryLock(mutex);
}

static inline int MutexUnlock(Mutex* mutex) {
	return sysLwMutexUnlock(mutex);
}

static inline int ConditionInit(Condition* cond) {
	int res = MutexInit(&cond->mutex);
	if (res < 0) {
		return res;
	}
	sys_sem_attr_t attr  = { 0 };
	attr.attr_protocol = SYS_SEM_ATTR_PROTOCOL;
	attr.attr_pshared = SYS_SEM_ATTR_PSHARED;
	attr.key = 0;
	attr.flags = 0;

	res = sysSemCreate(&cond->semaphore, &attr, 0, 1);
	if (res < 0) {
		MutexDeinit(&cond->mutex);
		res = cond->semaphore;
	}
	cond->waiting = 0;
	return res;
}

static inline int ConditionDeinit(Condition* cond) {
	MutexDeinit(&cond->mutex);
	return sysSemDestroy(cond->semaphore);
}

static inline int ConditionWait(Condition* cond, Mutex* mutex) {
	int ret = MutexLock(&cond->mutex);
	if (ret < 0) {
		return ret;
	}
	++cond->waiting;
	MutexUnlock(mutex);
	MutexUnlock(&cond->mutex);
	ret = sysSemWait(cond->semaphore, 0);
	if (ret < 0) {
		printf("Premature wakeup: %08X", ret);
	}
	MutexLock(mutex);
	return ret;
}

static inline int ConditionWaitTimed(Condition* cond, Mutex* mutex, int32_t timeoutMs) {
	int ret = MutexLock(&cond->mutex);
	if (ret < 0) {
		return ret;
	}
	++cond->waiting;
	MutexUnlock(mutex);
	MutexUnlock(&cond->mutex);
	u32 timeout = 0;
	if (timeoutMs > 0) {
		timeout = timeoutMs;
	}
	ret = sysSemWait(cond->semaphore, timeout);
	if (ret < 0) {
		printf("Premature wakeup: %08X", ret);
	}
	MutexLock(mutex);
	return ret;
}

static inline int ConditionWake(Condition* cond) {
	MutexLock(&cond->mutex);
	if (cond->waiting) {
		--cond->waiting;
		sysSemPost(cond->semaphore, 1);
	}
	MutexUnlock(&cond->mutex);
	return 0;
}

static inline int ThreadCreate(Thread* thread, ThreadEntry entry, void* context) {
	return sysThreadCreate(thread, entry, context, 0, 0x10000, THREAD_JOINABLE, "sysThread");
}

static inline int ThreadJoin(Thread* thread) {
	return sysThreadJoin(*thread, 0);
}

static inline int ThreadSetName(const char* name) {
	Thread threadToRename;
	sysThreadGetId(&threadToRename);
	return sysThreadRename(threadToRename, name);
}

CXX_GUARD_END

#endif
