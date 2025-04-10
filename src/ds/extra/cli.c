/* Copyright (c) 2013-2017 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba/internal/ds/extra/cli.h>

#include <mgba/internal/arm/debugger/cli-debugger.h>
#include <mgba/core/core.h>
#include <mgba/ds/core.h>
#include <mgba/internal/ds/ds.h>

static void _DSCLIDebuggerInit(struct CLIDebuggerSystem*);
static bool _DSCLIDebuggerCustom(struct CLIDebuggerSystem*);

static void _frame(struct CLIDebugger*, struct CLIDebugVector*);
static void _switchCpu(struct CLIDebugger*, struct CLIDebugVector*);

struct CLIDebuggerCommandSummary _DSCLIDebuggerCommands[] = {
	{ "frame", _frame, 0, "Frame advance" },
	{ "cpu", _switchCpu, 0 , "Switch active CPU" },
	{ 0, 0, 0, 0 }
};

struct DSCLIDebugger* DSCLIDebuggerCreate(struct mCore* core) {
	struct DSCLIDebugger* debugger = malloc(sizeof(struct DSCLIDebugger));
	ARMCLIDebuggerCreate(&debugger->d);
	debugger->d.init = _DSCLIDebuggerInit;
	debugger->d.deinit = NULL;
	debugger->d.custom = _DSCLIDebuggerCustom;

	debugger->d.name = "DS";
	debugger->d.commands = _DSCLIDebuggerCommands;
	debugger->d.commandAliases = NULL;

	debugger->core = core;

	return debugger;
}

static void _DSCLIDebuggerInit(struct CLIDebuggerSystem* debugger) {
	struct DSCLIDebugger* dsDebugger = (struct DSCLIDebugger*) debugger;

	dsDebugger->frameAdvance = false;
}

static bool _DSCLIDebuggerCustom(struct CLIDebuggerSystem* debugger) {
	return false;
}

static void _frame(struct CLIDebugger* debugger, struct CLIDebugVector* dv) {
	UNUSED(dv);
	mDebuggerModuleSetNeedsCallback(&debugger->d);

	struct DSCLIDebugger* dsDebugger = (struct DSCLIDebugger*) debugger->system;
	dsDebugger->frameAdvance = true;
}

static void _switchCpu(struct CLIDebugger* debugger, struct CLIDebugVector* dv) {
	UNUSED(dv);
	struct DSCLIDebugger* dsDebugger = (struct DSCLIDebugger*) debugger->system;
	struct mCore* core = dsDebugger->core;
	struct DS* ds = core->board;
	debugger->d.p->platform->deinit(debugger->d.p->platform);
	if (core->cpu == ds->ds9.cpu) {
		core->cpu = ds->ds7.cpu;
		core->timing = &ds->ds7.timing;
	} else {
		core->cpu = ds->ds9.cpu;
		core->timing = &ds->ds9.timing;
	}
	debugger->d.p->platform->init(core->cpu, debugger->d.p->platform);
	debugger->system->printStatus(debugger->system);
}
