#include "gaterecorder.h"

#include <cstdarg>
#include <thread>
#include <map>
#include <cmath>

#include <plog/Log.h>

extern bool quiet;

void my_printf ( const char * format, ... )
{
    if (quiet)
        return;
    va_list args;
    va_start (args, format);
    vprintf (format, args);
    va_end (args);
}

std::string GateRecorder::get_filename()
{
    char str[256] = {0};
    std::strftime(str, sizeof(str), "%F_%T.wav", std::localtime(&event_time));
    return str;
}

void audio_dumper(GateRecorder::buffer_type buf, kfr::audio_format af, std::string filename)
{
    size_t sec = buf.size() * buf.front().size() / af.samplerate;
    LOGI.printf("AW %lu sec '%s'", sec, filename.c_str());
    fflush(stdout);
    kfr::audio_writer_wav<float> aw(
                 kfr::open_file_for_writing(filename),
                 af);
    while(buf.size() > 0) {
        aw.write(buf.front());
        buf.pop_front();
    }
}

GateRecorder::GateRecorder(float loudness, float loudness_p, float cutoff_,
                           float rolloff_, float before_, float after_, float wait_,
                           float event_)
    : JackCpp::AudioIO("gate_recorder", 2, 2)
    , loudness_threshold(loudness)
    , passthrough_delta_threshold(loudness_p)
    , cutoff(cutoff_)
    , rolloff(rolloff_)
    , ebur128(getSampleRate(), {kfr::Speaker::Mono}, 3)
{
    //chunks = getBufferSize()/chunk_size;
    LOGI << kfr::library_version();

    af.samplerate = getSampleRate();
    af.channels = 1;

    max_buffers_wait = buffers_in_seconds(wait_);
    buffers_begin = buffers_in_seconds(before_);
    buffers_end = buffers_in_seconds(after_);
    buffer_limit_soft = buffers_in_seconds(60*1);
    buffer_limit_hard = buffers_in_seconds(60*3);
    consecutive_loud_buffers_limit = buffers_in_seconds(event_);


    LOGI.printf("loudness_threshold: %.2f, consecutive_loud_buffers_limit %d, sr %d, bs %d, 1sec %d",
                loudness_threshold,
                consecutive_loud_buffers_limit,
                getSampleRate(), getBufferSize(), buffers_in_seconds(1));

    if (rolloff > 0 && cutoff > 0)
    {
        LOGI.printf("Using highpass filter cutoff %2.f, rolloff %.2f", cutoff, rolloff);
        filt = kfr::iir_highpass(kfr::bessel<kfr::fbase>(rolloff), cutoff, getSampleRate());
        bqs = kfr::to_sos(filt);
    }

    ebur128.start();

    start();
}

int GateRecorder::audioCallback(jack_nframes_t nframes, JackCpp::AudioIO::audioBufVector inBufs, JackCpp::AudioIO::audioBufVector outBufs) noexcept
{
    buffers_buffer.emplace_back(kfr::make_univector(inBufs[0], nframes));

    if (rolloff > 0 && cutoff > 0)
    {
        kfr::univector<float> filtered = kfr::biquad<32>(bqs, buffers_buffer.back());
        float * p = &outBufs[0][0];
        for (auto & v : filtered)
            *p++ = v;
        buffers_buffer.back() = filtered;
    }

    update_ebu();

    if (passthrough)
        buffers_passthrough.emplace_back(buffers_buffer.back());

    bool frame_loud = loudness_momentary > loudness_threshold;
    if (frame_loud)
    {
        ++consecutive_loud_frames;
        buffers_past_loud = 0;
        if (!recording && consecutive_loud_frames >= consecutive_loud_buffers_limit)
        {
            while(buffers_buffer.size() > buffers_begin)
                buffers_buffer.pop_front();
            recording = true;
            event_time = std::time(nullptr);
        }
    }
    else
        consecutive_loud_frames = 0;

    ++buffers_past_loud;

    if (!passthrough)
    {
        passthrough = loudness_short > passthrough_delta_threshold;
        buffers_passed_through = 0;
    }
    if (passthrough)
    {
        if (buffers_passthrough.size() == 0)
            buffers_passthrough = buffer_type(
                        buffers_buffer.end()-std::min(buffers_buffer.size(), buffers_begin),
                        buffers_buffer.end());

        if (buffers_passed_through > buffers_end)
        {
            passthrough = false;
            ebur128.reset();
        }
    }

    if (buffers_past_loud == max_buffers_wait && recording)
    {
        // Too long silence. Normal end of recording.
        buffers_buffer = bflush(max_buffers_wait - buffers_end);
        recording = false;
        ebur128.reset();
    }

    if (buffers_past_loud < max_buffers_wait && recording)
    {
        // Normal recording state. Check file length limits.
        if (buffers_buffer.size() > buffer_limit_soft)
        {
            if (buffers_buffer.size() > buffer_limit_hard)
            {
                my_printf("hard hit\n");
                bflush();
            }
            else if(buffers_past_loud > max_buffers_wait/3)
            {
                my_printf("soft hit\n");
                buffers_buffer = bflush(max_buffers_wait/3 - buffers_end);
            }
        }
    }

    my_printf("  fp:%3lu", buffers_passthrough.size());
    if (buffers_passthrough.size() > 0)
    {
        std::copy_n(buffers_passthrough.front().begin(), nframes, outBufs[0]);
        buffers_passthrough.pop_front();
        ++buffers_passed_through;
        my_printf(" fpt:%3u", buffers_passed_through);
    }
    else
        std::fill_n(outBufs[0], nframes, 0);

    if (buffers_past_loud > max_buffers_wait)
    {
        // Too long after last loud frame. This means that we are not recording
        // and do not need to pile up buffers_buffer
        buffers_buffer.pop_front();

        buffers_past_loud = max_buffers_wait+1; // avoid overflow
    }

    if (frame_loud)
        my_printf("  loud frame\n");
    else if (loudness_momentary > passthrough_delta_threshold)
        my_printf("  momentary > passthrough\n");
    else if (loudness_short > passthrough_delta_threshold)
        my_printf("  short > passthrough\n");
    else if (consecutive_loud_frames > 0)
        my_printf("  clf:%d", consecutive_loud_frames);
    else
        my_printf("\r");
    fflush(stdout);
    return 0;
}

size_t GateRecorder::buffers_in_seconds(float seconds) const
{
    return std::ceil(getSampleRate()*seconds/getBufferSize());
}

void GateRecorder::update_ebu()
{
    ebuffer.insert(ebuffer.end(), buffers_buffer.back().begin(), buffers_buffer.back().end());

    while (ebuffer.size() > ebur128.packet_size())
    {
        kfr::univector<float> packet = ebuffer.slice(0 ,ebur128.packet_size());
        ebur128.process_packet({packet});
        ebuffer = ebuffer.slice(ebur128.packet_size());
    }

    ebur128.get_values(loudness_momentary, loudness_short,
                       loudness_intergrated, loudness_range_low,
                       loudness_range_high);

    // loud is short:-48.56 noise -68
    my_printf("m:%5.2f (%5.2f)  ", loudness_momentary, loudness_threshold);
    my_printf("s:%5.2f (%5.2f)  ", loudness_short, passthrough_delta_threshold);
    my_printf("i:%5.2f  ", loudness_intergrated);
    my_printf("l:%5.2f  ", loudness_range_low);
    my_printf("h:%5.2f  ", loudness_range_high);
}

GateRecorder::buffer_type GateRecorder::bflush(size_t tail_return)
{
    buffer_type tailb;
    while(tail_return--)
    {
        tailb.emplace_front(buffers_buffer.back());
        buffers_buffer.pop_back();
    }

    std::thread t(audio_dumper, std::move(buffers_buffer), af, get_filename());
    t.detach();
    buffers_past_loud = 0;
    return tailb;
}
