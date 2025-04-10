/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef VIDEO_SOFTWARE_H
#define VIDEO_SOFTWARE_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include <mgba/core/core.h>
#include <mgba/gba/interface.h>
#include <mgba/internal/gba/io.h>
#include <mgba/internal/gba/renderers/common.h>
#include <mgba/internal/gba/video.h>
#ifdef M_CORE_DS
#include <mgba/internal/ds/video.h>
#endif

struct GBAVideoSoftwareBackground {
	unsigned index;
	int enabled;
	unsigned priority;
	uint32_t charBase;
	uint16_t control;
	int mosaic;
	int multipalette;
	uint32_t screenBase;
	int overflow;
	int size;
	int target1;
	int target2;
	uint16_t x;
	uint16_t y;
	int32_t refx;
	int32_t refy;
	int16_t dx;
	int16_t dmx;
	int16_t dy;
	int16_t dmy;
	int32_t sx;
	int32_t sy;
	int yCache;
	uint16_t mapCache[64];
	color_t* extPalette;
	color_t* variantPalette;
	uint32_t flags;
	uint32_t objwinFlags;
	int objwinForceEnable;
	bool objwinOnly;
	bool variant;
	int32_t offsetX;
	int32_t offsetY;
	bool highlight;
};

enum {
	OFFSET_PRIORITY = 30,
	OFFSET_INDEX = 28,
};

#define FLAG_PRIORITY       0xC0000000
#define FLAG_INDEX          0x30000000
#define FLAG_IS_BACKGROUND  0x08000000
#define FLAG_UNWRITTEN      0xFC000000
#define FLAG_REBLEND        0x04000000
#define FLAG_TARGET_1       0x02000000
#define FLAG_TARGET_2       0x01000000
#define FLAG_OBJWIN         0x01000000
#define FLAG_ORDER_MASK     0xF8000000

#define IS_WRITABLE(PIXEL) ((PIXEL) & 0xFE000000)

struct WindowRegion {
	int end;
	int start;
};

struct WindowControl {
	GBAWindowControl packed;
	int8_t priority;
};

#define MAX_WINDOW 5

struct Window {
	uint16_t endX;
	struct WindowControl control;
};

struct GBAVideoSoftwareRenderer {
	struct GBAVideoRenderer d;

	color_t* outputBuffer;
	int outputBufferStride;

	uint32_t* temporaryBuffer;

	uint32_t dispcnt;

	uint32_t row[256];
	uint32_t spriteLayer[256];
	uint8_t alphaA[256];
	uint8_t alphaB[256];
	int32_t spriteCyclesRemaining;

	// BLDCNT
	unsigned target1Obj;
	unsigned target1Bd;
	unsigned target2Obj;
	unsigned target2Bd;
	bool blendDirty;
	enum GBAVideoBlendEffect blendEffect;
	color_t normalPalette[512];
	color_t variantPalette[512];
	color_t* objExtPalette;
	color_t* objExtVariantPalette;
	color_t highlightPalette[512];
	color_t highlightVariantPalette[512];

	uint16_t blda;
	uint16_t bldb;
	uint16_t bldy;

	GBAMosaicControl mosaic;
	bool greenswap;

	struct WindowN {
		struct GBAVideoWindowRegion h;
		struct GBAVideoWindowRegion v;
		struct WindowControl control;
		int16_t offsetX;
		int16_t offsetY;
	} winN[2];

	struct WindowControl winout;
	struct WindowControl objwin;

	struct WindowControl currentWindow;

	int nWindows;
	struct Window windows[MAX_WINDOW];

	struct GBAVideoSoftwareBackground bg[4];

	bool forceTarget1;
	bool oamDirty;
	int oamMax;
	struct GBAVideoRendererSprite sprites[128];
	int tileStride;
	int bitmapStride;
	bool combinedObjSort;
	int16_t objOffsetX;
	int16_t objOffsetY;

	uint32_t scanlineDirty[6];
	uint16_t nextIo[GBA_REG(SOUND1CNT_LO)];
	struct ScanlineCache {
		uint16_t io[GBA_REG(SOUND1CNT_LO)];
		int32_t scale[2][2];
#ifdef M_CORE_DS
	} cache[DS_VIDEO_VERTICAL_PIXELS];
#else
	} cache[GBA_VIDEO_VERTICAL_PIXELS];
#endif
	int nextY;

	int start;
	int end;
	int masterEnd;
	int masterHeight;
	int masterScanlines;

	int masterBright;
	int masterBrightY;

	uint8_t lastHighlightAmount;
};

void GBAVideoSoftwareRendererCreate(struct GBAVideoSoftwareRenderer* renderer);

CXX_GUARD_START

#endif
