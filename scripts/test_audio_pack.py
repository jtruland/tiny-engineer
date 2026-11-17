#!/usr/bin/env python3
"""Host checks for scripts/audio_pack.py. Does not flash."""

import sys
import tempfile
import unittest
import wave
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from audio_pack import (  # noqa: E402
    STOCK_WAVS,
    AudioPackError,
    pack_audio,
    spiffs_size_bytes,
    validate_mod_name,
)

ROOT = Path(__file__).resolve().parents[1]


def write_wav(path, duration_ms, rate=44100, channels=1, width=2):
    frames = rate * duration_ms // 1000
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(width)
        wav.setframerate(rate)
        wav.writeframes(b"\x00" * (frames * channels * width))


def frames_and_rate(path):
    with wave.open(str(path), "rb") as wav:
        return wav.getnframes(), wav.getframerate()


def welcome_cue(end_ms):
    return (
        "greeting_end_ms=100\n"
        "pause_end_ms=200\n"
        "blink_start_ms=120\n"
        "blink_end_ms=160\n"
        f"question_end_ms={end_ms - 50}\n"
        f"end_ms={end_ms}\n"
    )


class AudioPackTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.assets = self.root / "assets"
        self.dest = self.root / "data"
        self.mod = self.root / "mod"
        self.assets.mkdir()
        self.dest.mkdir()
        self.mod.mkdir()
        for name in STOCK_WAVS:
            write_wav(self.assets / name, 500)

    def tearDown(self):
        self.tmp.cleanup()

    def assertPackedFrom(self, packed, source):
        source_frames, _ = frames_and_rate(source)
        self.assertEqual(frames_and_rate(packed), (source_frames // 2, 22050))

    def test_stock_rebuild_drops_old_cue(self):
        (self.dest / "welcome.cue").write_text("end_ms=1\n")
        pack_audio(self.assets, self.dest, 2_000_000)
        self.assertFalse((self.dest / "welcome.cue").exists())

    def test_rebuild_drops_stale_directories(self):
        (self.dest / "ui" / "assets").mkdir(parents=True)
        (self.dest / "ui" / "assets" / "index.js").write_text("stale")
        pack_audio(self.assets, self.dest, 2_000_000)
        self.assertFalse((self.dest / "ui").exists())

    def test_stock_copy_has_no_cues(self):
        pack_audio(self.assets, self.dest, 2_000_000)
        for name in STOCK_WAVS:
            self.assertTrue((self.dest / name).is_file(), name)
        self.assertFalse(list(self.dest.glob("*.cue")))
        self.assertPackedFrom(
            self.dest / "bell.wav",
            self.assets / "bell.wav",
        )

    def test_partial_overlay_keeps_omitted_clips(self):
        write_wav(self.mod / "welcome.wav", 800)
        (self.mod / "welcome.cue").write_text(welcome_cue(800))
        pack_audio(self.assets, self.dest, 2_000_000, self.mod)
        self.assertPackedFrom(
            self.dest / "welcome.wav",
            self.mod / "welcome.wav",
        )
        self.assertPackedFrom(
            self.dest / "bell.wav",
            self.assets / "bell.wav",
        )
        self.assertPackedFrom(
            self.dest / "dead.wav",
            self.assets / "dead.wav",
        )
        self.assertTrue((self.dest / "welcome.cue").is_file())
        self.assertFalse((self.dest / "dead.cue").exists())
        self.assertFalse((self.dest / "bell.cue").exists())

    def test_missing_cue_fails(self):
        write_wav(self.mod / "welcome.wav", 800)
        with self.assertRaises(AudioPackError):
            pack_audio(self.assets, self.dest, 2_000_000, self.mod)

    def test_unknown_wav_fails(self):
        write_wav(self.mod / "extra.wav", 200)
        with self.assertRaises(AudioPackError):
            pack_audio(self.assets, self.dest, 2_000_000, self.mod)

    def test_cue_without_wav_fails(self):
        (self.mod / "welcome.cue").write_text(welcome_cue(500))
        with self.assertRaises(AudioPackError):
            pack_audio(self.assets, self.dest, 2_000_000, self.mod)

    def test_oversize_image_fails(self):
        with self.assertRaises(AudioPackError):
            pack_audio(self.assets, self.dest, 1024)

    def test_mod_name_is_one_segment(self):
        validate_mod_name("halloween")
        with self.assertRaises(AudioPackError):
            validate_mod_name("../assets")
        with self.assertRaises(AudioPackError):
            validate_mod_name("Halloween")

    def test_halloween_overlay_fits_partition(self):
        dest = self.root / "halloween-data"
        dest.mkdir()
        pack_audio(
            ROOT / "assets",
            dest,
            spiffs_size_bytes(ROOT / "partitions.csv"),
            ROOT / "mods" / "halloween" / "assets",
        )
        for name in STOCK_WAVS:
            self.assertTrue((dest / name).is_file(), name)
        self.assertPackedFrom(
            dest / "bell.wav",
            ROOT / "assets" / "bell.wav",
        )
        self.assertPackedFrom(
            dest / "dead.wav",
            ROOT / "assets" / "dead.wav",
        )
        for clip in ("welcome", "attention", "error", "abort"):
            self.assertPackedFrom(
                dest / f"{clip}.wav",
                ROOT / "mods" / "halloween" / "assets" / f"{clip}.wav",
            )
            self.assertTrue((dest / f"{clip}.cue").is_file())
        self.assertFalse((dest / "bell.cue").exists())
        self.assertFalse((dest / "dead.cue").exists())

    def test_halloween_overlay_fits_ota_partition(self):
        pack_audio(
            ROOT / "assets",
            self.dest,
            spiffs_size_bytes(ROOT / "partitions_ota.csv"),
            ROOT / "mods" / "halloween" / "assets",
        )

    def test_stock_wav_at_packed_rate_fails(self):
        write_wav(self.assets / "bell.wav", 500, rate=22050)
        with self.assertRaises(AudioPackError):
            pack_audio(self.assets, self.dest, 2_000_000)


if __name__ == "__main__":
    unittest.main()
