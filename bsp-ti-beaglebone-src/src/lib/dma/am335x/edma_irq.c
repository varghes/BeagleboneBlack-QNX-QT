/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include <stdio.h>
#include <sys/neutrino.h>
#include "edma.h"

static const struct sigevent *irq_handler(void *area, int irq_id)
{
	channel_t *ch = area;
	struct sigevent *event = ch->event;
	/* get_param is an inline function defined in edma.h &
	 * it does not call any QNX functions , so safe to call in qnx context
	 */
	param_t *param = get_param(ch);

	/* re-trigger if the channel is manually triggered */
	if(ch->chfl & CH_TRIG_MANUAL) {
		/* only return sigevent on the last txr if event on complete is set */
		if((ch->flags & DMA_ATTACH_EVENT_ON_COMPLETE) && (param->a_b_cnt && param->ccnt))
			event = NULL;
		out32(ch->cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_ESR, setbit(ch->chno));
	}
	return event;
}


int edma_init_irq(channel_t *ch)
{
	slog("chfl b4 InterrupAttach = %x", ch->chfl);
	ch->irq_id = InterruptAttach(edma_irqno(ch), irq_handler, (void*)ch, 0, _NTO_INTR_FLAGS_TRK_MSK);
	if(ch->irq_id == -1) {
		slog("error in attaching interrupt for cc = %x , irqno = %x", ch->ccno, edma_irqno(ch));
		slog("errno=%d, %s", errno, strerror(errno));
		return -1;
	}
	InterruptMask(edma_irqno(ch), ch->irq_id);
	slog("attached & masked interrupt for cc = %x , irqno = %x", ch->ccno, edma_irqno(ch));
	return 0;
}

void edma_fini_irq(channel_t *ch)
{
	InterruptDetach(ch->irq_id);
	ch->event = NULL;
}
