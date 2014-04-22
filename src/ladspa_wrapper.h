
#ifdef BUILD_LADSPA

#ifndef LADSPA_WRAPPER_H
#define LADSPA_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ladspa.h"

static LADSPA_Handle ladspa_instantiate (const LADSPA_Descriptor*, unsigned long);
static void ladspa_connect_port (LADSPA_Handle, unsigned long, LADSPA_Data*);
static void ladspa_activate (LADSPA_Handle);
static void ladspa_run (LADSPA_Handle, unsigned long);
static void ladspa_deactivate (LADSPA_Handle);
static void ladspa_cleanup (LADSPA_Handle);

static const LADSPA_PortDescriptor pdescs[NUM_PORTS] = {
    LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT,
    LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT,
    LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT,
    LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT,
    LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
    LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
    LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
    LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
    LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
    LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT
};

static const char* const names[NUM_PORTS] = {
    "Left In",
    "Right In",
    "Left Out",
    "Right Out",
    "Dry/Wet",
    "Feedback",
    "Max. Depth (ms)",
    "Min. Depth (ms)",
    "LFO rate (Hz)",
    "Stereo Phase"
};

static const LADSPA_PortRangeHint hints[NUM_PORTS] = {
    { 0 }, { 0 }, { 0 }, { 0 },
    {
        LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_HIGH,
        -1.0, 1.0
    },
    {
        LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MIDDLE,
        -0.98, 0.98
    },
    {
        LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM,
        0.0, 20.0
    },
    {
        LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM,
        0.0, 20.0
    },
    {
        LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_1,
        0.0, 16.0
    },
    {
        LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MIDDLE,
        0.0, 360.0
    }
};

static const LADSPA_Descriptor ladspadescriptor = {
    271,
    "tfcho",
    0,
    "TF Chorus",
    "original code by TraumFlug, ported by grejppi",
    NULL,
    NUM_PORTS,
    pdescs,
    names,
    hints,
    NULL,
    ladspa_instantiate,
    ladspa_connect_port,
    ladspa_activate,
    ladspa_run,
    NULL,
    NULL,
    ladspa_deactivate,
    ladspa_cleanup
};

static LADSPA_Handle ladspa_instantiate (const LADSPA_Descriptor* ladspadescriptor, unsigned long rate) {
    return (LADSPA_Handle) descriptor.instantiate (&descriptor, (double) rate, NULL, NULL);
}

static void ladspa_connect_port (LADSPA_Handle instance, unsigned long port, LADSPA_Data* data) {
    descriptor.connect_port ((LV2_Handle) instance, (uint32_t) port, (void*) data);
}

static void ladspa_activate (LADSPA_Handle instance) {
    descriptor.activate ((LV2_Handle) instance);
}

static void ladspa_run (LADSPA_Handle instance, unsigned long nframes) {
    descriptor.run ((LV2_Handle) instance, (uint32_t) nframes);
}

static void ladspa_deactivate (LADSPA_Handle instance) {
    descriptor.deactivate ((LV2_Handle) instance);
}

static void ladspa_cleanup (LADSPA_Handle instance) {
    descriptor.cleanup ((LV2_Handle) instance);
}

LV2_SYMBOL_EXPORT const LADSPA_Descriptor* ladspa_descriptor (unsigned long index) {
    switch (index) {
    case 0:
        return &ladspadescriptor;
    
    default:
        return NULL;
    }
}

#ifdef __cplusplus
}
#endif

#endif
#endif
