#include <unity.h>

#include <vector>

#include "audio/wav_header.h"

namespace {

constexpr uint32_t kRate = 22050;

struct BufferReader {
  const std::vector<uint8_t>& bytes;
  size_t position = 0;

  bool read(uint8_t* dest, size_t len) {
    if (position + len > bytes.size()) {
      return false;
    }

    memcpy(dest, bytes.data() + position, len);
    position += len;
    return true;
  }

  bool skip(uint32_t len) {
    if (position + len > bytes.size()) {
      return false;
    }

    position += len;
    return true;
  }
};

void put16(std::vector<uint8_t>& out, uint16_t value) {
  out.push_back(value & 0xFF);
  out.push_back(value >> 8);
}

void put32(std::vector<uint8_t>& out, uint32_t value) {
  put16(out, value & 0xFFFF);
  put16(out, value >> 16);
}

void putChunk(std::vector<uint8_t>& out, const char* id, const std::vector<uint8_t>& body) {
  out.insert(out.end(), id, id + 4);
  put32(out, body.size());
  out.insert(out.end(), body.begin(), body.end());

  if (body.size() & 1) {
    out.push_back(0);
  }
}

std::vector<uint8_t> fmtBody(uint16_t format, uint16_t channels, uint32_t rate, uint16_t bits) {
  std::vector<uint8_t> body;
  put16(body, format);
  put16(body, channels);
  put32(body, rate);
  put32(body, rate * channels * bits / 8);
  put16(body, channels * bits / 8);
  put16(body, bits);
  return body;
}

std::vector<uint8_t> riffWave() {
  std::vector<uint8_t> out = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'};
  return out;
}

const std::vector<uint8_t> kPcm = {0x34, 0x12, 0x78, 0x56};

WavHeaderResult parse(const std::vector<uint8_t>& bytes, size_t* dataOffset = nullptr) {
  BufferReader reader{bytes};
  const WavHeaderResult result = readWavHeader(reader, kRate);

  if (dataOffset != nullptr) {
    *dataOffset = reader.position;
  }

  return result;
}

}  // namespace

void test_valid_header_stops_at_pcm() {
  std::vector<uint8_t> wav = riffWave();
  putChunk(wav, "fmt ", fmtBody(1, 1, kRate, 16));
  const size_t pcmStart = wav.size() + 8;
  putChunk(wav, "data", kPcm);

  size_t offset = 0;
  TEST_ASSERT_EQUAL(WavHeaderResult::Ok, parse(wav, &offset));
  TEST_ASSERT_EQUAL(pcmStart, offset);
}

void test_rate_mismatch_rejected() {
  std::vector<uint8_t> wav = riffWave();
  putChunk(wav, "fmt ", fmtBody(1, 1, 44100, 16));
  putChunk(wav, "data", kPcm);
  TEST_ASSERT_EQUAL(WavHeaderResult::SampleRateMismatch, parse(wav));
}

void test_format_mismatches_rejected() {
  std::vector<uint8_t> stereo = riffWave();
  putChunk(stereo, "fmt ", fmtBody(1, 2, kRate, 16));
  putChunk(stereo, "data", kPcm);
  TEST_ASSERT_EQUAL(WavHeaderResult::NotMono, parse(stereo));

  std::vector<uint8_t> eightBit = riffWave();
  putChunk(eightBit, "fmt ", fmtBody(1, 1, kRate, 8));
  putChunk(eightBit, "data", kPcm);
  TEST_ASSERT_EQUAL(WavHeaderResult::Not16Bit, parse(eightBit));

  std::vector<uint8_t> floatPcm = riffWave();
  putChunk(floatPcm, "fmt ", fmtBody(3, 1, kRate, 16));
  putChunk(floatPcm, "data", kPcm);
  TEST_ASSERT_EQUAL(WavHeaderResult::NotPcm, parse(floatPcm));
}

void test_odd_and_extended_chunks_skipped() {
  std::vector<uint8_t> wav = riffWave();
  putChunk(wav, "LIST", {1, 2, 3});
  std::vector<uint8_t> fmt = fmtBody(1, 1, kRate, 16);
  fmt.push_back(0);
  fmt.push_back(0);
  putChunk(wav, "fmt ", fmt);
  putChunk(wav, "junk", {9});
  const size_t pcmStart = wav.size() + 8;
  putChunk(wav, "data", kPcm);

  size_t offset = 0;
  TEST_ASSERT_EQUAL(WavHeaderResult::Ok, parse(wav, &offset));
  TEST_ASSERT_EQUAL(pcmStart, offset);
}

void test_data_before_fmt_rejected() {
  std::vector<uint8_t> wav = riffWave();
  putChunk(wav, "data", kPcm);
  putChunk(wav, "fmt ", fmtBody(1, 1, kRate, 16));
  TEST_ASSERT_EQUAL(WavHeaderResult::DataBeforeFmt, parse(wav));
}

void test_missing_or_short_fmt_rejected() {
  std::vector<uint8_t> noFmt = riffWave();
  putChunk(noFmt, "LIST", {1, 2});
  TEST_ASSERT_EQUAL(WavHeaderResult::MissingFmt, parse(noFmt));

  std::vector<uint8_t> shortFmt = riffWave();
  putChunk(shortFmt, "fmt ", {1, 0, 1, 0});
  TEST_ASSERT_EQUAL(WavHeaderResult::Truncated, parse(shortFmt));

  std::vector<uint8_t> cutFmt = riffWave();
  putChunk(cutFmt, "fmt ", fmtBody(1, 1, kRate, 16));
  cutFmt.resize(cutFmt.size() - 4);
  TEST_ASSERT_EQUAL(WavHeaderResult::Truncated, parse(cutFmt));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_valid_header_stops_at_pcm);
  RUN_TEST(test_rate_mismatch_rejected);
  RUN_TEST(test_format_mismatches_rejected);
  RUN_TEST(test_odd_and_extended_chunks_skipped);
  RUN_TEST(test_data_before_fmt_rejected);
  RUN_TEST(test_missing_or_short_fmt_rejected);
  return UNITY_END();
}
