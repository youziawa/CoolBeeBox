from __future__ import annotations

import math
import shutil
import subprocess
import sys
from pathlib import Path

import numpy as np
import soundfile as sf


def make_tone(freq: float, seconds: float = 2.0, sr: int = 16000) -> np.ndarray:
    t = np.linspace(0, seconds, int(sr * seconds), endpoint=False, dtype=np.float32)
    return 0.2 * np.sin(2.0 * math.pi * freq * t).astype(np.float32)


def build_fake_dataset(root: Path) -> None:
    rng = np.random.default_rng(42)
    for cls in ["normal", "stress"]:
        d = root / cls
        d.mkdir(parents=True, exist_ok=True)
        base_freq = 220 if cls == "normal" else 440
        for i in range(20):
            sig = make_tone(base_freq + rng.uniform(-8, 8))
            sig += 0.02 * rng.normal(size=sig.shape).astype(np.float32)
            sf.write((d / f"fake_{cls}_{i:03d}.wav").as_posix(), sig, 16000)


def run() -> None:
    this_dir = Path(__file__).resolve().parent
    tmp_root = this_dir / "_tmp_fake_data"
    out_dir = this_dir / "_tmp_artifacts"

    if tmp_root.exists():
        shutil.rmtree(tmp_root)
    if out_dir.exists():
        shutil.rmtree(out_dir)

    build_fake_dataset(tmp_root)

    cmd = [
        sys.executable,
        (this_dir / "train.py").as_posix(),
        "--data_root",
        tmp_root.as_posix(),
        "--out_dir",
        out_dir.as_posix(),
        "--epochs",
        "1",
        "--batch_size",
        "8",
    ]
    print("Running:", " ".join(cmd))
    subprocess.check_call(cmd)

    best = out_dir / "best.pt"
    if not best.exists():
        raise RuntimeError("Smoke test failed: best.pt not generated")
    print("Smoke test ok:", best)


if __name__ == "__main__":
    run()

