from __future__ import annotations

import argparse
import json
import random
from pathlib import Path
from typing import Any, Dict, List

import numpy as np
import onnxruntime as ort
import torch

from dataset import BeeAudioDataset, scan_dataset
from model import TinyAudioCNN


def softmax_np(x: np.ndarray) -> np.ndarray:
    x = x - np.max(x)
    e = np.exp(x)
    return e / (np.sum(e) + 1e-12)


def load_pt_model(checkpoint_path: Path) -> TinyAudioCNN:
    ckpt = torch.load(checkpoint_path.as_posix(), map_location="cpu")
    model = TinyAudioCNN(num_classes=2)
    model.load_state_dict(ckpt["state_dict"])
    model.eval()
    return model


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--checkpoint", type=str, default="../artifacts_full_gpu/best.pt")
    ap.add_argument("--onnx", type=str, default="../artifacts_full_gpu/model.onnx")
    ap.add_argument("--data_root", type=str, default="../source/processed_2class")
    ap.add_argument("--num_samples", type=int, default=64)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--out", type=str, default="../artifacts_full_gpu/consistency_report.json")
    args = ap.parse_args()

    checkpoint = Path(args.checkpoint)
    onnx_path = Path(args.onnx)
    data_root = Path(args.data_root)

    if not checkpoint.exists():
        raise FileNotFoundError(f"Checkpoint not found: {checkpoint}")
    if not onnx_path.exists():
        raise FileNotFoundError(f"ONNX model not found: {onnx_path}")

    samples = scan_dataset(data_root)
    ds = BeeAudioDataset(samples)

    total = min(args.num_samples, len(ds))
    rng = random.Random(args.seed)
    indices = rng.sample(range(len(ds)), k=total)

    pt_model = load_pt_model(checkpoint)
    ort_session = ort.InferenceSession(onnx_path.as_posix(), providers=["CPUExecutionProvider"])
    input_name = ort_session.get_inputs()[0].name

    agree = 0
    max_prob_abs_diff = 0.0
    mean_prob_abs_diff = 0.0
    mismatches: List[Dict[str, Any]] = []

    for idx in indices:
        feat, label = ds[idx]
        x = feat.unsqueeze(0)  # [1,1,128,128]

        with torch.no_grad():
            pt_logits = pt_model(x).squeeze(0).cpu().numpy().astype(np.float64)
        onnx_logits = ort_session.run(None, {input_name: x.cpu().numpy().astype(np.float32)})[0]
        onnx_logits = np.asarray(onnx_logits).squeeze(0).astype(np.float64)

        pt_prob = softmax_np(pt_logits)
        onnx_prob = softmax_np(onnx_logits)

        pt_pred = int(np.argmax(pt_prob))
        onnx_pred = int(np.argmax(onnx_prob))

        if pt_pred == onnx_pred:
            agree += 1

        prob_abs_diff = np.abs(pt_prob - onnx_prob)
        max_prob_abs_diff = max(max_prob_abs_diff, float(np.max(prob_abs_diff)))
        mean_prob_abs_diff += float(np.mean(prob_abs_diff))

        if pt_pred != onnx_pred:
            mismatches.append(
                {
                    "index": idx,
                    "file": samples[idx].path.as_posix(),
                    "label": int(label.item()),
                    "pt_pred": pt_pred,
                    "onnx_pred": onnx_pred,
                    "pt_prob": pt_prob.tolist(),
                    "onnx_prob": onnx_prob.tolist(),
                }
            )

    consistency = agree / max(total, 1)
    mean_prob_abs_diff = mean_prob_abs_diff / max(total, 1)

    report = {
        "num_checked": total,
        "consistency_top1": consistency,
        "mismatch_count": len(mismatches),
        "max_prob_abs_diff": max_prob_abs_diff,
        "mean_prob_abs_diff": mean_prob_abs_diff,
        "mismatches": mismatches,
    }

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(f"Checked samples: {total}")
    print(f"Top-1 consistency: {consistency:.4f}")
    print(f"Mismatch count: {len(mismatches)}")
    print(f"Max prob abs diff: {max_prob_abs_diff:.6f}")
    print(f"Mean prob abs diff: {mean_prob_abs_diff:.6f}")
    print(f"Saved report: {out_path}")


if __name__ == "__main__":
    main()

