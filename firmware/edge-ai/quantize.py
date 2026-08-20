from __future__ import annotations

import argparse
import sys
import wave
from pathlib import Path

import numpy as np
import torch
import torch.nn.functional as F
from torch.utils.data import DataLoader, Dataset

LOCAL_ESPPQ_ZIP = Path(__file__).with_name("esp-ppq.zip")
if LOCAL_ESPPQ_ZIP.exists():
    sys.path.insert(0, LOCAL_ESPPQ_ZIP.as_posix() + "/esp-ppq-master")

from esp_ppq.api import espdl_quantize_onnx

DEVICE = "cpu"
BATCH_SIZE = 1
INPUT_SHAPE = [1, 128, 128]
TARGET = "esp32s3"
NUM_OF_BITS = 8


def simple_resample(x: np.ndarray, src_sr: int, dst_sr: int) -> np.ndarray:
    if src_sr == dst_sr:
        return x
    src_idx = np.arange(len(x), dtype=np.float32)
    dst_len = int(round(len(x) * (dst_sr / src_sr)))
    dst_idx = np.linspace(0, max(0, len(x) - 1), num=max(dst_len, 1), dtype=np.float32)
    return np.interp(dst_idx, src_idx, x).astype(np.float32)


def read_pcm_wav(path: Path) -> tuple[np.ndarray, int]:
    with wave.open(path.as_posix(), "rb") as wf:
        channels = wf.getnchannels()
        sample_width = wf.getsampwidth()
        sr = wf.getframerate()
        frames = wf.readframes(wf.getnframes())

    if sample_width == 1:
        audio = (np.frombuffer(frames, dtype=np.uint8).astype(np.float32) - 128.0) / 128.0
    elif sample_width == 2:
        audio = np.frombuffer(frames, dtype="<i2").astype(np.float32) / 32768.0
    elif sample_width == 3:
        raw = np.frombuffer(frames, dtype=np.uint8).reshape(-1, 3)
        vals = (
            raw[:, 0].astype(np.int32)
            | (raw[:, 1].astype(np.int32) << 8)
            | (raw[:, 2].astype(np.int32) << 16)
        )
        vals = np.where(vals & 0x800000, vals | ~0xFFFFFF, vals)
        audio = vals.astype(np.float32) / 8388608.0
    elif sample_width == 4:
        audio = np.frombuffer(frames, dtype="<i4").astype(np.float32) / 2147483648.0
    else:
        raise ValueError(f"Unsupported WAV sample width {sample_width}: {path}")

    if channels > 1:
        audio = audio.reshape(-1, channels).mean(axis=1)
    return audio.astype(np.float32), sr


def load_mono(path: Path, target_sr: int = 16000, num_samples: int = 32000) -> np.ndarray:
    audio, sr = read_pcm_wav(path)
    audio = simple_resample(audio, sr, target_sr)

    if len(audio) < num_samples:
        audio = np.concatenate([audio, np.zeros(num_samples - len(audio), dtype=np.float32)])
    elif len(audio) > num_samples:
        audio = audio[:num_samples]

    peak = np.max(np.abs(audio)) + 1e-8
    return (audio / peak).astype(np.float32)


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
    log_power = log_power.unsqueeze(0).unsqueeze(0)
    resized = F.interpolate(
        log_power,
        size=(out_freq_bins, out_frames),
        mode="bilinear",
        align_corners=False,
    )
    return resized.squeeze(0)


class AudioCalibDataset(Dataset):
    def __init__(self, calib_list_file: Path) -> None:
        self.file_paths = [
            Path(line.strip().lstrip("\ufeff"))
            for line in calib_list_file.read_text(encoding="utf-8-sig").splitlines()
            if line.strip().lstrip("\ufeff")
        ]
        if not self.file_paths:
            raise FileNotFoundError(f"No calibration files listed in: {calib_list_file}")

    def __len__(self) -> int:
        return len(self.file_paths)

    def __getitem__(self, idx: int) -> torch.Tensor:
        waveform = load_mono(self.file_paths[idx])
        return waveform_to_logspec(waveform).to(DEVICE)


def collate_fn(batch: list[torch.Tensor]) -> torch.Tensor:
    return torch.stack(batch).to(DEVICE)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base_dir", type=str, default="artifacts_full_gpu")
    ap.add_argument("--onnx", type=str, default=None)
    ap.add_argument("--calib_list", type=str, default=None)
    ap.add_argument("--out", type=str, default=None)
    ap.add_argument("--calib_steps", type=int, default=64)
    args = ap.parse_args()

    base_dir = Path(args.base_dir)
    onnx_model_path = Path(args.onnx) if args.onnx else base_dir / "model.onnx"
    calib_list_path = Path(args.calib_list) if args.calib_list else base_dir / "calib_list.txt"
    espdl_export_path = Path(args.out) if args.out else base_dir / "model.espdl"

    print("Preparing log-spectrogram calibration dataloader...")
    calib_dataset = AudioCalibDataset(calib_list_path)
    calib_dataloader = DataLoader(
        calib_dataset,
        batch_size=BATCH_SIZE,
        shuffle=False,
        collate_fn=collate_fn,
    )
    test_input = calib_dataset[0].unsqueeze(0)

    print(f"Start ESP-DL quantization: target={TARGET}, bits={NUM_OF_BITS}")
    espdl_quantize_onnx(
        onnx_import_file=onnx_model_path.as_posix(),
        espdl_export_file=espdl_export_path.as_posix(),
        calib_dataloader=calib_dataloader,
        calib_steps=args.calib_steps,
        input_shape=[1] + INPUT_SHAPE,
        inputs=[test_input],
        target=TARGET,
        num_of_bits=NUM_OF_BITS,
        device=DEVICE,
        error_report=True,
        skip_export=False,
        export_test_values=True,
        verbose=1,
        dispatching_override=None,
    )

    print(f"Quantization finished: {espdl_export_path}")


if __name__ == "__main__":
    main()
