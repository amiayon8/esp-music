#include "audio_decoder.hpp"
#include "flac_decoder.hpp"
#include "mp3_decoder.hpp"
#include "aac_decoder.hpp"
#include "opus_decoder.hpp"
#include "wav_decoder.hpp"

std::unique_ptr<AudioDecoder> AudioDecoder::createDecoder(AudioCodecType codec) {
    switch (codec) {
        case AudioCodecType::FLAC:
            return std::make_unique<FlacDecoder>();
        case AudioCodecType::MP3:
            return std::make_unique<Mp3Decoder>();
        case AudioCodecType::AAC:
            return std::make_unique<AacDecoder>();
        case AudioCodecType::Opus:
            return std::make_unique<OpusDecoder>();
        case AudioCodecType::WAV:
            return std::make_unique<WavDecoder>();
        default:
            return nullptr;
    }
}
