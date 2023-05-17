#include "sono.h"

#include <thread>

Sono::Sono(float loudness, float loudness_p, float cutoff_,
                           float rolloff_, float before_, float after_, float wait_,
                           float event_)
    : JackCpp::AudioIO("imsono", 2, 2)
    , ebur128(getSampleRate(), {kfr::Speaker::Mono}, 3)
{
    start();
}

int Sono::audioCallback(jack_nframes_t nframes, JackCpp::AudioIO::audioBufVector inBufs, JackCpp::AudioIO::audioBufVector outBufs) noexcept
{
    buffers_buffer.emplace_back(kfr::make_univector(inBufs[0], nframes));

    update_ebu();


    return 0;
}

size_t Sono::buffers_in_seconds(float seconds) const
{
    return std::ceil(getSampleRate()*seconds/getBufferSize());
}

void Sono::update_ebu()
{
    ebuffer.insert(ebuffer.end(), buffers_buffer.back().begin(), buffers_buffer.back().end());

    while (ebuffer.size() > ebur128.packet_size())
    {
        kfr::univector<float> packet = ebuffer.slice(0, ebur128.packet_size());
        ebur128.process_packet({packet});
        ebuffer = ebuffer.slice(ebur128.packet_size());
    }

    ebur128.get_values(loudness_momentary, loudness_short,
                       loudness_intergrated, loudness_range_low,
                       loudness_range_high);
}


void Sono::resize(int _w, int _h) {
    //default_resize(_w, _h);

}

bool Sono::render_gui() {
    bool up = false;



    if (up) {
        //reinit();
    }

    return up;
}


void Sono::render(uint32_t *p) {

    //std::copy(pixels.begin(), pixels.end(), p);
}


