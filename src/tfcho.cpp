/*

chorus effect with simple feedback mechanism
lags output by 1 sample, even in dry mode (!!!)

the dry/wet slider allows negative values, those
are applied to the wet signal only (to get a phase-
cancellation-at-minimum-delay kind of chorus). the
dry signal is always passed on without sign changed

the lfo minimum modulation depth gets substracted
from the overall maximum modulation depth, so the
maximum depth is the same whatever min. depth you
specify.

by TraumFlug

*/

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#define URI "urn:traumflug:lunar:effect:tfcho"


//maximum depth in milliseconds, for buffer sizes
//
#define MAX_DEPTH_MS 20.0
//
//make sure this is at least larger than the depths
//accessible in the manifest.xml

struct tfcho {
    //allover constants
    float smpfreq;           //samps/second

    //history sample buffers dry & wet
    unsigned int buf_size;   //size of the 4 buffers
    unsigned int buf_wrap;   //mask for wraparound
    float* dbufl;            //dry input buffers
    float* dbufr;
    float* wbufl;            //wet output buffers for feedback
    float* wbufr;
    unsigned int bpos;       //buffer(s) write position

    //parms constant to process_stereo
    float dry, wet;          //dry, wet levels
    float feedback;          //feedback can be neg., too
    float depth, minmod;     //modulation depths in _samples_ !!!
    float mrang;             //modulation range in samples
    float cyclen;            //rate, in samps per cycle (with fraction!)
    float cycinc;            //cycle propagation value (per sample) -> 1.0 / cyclen
    float sphase;            //stereo phaseshift of r, linear 0.0..1.0

    //vars used/propagated by process_stereo

    float cycpos;            //cycle/phase positions 0.0..1.0

    //routines

    //allocate buffers sized by power of two with enough space for desired depth
    void alloc_buffers (float srate, float maxdepth) {
        //find minimum size for buffer to hold full delay line
        //+4 just to be on the safe side... ;)
        int minsize = (int) ((srate/1000.0) * maxdepth) + 4;
        int bits = 2;
        while ((minsize - (1 << bits)) > 0 ) {
            ++bits;
        }
        buf_size = 1 << bits;
        buf_wrap = buf_size - 1;
        
        //allocate & set zero
        dbufl = new float[buf_size];
        wbufl = new float[buf_size];
        dbufr = new float[buf_size];
        wbufr = new float[buf_size];
        
        for (unsigned int i = 0; i < buf_size; ++i) {
            dbufl[i] = 0.0;
            wbufl[i] = 0.0;
            dbufr[i] = 0.0;
            wbufr[i] = 0.0;
        }
    }

    void init (double rate) {
        //basic parms
        smpfreq = (float) rate;
        dry = 0.5;
        wet = 0.5;
        feedback = 0.0;
        depth = (smpfreq * 0.001) * 10.0;
        minmod = 0.0;
        mrang = depth;
        cyclen = smpfreq;
        cycinc = 1.0 / smpfreq;
        sphase = 0.5;
        
        //buffers
        dbufl = 0;
        wbufl = 0;
        dbufr = 0;
        wbufr = 0;
        alloc_buffers (smpfreq, MAX_DEPTH_MS);
        bpos = 0;
        
        //running (lfo) values
        cycpos = 0.0;
    }

    void exit () {
        if (dbufl) {
            delete[] dbufl;
        }
        if (wbufl) {
            delete[] wbufl;
        }
        if (dbufr) {
            delete[] dbufr;
        }
        if (wbufr) {
            delete[] wbufr;
        }
    }

    //pseudo sine by quadratics...not because it's faster, but because is is "funny":
    //
    //phase 0.0 .. 0.99999 corresponds to 0..2pi ...don't pass 1.0...!!!
    //don't expect wraparound like with real sin(), wrap the phase yourself!!!
    //
    //result has range 0.0 .. 1.0 (because it's used for lfo stuff, not oscillator)

    //branchless version
    inline float pseudosin (float phase) {
        float tmp1;
        float tmp2;
        float sm1;
        float sm2;
        
        phase *= 2.0;
        sm2 = floor (phase);
        sm1 = 1.0 - sm2;
        phase *= 2.0;
        tmp1 = phase - 1.0;
        tmp1 = 0.5 + 0.5 * (1.0 - (tmp1 * tmp1));
        tmp2 = phase - 3.0;
        tmp2 = 0.5 * (tmp2 * tmp2);
        return tmp1 * sm1 + tmp2 * sm2;
    }

/*
    //branching version
    inline float pseudosin (float phase) {
        float tmp;
        if (phase < 0.5) {
            tmp = phase * 4.0 - 1.0;
            tmp = 0.5 + 0.5 * (1.0 - (tmp * tmp));
        } else {
            tmp = phase * 4.0 - 3.0;
            tmp = 0.5 * (tmp * tmp);
        }
        return tmp;
    }
*/

    inline float wrapflt (float towrap) {
        return towrap - floor (towrap);
    }

    void process_stereo (const float* inL, const float* inR, float* outL, float* outR, int n) {
        float tmpdl;
        float tmpdr;
        
        float tmpwl;
        float tmpwr;
        
        float offsl;
        float offsr;
        
        int readoffsl;
        float roffsfracl;
        
        int readoffsr;
        float roffsfracr;
        
        for (int i = 0; i < n; ++i) {
            //get input signal
            tmpdl = inL[i];
            tmpdr = inR[i];
            
            //feed dry input buffer with dry signal
            dbufl[bpos] = tmpdl;
            dbufr[bpos] = tmpdr;
            
            //calculate interpolation offsets
            offsl = (pseudosin (wrapflt (cycpos + 0.25)) * mrang) + minmod;
            readoffsl = (int) (floor (offsl));
            roffsfracl = offsl - floor(offsl);
            offsr = (pseudosin (wrapflt (cycpos + 0.25 + sphase)) * mrang) + minmod;
            readoffsr = (int) (floor(offsr));
            roffsfracr = offsr - floor(offsr);
            
            //get detuned signal, from dry buffer
            tmpwl = (dbufl[(bpos - readoffsl - 1) & buf_wrap] * (1.0 - roffsfracl) + dbufl[(bpos - readoffsl - 2) & buf_wrap] * roffsfracl);
            tmpwr = (dbufr[(bpos - readoffsr - 1) & buf_wrap] * (1.0 - roffsfracr) + dbufr[(bpos - readoffsr - 2) & buf_wrap] * roffsfracr);
            
            //add feedback from feedback buffer, also detuned!!! :)
            tmpwl += feedback * (wbufl[(bpos-readoffsl - 1) & buf_wrap] * (1.0 - roffsfracl) + wbufl[(bpos - readoffsl - 2) & buf_wrap] * roffsfracl);
            tmpwr += feedback * (wbufr[(bpos-readoffsr - 1) & buf_wrap] * (1.0 - roffsfracr) + wbufr[(bpos - readoffsr - 2) & buf_wrap] * roffsfracr);
            
            //store signal for feedback, apply wet-scalar
            wbufl[bpos] = tmpwl;
            tmpwl *= wet;
            
            wbufr[bpos] = tmpwr;
            tmpwr *= wet;
            
            //add dry signal, delayed 1 sample so it's in sync with the feedback buffer
            tmpwl += dry * dbufl[(bpos - 1) & buf_wrap];
            tmpwr += dry * dbufr[(bpos - 1) & buf_wrap];
            
            //write output
            outL[i] = tmpwl;
            outR[i] = tmpwr;
            
            //advance&wrap lfo cycle
            cycpos += cycinc;
            cycpos = wrapflt (cycpos);
            
            //advance&wrap ring buffer index
            ++bpos;
            bpos &= buf_wrap;
        }
    }
};

enum PortIndex {
    LIN,
    RIN,
    LOUT,
    ROUT,
    DRYWET,
    FEEDBACK,
    DEPTH,
    MINMOD,
    RATE,
    SPHASE,
    NUM_PORTS
};

struct TFChorus {
    struct {
        const float* lin;
        const float* rin;
        float* lout;
        float* rout;
        
        const float* drywet;
        const float* feedback;
        const float* depth;
        const float* minmod;
        const float* rate;
        const float* sphase;
    } ports;
    
    tfcho* instance;
    
    float minmod;
    float depth;
};

static LV2_Handle instantiate (const LV2_Descriptor* descriptor, double rate, const char* bundle_path, const LV2_Feature* const* features) {
    TFChorus* self = new TFChorus;
    self->instance = new tfcho ();
    self->instance->init (rate);
    
    return (LV2_Handle) self;
}

#define TFCHO_CONNECT_SWITCH(p) switch ((PortIndex) p)
#define TFCHO_CONNECT(c,f,t) case c: self->ports.f = (t) data; break

static void connect_port (LV2_Handle instance, uint32_t port, void* data) {
    TFChorus* self = (TFChorus*) instance;
    
    TFCHO_CONNECT_SWITCH (port) {
        TFCHO_CONNECT (LIN, lin, const float*);
        TFCHO_CONNECT (RIN, rin, const float*);
        TFCHO_CONNECT (LOUT, lout, float*);
        TFCHO_CONNECT (ROUT, rout, float*);
        TFCHO_CONNECT (DRYWET, drywet, const float*);
        TFCHO_CONNECT (FEEDBACK, feedback, const float*);
        TFCHO_CONNECT (DEPTH, depth, const float*);
        TFCHO_CONNECT (MINMOD, minmod, const float*);
        TFCHO_CONNECT (RATE, rate, const float*);
        TFCHO_CONNECT (SPHASE, sphase, const float*);
        default: break;
    }
}

static void activate (LV2_Handle instance) {
}

static void run (LV2_Handle instance, uint32_t nframes) {
    TFChorus* self = (TFChorus*) instance;
    
    self->instance->wet = *(self->ports.drywet);
    self->instance->dry = 1.0 - (std::abs (*(self->ports.drywet)));
    
    self->instance->feedback = *(self->ports.feedback);
    self->instance->minmod = (self->instance->smpfreq * 0.001) * (*(self->ports.minmod));
    self->instance->depth = (self->instance->smpfreq * 0.001) * (*(self->ports.depth));
    
    self->instance->cyclen = (1 / (*(self->ports.rate))) * self->instance->smpfreq;
    self->instance->cycinc = 1.0 / self->instance->cyclen;
    
    self->instance->sphase = *(self->ports.sphase) / 360.0;
    
    if ((self->depth != self->instance->depth) || (self->minmod != self->instance->minmod)) {
        if (self->instance->depth > self->instance->minmod) {
            self->instance->mrang = self->instance->depth - self->instance->minmod;
        } else {
            self->instance->mrang = 0.0;
        }
        
        self->depth = self->instance->depth;
        self->minmod = self->instance->minmod;
    }
    
    self->instance->process_stereo (self->ports.lin, self->ports.rin, self->ports.lout, self->ports.rout, nframes);
}

static void deactivate (LV2_Handle instance) {
}

static void cleanup (LV2_Handle instance) {
    TFChorus* self = (TFChorus*) instance;
    self->instance->exit ();
    delete self->instance;
    delete self;
}

static const void* extension_data (const char* uri) {
    return 0;
}

static const LV2_Descriptor descriptor = {
    URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor (uint32_t index) {
    switch (index) {
    case 0:
        return &descriptor;
    
    default:
        return 0;
    }
}

#include "ladspa_wrapper.h"
