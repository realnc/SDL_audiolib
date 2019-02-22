#include "Aulib/AudioDecoderBassmidi.h"
#include "Aulib/AudioDecoderFluidsynth.h"
#include "Aulib/AudioDecoderSndfile.h"
#include "Aulib/AudioDecoderWildmidi.h"
#include "Aulib/AudioResamplerSpeex.h"
#include "Aulib/AudioStream.h"
#include <SDL.h>
#include <iostream>
#include <memory>
//#include "filedetector.h"

using namespace Aulib;

int main(int /*argc*/, char* argv[])
{
    init(44100, AUDIO_S16SYS, 2, 4096);

    AudioDecoderBassmidi::setDefaultSoundfont("/usr/local/share/soundfonts/gs.sf2");
    AudioDecoderWildmidi::init("/usr/share/timidity/current/timidity.cfg", 44100, true, true);

    auto decoder = AudioDecoder::decoderFor(argv[1]);
    if (decoder == nullptr) {
        std::cerr << "No decoder found.\n";
        return 1;
    }

    auto fsynth = dynamic_cast<AudioDecoderFluidSynth*>(decoder.get());
    // auto bassmidi = dynamic_cast<AudioDecoderBassmidi*>(decoder.get());
    if (fsynth != nullptr) {
        fsynth->loadSoundfont("/usr/local/share/soundfonts/gs.sf2");
    }

    AudioStream stream(argv[1], std::move(decoder), std::make_unique<AudioResamplerSpeex>());
    stream.play();
    while (stream.isPlaying()) {
        SDL_Delay(200);
    }
    Aulib::quit();
}
