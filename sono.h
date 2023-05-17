#ifndef GATERECORDER_H
#define GATERECORDER_H

#include "art.hpp"
#include "imgui.h"

#include <kfr/all.hpp>

#include <deque>

#include "jackaudioio.hpp"

class Sono : public JackCpp::AudioIO, public Art
{
public:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual void render(uint32_t *p) override;

    using ftype = jack_default_audio_sample_t;
    using buffer_type = std::deque<kfr::univector<ftype>>;
    //using buffer_type = std::deque<ftype>;
    static constexpr size_t fftsize = 1024;
    static constexpr size_t chunk_size = 128;

    Sono()
        : Art("Sonogram from jackd") {}

    virtual int audioCallback(jack_nframes_t nframes, audioBufVector inBufs,
        audioBufVector outBufs) noexcept;

private:
    void update_ebu();
    buffer_type buffers_buffer;
    buffer_type bflush(size_t tail_return = 0);

    ftype loudness_momentary, loudness_short,
            loudness_intergrated, loudness_range_low,
            loudness_range_high;
    kfr::univector<ftype> ebuffer;
    kfr::ebu_r128<ftype> ebur128;
    void update_ebu();

};

#endif // GATERECORDER_H

