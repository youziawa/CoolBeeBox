from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple
import re

import numpy as np
import soundfile as sf
import torch
from torch.utils.data import Dataset

LABEL_TO_IDX: Dict[str, int] = {"normal": 0, "stress": 1}


@dataclass
class Sample:
    path: Path
    label: int
    source_id: str


def infer_source_id(file_name: str) -> str:
    """Extract source id to reduce train/val leakage across overlapped slices."""
    stem = Path(file_name).stem
    stem = re.sub(r"_seg\d+_f\d+_(normal|stress)$", "", stem)
    stem = re.sub(r"_(normal|stress)$", "", stem)
    return stem


def scan_dataset(root: Path) -> List[Sample]:
    samples: List[Sample] = []
    for label_name, label_idx in LABEL_TO_IDX.items():
        class_dir = root / label_name
        if not class_dir.exists():
            continue
        for wav in class_dir.glob("*.wav"):
            samples.append(
                Sample(path=wav, label=label_idx, source_id=infer_source_id(wav.name))
            )
    if not samples:
        raise FileNotFoundError(f"No wav files found under: {root}")
    return samples


def _simple_resample(x: np.ndarray, src_sr: int, dst_sr: int) -> np.ndarray:
    if src_sr == dst_sr:
        return x
    src_idx = np.arange(len(x), dtype=np.float32)
    dst_len = int(round(len(x) * (dst_sr / src_sr)))
    dst_idx = np.linspace(0, max(0, len(x) - 1), num=max(dst_len, 1), dtype=np.float32)
    return np.interp(dst_idx, src_idx, x).astype(np.float32)


def _load_mono(path: Path, target_sr: int, num_samples: int) -> np.ndarray:
    audio, sr = sf.read(path.as_posix(), dtype="float32", always_2d=False)
    if audio.ndim == 2:
        audio = audio.mean(axis=1)
    audio = _simple_resample(audio, sr, target_sr)

    if len(audio) < num_samples:
        pad = np.zeros(num_samples - len(audio), dtype=np.float32)
        audio = np.concatenate([audio, pad], axis=0)
    elif len(audio) > num_samples:
        audio = audio[:num_samples]

    peak = np.max(np.abs(audio)) + 1e-8
    audio = audio / peak
    return audio.astype(np.float32)


def waveform_to_logspec(
    waveform: np.ndarray,
    n_fft: int = 512,
    hop_length: int = 160,
    win_length: int = 400,
    out_freq_bins: int = 128,
    out_frames: int = 128,
) -> torch.Tensor:
    x = torch.from_numpy(waveform)
    window = torch.hann_window(win_length)
    spec = torch.stft(
        x,
        n_fft=n_fft,
        hop_length=hop_length,
        win_length=win_length,
        window=window,
        return_complex=True,
    )
    power = spec.abs().pow(2.0)
    log_power = torch.log1p(power)
    log_power = log_power.unsqueeze(0).unsqueeze(0)  # [1,1,F,T]
    resized = torch.nn.functional.interpolate(
        log_power,
        size=(out_freq_bins, out_frames),
        mode="bilinear",
        align_corners=False,
    )
    return resized.squeeze(0)  # [1,F,T]


class BeeAudioDataset(Dataset):
    def __init__(
        self,
        samples: List[Sample],
        sample_rate: int = 16000,
        clip_seconds: float = 2.0,
    ) -> None:
        self.samples = samples
        self.sample_rate = sample_rate
        self.num_samples = int(sample_rate * clip_seconds)

    def __len__(self) -> int:
        return len(self.samples)

    def __getitem__(self, idx: int) -> Tuple[torch.Tensor, torch.Tensor]:
        s = self.samples[idx]
        waveform = _load_mono(s.path, self.sample_rate, self.num_samples)
        feat = waveform_to_logspec(waveform)
        label = torch.tensor(s.label, dtype=torch.long)
        return feat, label

