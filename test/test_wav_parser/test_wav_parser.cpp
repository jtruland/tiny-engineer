#include <unity.h>

#include <cstring>
#include <vector>

#include "audio/wav_parser.h"

namespace {

constexpr uint32_t kRate = 44100;

struct CollectingSink : WavStreamParser::Sink {
  std::vector<int16_t> samples;

  void onPcm(const int16_t* data, size_t count) override {
    samples.insert(samples.end(), data, data + count);
  }
};

void appendLe16(std::vector<uint8_t>& out, uint16_t value) {
  out.push_back(value & 0xFF);
  out.push_back(value >> 8);
}

void appendLe32(std::vector<uint8_t>& out, uint32_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    out.push_back((value >> shift) & 0xFF);
  }
}

void appendChunk(std::vector<uint8_t>& out, const char* id, const std::vector<uint8_t>& body, uint32_t declaredSize) {
  out.insert(out.end(), id, id + 4);
  appendLe32(out, declaredSize);
  out.insert(out.end(), body.begin(), body.end());
  if (body.size() % 2 != 0) {
    out.push_back(0);
  }
}

std::vector<uint8_t> fmtBody(uint16_t channels, uint32_t rate) {
  std::vector<uint8_t> body;
  appendLe16(body, 1);
  appendLe16(body, channels);
  appendLe32(body, rate);
  appendLe32(body, rate * channels * 2);
  appendLe16(body, channels * 2);
  appendLe16(body, 16);
  return body;
}

std::vector<uint8_t> pcmBytes(const std::vector<int16_t>& samples) {
  std::vector<uint8_t> body;
  for (int16_t sample : samples) {
    appendLe16(body, static_cast<uint16_t>(sample));
  }
  return body;
}

std::vector<uint8_t> wav(
  const std::vector<int16_t>& samples,
  uint32_t rate = kRate,
  uint16_t channels = 1,
  bool streamingDataSize = false
) {
  std::vector<uint8_t> out = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'};
  appendChunk(out, "fmt ", fmtBody(channels, rate), 16);
  const std::vector<uint8_t> pcm = pcmBytes(samples);
  appendChunk(out, "data", pcm, streamingDataSize ? 0 : pcm.size());
  return out;
}

void feedInSlices(WavStreamParser& parser, const std::vector<uint8_t>& bytes, size_t slice) {
  for (size_t offset = 0; offset < bytes.size(); offset += slice) {
    const size_t length = bytes.size() - offset < slice ? bytes.size() - offset : slice;
    parser.feed(bytes.data() + offset, length);
  }
  parser.finish();
}

std::vector<int16_t> ramp(size_t count) {
  std::vector<int16_t> samples;
  for (size_t i = 0; i < count; i++) {
    samples.push_back(static_cast<int16_t>(i * 37 - 20000));
  }
  return samples;
}

}  // namespace

void test_delivers_samples_regardless_of_slicing() {
  const std::vector<int16_t> samples = ramp(1000);
  const std::vector<uint8_t> bytes = wav(samples);

  for (size_t slice : {1u, 3u, 7u, 1436u}) {
    CollectingSink sink;
    WavStreamParser parser(kRate, sink);
    feedInSlices(parser, bytes, slice);

    TEST_ASSERT_FALSE(parser.failed());
    TEST_ASSERT_EQUAL_UINT32(samples.size(), parser.samplesDelivered());
    TEST_ASSERT_EQUAL_INT16_ARRAY(samples.data(), sink.samples.data(), samples.size());
  }
}

void test_skips_unknown_chunks_including_padding() {
  const std::vector<int16_t> samples = ramp(10);
  std::vector<uint8_t> bytes = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'};
  appendChunk(bytes, "LIST", {1, 2, 3}, 3);
  appendChunk(bytes, "fmt ", fmtBody(1, kRate), 16);
  appendChunk(bytes, "junk", {9, 9, 9, 9, 9}, 5);
  const std::vector<uint8_t> pcm = pcmBytes(samples);
  appendChunk(bytes, "data", pcm, pcm.size());

  CollectingSink sink;
  WavStreamParser parser(kRate, sink);
  feedInSlices(parser, bytes, 2);

  TEST_ASSERT_FALSE(parser.failed());
  TEST_ASSERT_EQUAL_INT16_ARRAY(samples.data(), sink.samples.data(), samples.size());
}

void test_ignores_chunks_after_data() {
  const std::vector<int16_t> samples = ramp(8);
  std::vector<uint8_t> bytes = wav(samples);
  appendChunk(bytes, "LIST", {1, 2, 3, 4}, 4);

  CollectingSink sink;
  WavStreamParser parser(kRate, sink);
  feedInSlices(parser, bytes, 5);

  TEST_ASSERT_FALSE(parser.failed());
  TEST_ASSERT_EQUAL_size_t(samples.size(), sink.samples.size());
}

void test_streaming_data_size_plays_until_body_ends() {
  const std::vector<int16_t> samples = ramp(300);

  CollectingSink sink;
  WavStreamParser parser(kRate, sink);
  feedInSlices(parser, wav(samples, kRate, 1, true), 64);

  TEST_ASSERT_FALSE(parser.failed());
  TEST_ASSERT_EQUAL_size_t(samples.size(), sink.samples.size());
}

void test_rejects_other_sample_rate() {
  CollectingSink sink;
  WavStreamParser parser(kRate, sink);
  feedInSlices(parser, wav(ramp(10), 22050), 1436);

  TEST_ASSERT_TRUE(parser.failed());
  TEST_ASSERT_EQUAL_STRING("expected 16-bit mono PCM at 44100 Hz", parser.error());
  TEST_ASSERT_TRUE(sink.samples.empty());
}

void test_rejects_stereo() {
  CollectingSink sink;
  WavStreamParser parser(kRate, sink);
  feedInSlices(parser, wav(ramp(10), kRate, 2), 1436);

  TEST_ASSERT_TRUE(parser.failed());
  TEST_ASSERT_TRUE(sink.samples.empty());
}

void test_rejects_non_wav() {
  const char* text = "{\"name\":\"ring\"}";
  CollectingSink sink;
  WavStreamParser parser(kRate, sink);
  parser.feed(reinterpret_cast<const uint8_t*>(text), strlen(text));
  parser.finish();

  TEST_ASSERT_EQUAL_STRING("not a WAV file", parser.error());
}

void test_body_shorter_than_header_is_not_wav() {
  const uint8_t bytes[] = {'h', 'e', 'l', 'l', 'o'};
  CollectingSink sink;
  WavStreamParser parser(kRate, sink);
  parser.feed(bytes, sizeof(bytes));
  parser.finish();

  TEST_ASSERT_EQUAL_STRING("not a WAV file", parser.error());
}

void test_body_without_data_chunk_fails() {
  std::vector<uint8_t> bytes = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'};
  appendChunk(bytes, "fmt ", fmtBody(1, kRate), 16);

  CollectingSink sink;
  WavStreamParser parser(kRate, sink);
  feedInSlices(parser, bytes, 1436);

  TEST_ASSERT_EQUAL_STRING("no audio data", parser.error());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_delivers_samples_regardless_of_slicing);
  RUN_TEST(test_skips_unknown_chunks_including_padding);
  RUN_TEST(test_ignores_chunks_after_data);
  RUN_TEST(test_streaming_data_size_plays_until_body_ends);
  RUN_TEST(test_rejects_other_sample_rate);
  RUN_TEST(test_rejects_stereo);
  RUN_TEST(test_rejects_non_wav);
  RUN_TEST(test_body_shorter_than_header_is_not_wav);
  RUN_TEST(test_body_without_data_chunk_fails);
  return UNITY_END();
}
