from __future__ import annotations

import argparse
import re
import wave
from pathlib import Path


BEGIN_RE = re.compile(r"PCM_DUMP_BEGIN\s+index=(\d+)\s+sample_rate=(\d+)\s+samples=(\d+)")
END_RE = re.compile(r"PCM_DUMP_END\s+index=(\d+)")
HEX_PREFIX = "PCM_DUMP_HEX "


def write_wav(path: Path, pcm: bytes, sample_rate: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(path.as_posix(), "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.writeframes(pcm)


def extract(log_path: Path, out_dir: Path, label: str) -> int:
    current_index: int | None = None
    current_sample_rate = 16000
    current_hex: list[str] = []
    written = 0

    for raw_line in log_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw_line.strip()
        begin = BEGIN_RE.search(line)
        if begin:
            current_index = int(begin.group(1))
            current_sample_rate = int(begin.group(2))
            current_hex = []
            continue

        if current_index is not None and HEX_PREFIX in line:
            current_hex.append(line.split(HEX_PREFIX, 1)[1].strip())
            continue

        end = END_RE.search(line)
        if end and current_index is not None:
            pcm = bytes.fromhex("".join(current_hex))
            wav_path = out_dir / label / f"board_{label}_{current_index:04d}.wav"
            write_wav(wav_path, pcm, current_sample_rate)
            print(f"wrote {wav_path} ({len(pcm)} bytes)")
            written += 1
            current_index = None
            current_hex = []

    return written


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--log", required=True, type=Path)
    ap.add_argument("--out_dir", default=Path(r"D:\Lab\ESP\EdgeAI\source\processed_board"), type=Path)
    ap.add_argument("--label", required=True, choices=["normal", "stress"])
    args = ap.parse_args()

    count = extract(args.log, args.out_dir, args.label)
    print(f"done, extracted {count} wav files")


if __name__ == "__main__":
    main()
