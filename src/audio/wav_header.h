#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum class WavHeaderResult : uint8_t {
  Ok,
  NotRiffWave,
  Truncated,
  MissingFmt,
  DataBeforeFmt,
  MissingData,
  NotPcm,
  NotMono,
  Not16Bit,
  SampleRateMismatch,
};

inline const char* wavHeaderResultText(WavHeaderResult result) {
  switch (result) {
    case WavHeaderResult::Ok: return "ok";
    case WavHeaderResult::NotRiffWave: return "not RIFF/WAVE";
    case WavHeaderResult::Truncated: return "truncated header";
    case WavHeaderResult::MissingFmt: return "no fmt chunk";
    case WavHeaderResult::DataBeforeFmt: return "data before fmt";
    case WavHeaderResult::MissingData: return "no data chunk";
    case WavHeaderResult::NotPcm: return "not PCM";
    case WavHeaderResult::NotMono: return "not mono";
    case WavHeaderResult::Not16Bit: return "not 16-bit";
    case WavHeaderResult::SampleRateMismatch: return "sample rate mismatch";
  }

  return "invalid";
}

namespace wav_header_detail {

inline uint16_t le16(const uint8_t* bytes) {
  return (uint16_t)(bytes[0] | (bytes[1] << 8));
}

inline uint32_t le32(const uint8_t* bytes) {
  return (uint32_t)bytes[0] |
    ((uint32_t)bytes[1] << 8) |
    ((uint32_t)bytes[2] << 16) |
    ((uint32_t)bytes[3] << 24);
}

}  // namespace wav_header_detail

// Reader needs `bool read(uint8_t* dest, size_t len)` (all bytes or false) and
// `bool skip(uint32_t len)`. On Ok the reader sits at the first PCM byte.
template <typename Reader>
WavHeaderResult readWavHeader(Reader& reader, uint32_t sampleRate) {
  using wav_header_detail::le16;
  using wav_header_detail::le32;

  uint8_t riff[12];

  if (!reader.read(riff, sizeof(riff)) ||
      memcmp(riff, "RIFF", 4) != 0 ||
      memcmp(riff + 8, "WAVE", 4) != 0) {
    return WavHeaderResult::NotRiffWave;
  }

  bool fmtSeen = false;
  uint8_t chunk[8];

  while (reader.read(chunk, sizeof(chunk))) {
    const uint32_t chunkSize = le32(chunk + 4);
    // RIFF pads odd-sized chunks to an even length.
    const uint32_t padding = chunkSize & 1;

    if (memcmp(chunk, "data", 4) == 0) {
      return fmtSeen ? WavHeaderResult::Ok : WavHeaderResult::DataBeforeFmt;
    }

    if (memcmp(chunk, "fmt ", 4) != 0) {
      if (!reader.skip(chunkSize + padding)) {
        return WavHeaderResult::Truncated;
      }

      continue;
    }

    uint8_t fmt[16];

    if (chunkSize < sizeof(fmt) || !reader.read(fmt, sizeof(fmt))) {
      return WavHeaderResult::Truncated;
    }

    if (le16(fmt) != 1) {
      return WavHeaderResult::NotPcm;
    }

    if (le16(fmt + 2) != 1) {
      return WavHeaderResult::NotMono;
    }

    if (le16(fmt + 14) != 16) {
      return WavHeaderResult::Not16Bit;
    }

    if (le32(fmt + 4) != sampleRate) {
      return WavHeaderResult::SampleRateMismatch;
    }

    if (!reader.skip(chunkSize - sizeof(fmt) + padding)) {
      return WavHeaderResult::Truncated;
    }

    fmtSeen = true;
  }

  return fmtSeen ? WavHeaderResult::MissingData : WavHeaderResult::MissingFmt;
}
