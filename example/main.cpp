#include "Aulib/AudioStream.h"
#include "Aulib/AudioResamplerSpeex.h"
#include "Aulib/AudioDecoderSndfile.h"
#include "Aulib/AudioDecoderFluidsynth.h"
#include "Aulib/AudioDecoderWildmidi.h"
#include "Aulib/AudioDecoderBassmidi.h"
#include <memory>
#include <SDL.h>
#include <iostream>
//#include "filedetector.h"

using namespace Aulib;

int main(int argc, char* argv[])
{
    init(44100, AUDIO_S16SYS, 2, 4096);

    auto decoder = AudioDecoder::decoderFor(argv[1]);
    if (decoder == nullptr) {
        std::cerr << "No decoder found.\n";
        return 1;
    }

    AudioDecoderBassmidi::setDefaultSoundfont("/usr/local/share/soundfonts/gs.sf2");
    AudioDecoderWildmidi::init("/usr/share/timidity/eawpatches/timidity.cfg", 44100, true, true);

    auto fsynth = dynamic_cast<AudioDecoderFluidSynth*>(decoder);
    auto bassmidi = dynamic_cast<AudioDecoderBassmidi*>(decoder);
    if (fsynth) {
        fsynth->loadSoundfont("/usr/local/share/soundfonts/gs.sf2");
    }

    auto stream = std::make_unique<AudioStream>(argv[1], decoder, new AudioResamplerSpeex);
    stream->play();
    while (stream->isPlaying()) {
        SDL_Delay(200);
    }
    Aulib::quit();
}
