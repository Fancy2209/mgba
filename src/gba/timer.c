/* Copyright (c) 2013-2018 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba/internal/gba/timer.h>

#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>

static void GBATimerIrq(struct GBA* gba, int timerId, uint32_t cyclesLate) {
	struct GBATimer* timer = &gba->timers[timerId];
	if (GBATimerFlagsIsDoIrq(timer->flags)) {
		GBARaiseIRQ(gba, GBA_IRQ_TIMER0 + timerId, cyclesLate);
	}
}

void GBATimerUpdate(struct mTiming* timing, struct GBATimer* timer, uint16_t* io, uint32_t cyclesLate) {
	if (GBATimerFlagsIsCountUp(timer->flags)) {
		*io = timer->reload;
	} else {
		GBATimerUpdateRegisterInternal(timer, timing, io, cyclesLate);
	}
}

static void GBATimerUpdateAudio(struct GBA* gba, int timerId, uint32_t cyclesLate) {
	if (!gba->audio.enable) {
		return;
	}
	if ((gba->audio.chALeft || gba->audio.chARight) && gba->audio.chATimer == timerId) {
		GBAAudioSampleFIFO(&gba->audio, 0, cyclesLate);
	}

	if ((gba->audio.chBLeft || gba->audio.chBRight) && gba->audio.chBTimer == timerId) {
		GBAAudioSampleFIFO(&gba->audio, 1, cyclesLate);
	}
}

bool GBATimerUpdateCountUp(struct mTiming* timing, struct GBATimer* nextTimer, uint16_t* io, uint32_t cyclesLate) {
	if (GBATimerFlagsIsCountUp(nextTimer->flags) && GBATimerFlagsIsEnable(nextTimer->flags)) {
		++*io;
		if (!*io && GBATimerFlagsIsEnable(nextTimer->flags)) {
			GBATimerUpdate(timing, nextTimer, io, cyclesLate);
			return true;
		}
	}
	return false;
}

static void GBATimerUpdate0(struct mTiming* timing, void* context, uint32_t cyclesLate) {
	struct GBA* gba = context;
	GBATimerUpdateAudio(gba, 0, cyclesLate);
	GBATimerUpdate(timing, &gba->timers[0], &gba->memory.io[GBA_REG(TM0CNT_LO)], cyclesLate);
	GBATimerIrq(gba, 0, cyclesLate);
	if (GBATimerUpdateCountUp(timing, &gba->timers[1], &gba->memory.io[GBA_REG(TM1CNT_LO)], cyclesLate)) {
		GBATimerIrq(gba, 1, cyclesLate);
	}
}

static void GBATimerUpdate1(struct mTiming* timing, void* context, uint32_t cyclesLate) {
	struct GBA* gba = context;
	GBATimerUpdateAudio(gba, 1, cyclesLate);
	GBATimerUpdate(timing, &gba->timers[1], &gba->memory.io[GBA_REG(TM1CNT_LO)], cyclesLate);
	GBATimerIrq(gba, 1, cyclesLate);
	if (GBATimerUpdateCountUp(timing, &gba->timers[2], &gba->memory.io[GBA_REG(TM2CNT_LO)], cyclesLate)) {
		GBATimerIrq(gba, 2, cyclesLate);
	}
}

static void GBATimerUpdate2(struct mTiming* timing, void* context, uint32_t cyclesLate) {
	struct GBA* gba = context;
	GBATimerUpdate(timing, &gba->timers[2], &gba->memory.io[GBA_REG(TM2CNT_LO)], cyclesLate);
	GBATimerIrq(gba, 2, cyclesLate);
	if (GBATimerUpdateCountUp(timing, &gba->timers[3], &gba->memory.io[GBA_REG(TM3CNT_LO)], cyclesLate)) {
		GBATimerIrq(gba, 3, cyclesLate);
	}
}

static void GBATimerUpdate3(struct mTiming* timing, void* context, uint32_t cyclesLate) {
	struct GBA* gba = context;
	GBATimerUpdate(timing, &gba->timers[3], &gba->memory.io[GBA_REG(TM3CNT_LO)], cyclesLate);
	GBATimerIrq(gba, 3, cyclesLate);
}

void GBATimerInit(struct GBA* gba) {
	memset(gba->timers, 0, sizeof(gba->timers));
	gba->timers[0].event.name = "GBA Timer 0";
	gba->timers[0].event.callback = GBATimerUpdate0;
	gba->timers[0].event.context = gba;
	gba->timers[0].event.priority = 0x20;
	gba->timers[1].event.name = "GBA Timer 1";
	gba->timers[1].event.callback = GBATimerUpdate1;
	gba->timers[1].event.context = gba;
	gba->timers[1].event.priority = 0x21;
	gba->timers[2].event.name = "GBA Timer 2";
	gba->timers[2].event.callback = GBATimerUpdate2;
	gba->timers[2].event.context = gba;
	gba->timers[2].event.priority = 0x22;
	gba->timers[3].event.name = "GBA Timer 3";
	gba->timers[3].event.callback = GBATimerUpdate3;
	gba->timers[3].event.context = gba;
	gba->timers[3].event.priority = 0x23;
}

void GBATimerUpdateRegister(struct GBA* gba, int timer, int32_t cyclesLate) {
	struct GBATimer* currentTimer = &gba->timers[timer];
	if (GBATimerFlagsIsEnable(currentTimer->flags) && !GBATimerFlagsIsCountUp(currentTimer->flags)) {
		GBATimerUpdateRegisterInternal(currentTimer, &gba->timing, &gba->memory.io[(GBA_REG_TM0CNT_LO + (timer << 2)) >> 1], cyclesLate);
	}
}

void GBATimerUpdateRegisterInternal(struct GBATimer* timer, struct mTiming* timing, uint16_t* io, int32_t skew) {
	if (!GBATimerFlagsIsEnable(timer->flags) || GBATimerFlagsIsCountUp(timer->flags)) {
		return;
	}

	// Align timer
	int prescaleBits = GBATimerFlagsGetPrescaleBits(timer->flags);
	int32_t currentTime = mTimingCurrentTime(timing) - skew;
	int32_t tickMask = (1 << prescaleBits) - 1;
	currentTime &= ~tickMask;

	// Update register
	int32_t tickIncrement = currentTime - timer->lastEvent;
	timer->lastEvent = currentTime;
	tickIncrement >>= prescaleBits;
	tickIncrement += *io;
	while (tickIncrement >= 0x10000) {
		tickIncrement -= 0x10000 - timer->reload;
	}
	*io = tickIncrement;

	// Schedule next update
	tickIncrement = (0x10000 - tickIncrement) << prescaleBits;
	currentTime += tickIncrement;
	currentTime &= ~tickMask;
	mTimingDeschedule(timing, &timer->event);
	mTimingScheduleAbsolute(timing, &timer->event, currentTime);
}

void GBATimerWriteTMCNT_LO(struct GBATimer* timer, uint16_t reload) {
	timer->reload = reload;
}

void GBATimerWriteTMCNT_HI(struct GBATimer* timer, struct mTiming* timing, uint16_t* io, uint16_t control) {
	GBATimerUpdateRegisterInternal(timer, timing, io, 0);

	const unsigned prescaleTable[4] = { 0, 6, 8, 10 };
	unsigned prescaleBits = prescaleTable[control & 0x0003];
	prescaleBits += timer->forcedPrescale;

	GBATimerFlags oldFlags = timer->flags;
	timer->flags = GBATimerFlagsSetPrescaleBits(timer->flags, prescaleBits);
	timer->flags = GBATimerFlagsTestFillCountUp(timer->flags, timer > 0 && (control & 0x0004)); // TODO: Need timer ID
	timer->flags = GBATimerFlagsTestFillDoIrq(timer->flags, control & 0x0040);
	timer->flags = GBATimerFlagsTestFillEnable(timer->flags, control & 0x0080);

	bool reschedule = false;
	if (GBATimerFlagsIsEnable(oldFlags) != GBATimerFlagsIsEnable(timer->flags)) {
		reschedule = true;
		if (GBATimerFlagsIsEnable(timer->flags)) {
			*io = timer->reload;
		}
	} else if (GBATimerFlagsIsCountUp(oldFlags) != GBATimerFlagsIsCountUp(timer->flags)) {
		reschedule = true;
	} else if (GBATimerFlagsGetPrescaleBits(timer->flags) != GBATimerFlagsGetPrescaleBits(oldFlags)) {
		reschedule = true;
	}

	if (reschedule) {
		mTimingDeschedule(timing, &timer->event);
		if (GBATimerFlagsIsEnable(timer->flags) && !GBATimerFlagsIsCountUp(timer->flags)) {
			int32_t tickMask = (1 << prescaleBits) - 1;
			timer->lastEvent = mTimingCurrentTime(timing) & ~tickMask;
			GBATimerUpdateRegisterInternal(timer, timing, io, 0);
		}
	}
}
