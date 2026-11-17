#include "audio/wav_parser.h"

#include <cstdio>
#include <cstring>

namespace {

uint16_t readLe16(const uint8_t* bytes) {
  return static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
}

uint32_t readLe32(const uint8_t* bytes) {
  return static_cast<uint32_t>(bytes[0]) |
         (static_cast<uint32_t>(bytes[1]) << 8) |
         (static_cast<uint32_t>(bytes[2]) << 16) |
         (static_cast<uint32_t>(bytes[3]) << 24);
}

constexpr uint16_t kFormatPcm = 1;
constexpr uint16_t kFormatExtensible = 0xFFFE;

}  // namespace

WavStreamParser::WavStreamParser(uint32_t sampleRate, Sink& sink)
  : sink_(sink), sampleRate_(sampleRate) {}

void WavStreamParser::feed(const uint8_t* bytes, size_t length) {
  while (length > 0 && state_ != State::Failed) {
    size_t used = 0;

    switch (state_) {
      case State::RiffHeader:
        used = fillHeader(bytes, length);
        if (headerFill_ == headerNeed_) {
          if (memcmp(header_, "RIFF", 4) != 0 || memcmp(header_ + 8, "WAVE", 4) != 0) {
            fail("not a WAV file");
            break;
          }
          state_ = State::ChunkHeader;
          headerFill_ = 0;
          headerNeed_ = 8;
        }
        break;

      case State::ChunkHeader:
        used = fillHeader(bytes, length);
        if (headerFill_ == headerNeed_) {
          beginChunk();
        }
        break;

      case State::Format:
        used = fillHeader(bytes, length);
        if (headerFill_ == headerNeed_) {
          parseFormat();
        }
        break;

      case State::Skip:
        if (chunkRemaining_ == 0) {
          endChunk();
          break;
        }
        used = length < chunkRemaining_ ? length : chunkRemaining_;
        chunkRemaining_ -= used;
        break;

      case State::Pcm:
        used = consumePcm(bytes, length);
        break;

      case State::Trailer:
        used = length;
        break;

      case State::Failed:
        break;
    }

    bytes += used;
    length -= used;
  }
}

void WavStreamParser::finish() {
  flush();

  if (state_ == State::RiffHeader && headerFill_ > 0) {
    fail("not a WAV file");
  } else if (state_ != State::Failed && !reachedPcm_) {
    fail("no audio data");
  }
}

bool WavStreamParser::failed() const {
  return state_ == State::Failed;
}

const char* WavStreamParser::error() const {
  return failed() ? message_ : nullptr;
}

uint32_t WavStreamParser::samplesDelivered() const {
  return samplesDelivered_;
}

size_t WavStreamParser::fillHeader(const uint8_t* bytes, size_t length) {
  const size_t wanted = headerNeed_ - headerFill_;
  const size_t used = length < wanted ? length : wanted;
  memcpy(header_ + headerFill_, bytes, used);
  headerFill_ += used;
  return used;
}

void WavStreamParser::beginChunk() {
  const uint32_t size = readLe32(header_ + 4);
  headerFill_ = 0;
  chunkPadded_ = (size & 1u) != 0;

  if (memcmp(header_, "fmt ", 4) == 0) {
    if (size < 16) {
      fail("malformed fmt chunk");
      return;
    }
    headerNeed_ = size < kHeaderBytes ? size : kHeaderBytes;
    chunkRemaining_ = size - headerNeed_;
    state_ = State::Format;
    return;
  }

  if (memcmp(header_, "data", 4) == 0) {
    if (!formatSeen_) {
      fail("data chunk before fmt chunk");
      return;
    }
    // Streaming encoders write 0 or 0xFFFFFFFF when the length is not known up front.
    chunkRemaining_ = size == 0 ? kUnknownLength : size;
    reachedPcm_ = true;
    state_ = State::Pcm;
    return;
  }

  chunkRemaining_ = size;
  state_ = State::Skip;
}

void WavStreamParser::parseFormat() {
  uint16_t format = readLe16(header_);
  const uint16_t channels = readLe16(header_ + 2);
  const uint32_t rate = readLe32(header_ + 4);
  const uint16_t bits = readLe16(header_ + 14);

  if (format == kFormatExtensible && headerNeed_ >= 26) {
    format = readLe16(header_ + 24);
  }

  if (format != kFormatPcm || channels != 1 || bits != 16 || rate != sampleRate_) {
    char expected[48];
    snprintf(expected, sizeof(expected), "expected 16-bit mono PCM at %lu Hz", static_cast<unsigned long>(sampleRate_));
    fail(expected);
    return;
  }

  formatSeen_ = true;
  state_ = State::Skip;
}

void WavStreamParser::endChunk() {
  if (chunkPadded_) {
    chunkPadded_ = false;
    chunkRemaining_ = 1;
    state_ = State::Skip;
    return;
  }

  state_ = State::ChunkHeader;
  headerFill_ = 0;
  headerNeed_ = 8;
}

size_t WavStreamParser::consumePcm(const uint8_t* bytes, size_t length) {
  const bool unbounded = chunkRemaining_ == kUnknownLength;
  const size_t used = unbounded || length < chunkRemaining_ ? length : chunkRemaining_;

  for (size_t i = 0; i < used; i++) {
    if (!hasCarry_) {
      carry_ = bytes[i];
      hasCarry_ = true;
      continue;
    }

    pushSample(static_cast<int16_t>(carry_ | (bytes[i] << 8)));
    hasCarry_ = false;
  }

  if (!unbounded) {
    chunkRemaining_ -= used;
    if (chunkRemaining_ == 0) {
      state_ = State::Trailer;
    }
  }

  return used;
}

void WavStreamParser::pushSample(int16_t sample) {
  batch_[batchFill_++] = sample;

  if (batchFill_ == kBatchSamples) {
    flush();
  }
}

void WavStreamParser::flush() {
  if (batchFill_ == 0) {
    return;
  }

  samplesDelivered_ += batchFill_;
  sink_.onPcm(batch_, batchFill_);
  batchFill_ = 0;
}

void WavStreamParser::fail(const char* message) {
  snprintf(message_, sizeof(message_), "%s", message);
  state_ = State::Failed;
}
