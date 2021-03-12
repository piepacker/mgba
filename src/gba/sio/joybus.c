/* Copyright (c) 2013-2017 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba/internal/gba/sio.h>

#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>

static uint16_t GBASIOJOYWriteRegister(struct GBASIODriver* sio, uint32_t address, uint16_t value);

void GBASIOJOYCreate(struct GBASIODriver* sio) {
	sio->init = NULL;
	sio->deinit = NULL;
	sio->load = NULL;
	sio->unload = NULL;
	sio->writeRegister = GBASIOJOYWriteRegister;
}

uint16_t GBASIOJOYWriteRegister(struct GBASIODriver* sio, uint32_t address, uint16_t value) {
	switch (address) {
	case REG_JOYCNT:
		return (value & 0x0040) | (sio->p->p->memory.io[REG_JOYCNT >> 1] & ~(value & 0x7) & ~0x0040);
	case REG_JOYSTAT:
		return (value & 0x0030) | (sio->p->p->memory.io[REG_JOYSTAT >> 1] & ~0x30);
	}
	return value;
}

int GBASIOJOYSendCommand(struct GBASIODriver* sio, enum GBASIOJOYCommand command, uint8_t* data) {
	switch (command) {
	case JOY_RESET:
		sio->p->p->memory.io[REG_JOYCNT >> 1] |= JOYCNT_RESET;
		if (sio->p->p->memory.io[REG_JOYCNT >> 1] & 0x40) {
			GBARaiseIRQ(sio->p->p, IRQ_SIO, 0);
		}
		// Fall through
	case JOY_POLL:
		data[0] = 0x00;
		data[1] = 0x04;
		data[2] = sio->p->p->memory.io[REG_JOYSTAT >> 1];

		mLOG(GBA_SIO, DEBUG, "JOY %s: %02X (%02X)", command == JOY_POLL ? "poll" : "reset", data[2], sio->p->p->memory.io[REG_JOYCNT >> 1]);
		return 3;
	case JOY_RECV:
		sio->p->p->memory.io[REG_JOYCNT >> 1] |= JOYCNT_RECV;
		sio->p->p->memory.io[REG_JOYSTAT >> 1] |= JOYSTAT_RECV;

		sio->p->p->memory.io[REG_JOY_RECV_LO >> 1] = data[0] | (data[1] << 8);
		sio->p->p->memory.io[REG_JOY_RECV_HI >> 1] = data[2] | (data[3] << 8);

		data[0] = sio->p->p->memory.io[REG_JOYSTAT >> 1];

		mLOG(GBA_SIO, DEBUG, "JOY recv: %02X (%02X)", data[0], sio->p->p->memory.io[REG_JOYCNT >> 1]);

		if (sio->p->p->memory.io[REG_JOYCNT >> 1] & 0x40) {
			GBARaiseIRQ(sio->p->p, IRQ_SIO, 0);
		}
		return 1;
	case JOY_TRANS:
		sio->p->p->memory.io[REG_JOYCNT >> 1] |= JOYCNT_TRANS;
		sio->p->p->memory.io[REG_JOYSTAT >> 1] &= ~JOYSTAT_TRANS;

		data[0] = sio->p->p->memory.io[REG_JOY_TRANS_LO >> 1];
		data[1] = sio->p->p->memory.io[REG_JOY_TRANS_LO >> 1] >> 8;
		data[2] = sio->p->p->memory.io[REG_JOY_TRANS_HI >> 1];
		data[3] = sio->p->p->memory.io[REG_JOY_TRANS_HI >> 1] >> 8;
		data[4] = sio->p->p->memory.io[REG_JOYSTAT >> 1];

		mLOG(GBA_SIO, DEBUG, "JOY trans: %02X%02X%02X%02X:%02X (%02X)", data[0], data[1], data[2], data[3], data[4], sio->p->p->memory.io[REG_JOYCNT >> 1]);

		if (sio->p->p->memory.io[REG_JOYCNT >> 1] & 0x40) {
			GBARaiseIRQ(sio->p->p, IRQ_SIO, 0);
		}
		return 5;
	}
	return 0;
}
