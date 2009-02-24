/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>

/*	
 *       System includes for CA		
 */

#include <cadef.h>
#include <alarm.h>

#include "probe.h"

void clearButtons(Buttons *b)
{
    int i = 0;

    b->panel = NULL;
    for (i=1;i<MAX_ADJUST_BUTTONS;i++) {
	b->buttons[i] = NULL;
    }
    b->buttonsSelected = -1;
}

void clearSlider(Slider *s)
{
    s->panel = NULL;
    s->slider = NULL;
    s->lowerLabel = NULL;
    s->upperLabel = NULL;
    s->valueLabel = NULL;
    s->info.D.max = 0;
    s->info.D.min = 0;
    s->info.D.size = 0;
    s->info.D.scl = 1;
}

void clearHistData(histData *h)
{
    h->dialog = NULL;
    h->updateMask = NO_UPDATE;
}

void clearAdjust(adjData *a)
{
    a->dialog = NULL;
    a->panel = NULL;
    a->optionDialog = NULL;
    clearSlider(&(a->slider));
    clearButtons(&(a->buttons));
    clearButtons(&(a->dials));
    a->upMask = ALL_DOWN;
    a->updateMask = NO_UPDATE;
}

void initChannel(atom *channel)
{
    int i;

    channel->updateMask = NO_UPDATE;
    channel->upMask = ALL_DOWN;
    channel->chId = NULL;
    channel->connected = FALSE;
    channel->monitored = FALSE;
    channel->changed = FALSE;
    channel->eventId = NULL;
    for (i=0;i<LAST_ITEM;i++) {
	channel->d[i].proc = NULL;
	channel->d[i].w = NULL;
    }
    initFormat(channel);
    clearHistData(&(channel->hist));
    clearAdjust(&(channel->adjust));
}
