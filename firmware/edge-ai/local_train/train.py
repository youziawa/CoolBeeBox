from __future__ import annotations

import argparse
import json
import random
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Tuple

import numpy as np
import torch
from torch import nn
from torch.utils.data import DataLoader
from tqdm import tqdm

from dataset import BeeAudioDataset, LABEL_TO_IDX, Sample, scan_dataset
from model import TinyAudioCNN


def seed_everything(seed: int) -> None:
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    torch.cuda.manual_seed_all(seed)


def split_by_source(
    samples: List[Sample], val_ratio: float, seed: int
) -> Tuple[List[Sample], List[Sample]]:
    by_source: Dict[str, List[Sample]] = defaultdict(list)
    for s in samples:
        by_source[s.source_id].append(s)

    source_keys = list(by_source.keys())
    rng = random.Random(seed)
    rng.shuffle(source_keys)

    val_count = max(1, int(len(source_keys) * val_ratio))
    val_keys = set(source_keys[:val_count])

    train_samples: List[Sample] = []
    val_samples: List[Sample] = []
    for k, grouped in by_source.items():
        if k in val_keys:
            val_samples.extend(grouped)
        else:
            train_samples.extend(grouped)

    return train_samples, val_samples


def build_loaders(
    data_root: Path,
    batch_size: int,
    val_ratio: float,
    seed: int,
) -> Tuple[DataLoader, DataLoader]:
    all_samples = scan_dataset(data_root)
    train_samples, val_samples = split_by_source(all_samples, val_ratio, seed)

    train_ds = BeeAudioDataset(train_samples)
    val_ds = BeeAudioDataset(val_samples)

    train_loader = DataLoader(train_ds, batch_size=batch_size, shuffle=True, num_workers=0)
    val_loader = DataLoader(val_ds, batch_size=batch_size, shuffle=False, num_workers=0)
    return train_loader, val_loader


def calc_metrics(logits: torch.Tensor, labels: torch.Tensor) -> Dict[str, float]:
    preds = logits.argmax(dim=1)
    correct = (preds == labels).sum().item()
    total = labels.numel()

    tp = ((preds == 1) & (labels == 1)).sum().item()
    fp = ((preds == 1) & (labels == 0)).sum().item()
    fn = ((preds == 0) & (labels == 1)).sum().item()

    precision = tp / (tp + fp + 1e-8)
    recall = tp / (tp + fn + 1e-8)
    f1 = 2 * precision * recall / (precision + recall + 1e-8)
    acc = correct / max(total, 1)

    return {
        "acc": float(acc),
        "precision_stress": float(precision),
        "recall_stress": float(recall),
        "f1_stress": float(f1),
    }


def run_epoch(
    model: nn.Module,
    loader: DataLoader,
    criterion: nn.Module,
    device: torch.device,
    optimizer: torch.optim.Optimizer | None,
) -> Tuple[float, Dict[str, float]]:
    train_mode = optimizer is not None
    model.train(train_mode)

    running_loss = 0.0
    all_logits: List[torch.Tensor] = []
    all_labels: List[torch.Tensor] = []

    for x, y in tqdm(loader, leave=False):
        x = x.to(device)
        y = y.to(device)

        logits = model(x)
        loss = criterion(logits, y)

        if train_mode:
            optimizer.zero_grad(set_to_none=True)
            loss.backward()
            optimizer.step()

        running_loss += loss.item() * y.size(0)
        all_logits.append(logits.detach().cpu())
        all_labels.append(y.detach().cpu())

    epoch_loss = running_loss / max(len(loader.dataset), 1)
    metrics = calc_metrics(torch.cat(all_logits, 0), torch.cat(all_labels, 0))
    return epoch_loss, metrics


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--data_root", type=str, default="../source/processed_2class_subset_2h")
    ap.add_argument("--out_dir", type=str, default="../artifacts")
    ap.add_argument("--device", type=str, default="auto", choices=["auto", "cpu", "cuda"])
    ap.add_argument("--epochs", type=int, default=10)
    ap.add_argument("--batch_size", type=int, default=32)
    ap.add_argument("--lr", type=float, default=1e-3)
    ap.add_argument("--val_ratio", type=float, default=0.2)
    ap.add_argument("--seed", type=int, default=42)
    args = ap.parse_args()

    seed_everything(args.seed)
    if args.device == "cpu":
        device = torch.device("cpu")
    elif args.device == "cuda":
        if not torch.cuda.is_available():
            raise RuntimeError("--device=cuda requested, but CUDA is not available")
        device = torch.device("cuda")
    else:
        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

    print(f"Using device: {device}")

    train_loader, val_loader = build_loaders(
        data_root=Path(args.data_root),
        batch_size=args.batch_size,
        val_ratio=args.val_ratio,
        seed=args.seed,
    )

    model = TinyAudioCNN(num_classes=len(LABEL_TO_IDX)).to(device)
    criterion = nn.CrossEntropyLoss()
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    best_f1 = -1.0
    best_path = out_dir / "best.pt"

    for epoch in range(1, args.epochs + 1):
        tr_loss, tr_metrics = run_epoch(model, train_loader, criterion, device, optimizer)
        va_loss, va_metrics = run_epoch(model, val_loader, criterion, device, optimizer=None)

        print(
            f"Epoch {epoch:02d} | "
            f"train_loss={tr_loss:.4f} val_loss={va_loss:.4f} | "
            f"val_acc={va_metrics['acc']:.4f} val_f1_stress={va_metrics['f1_stress']:.4f}"
        )

        if va_metrics["f1_stress"] > best_f1:
            best_f1 = va_metrics["f1_stress"]
            ckpt = {
                "state_dict": model.state_dict(),
                "label_to_idx": LABEL_TO_IDX,
                "input_shape": [1, 1, 128, 128],
                "sample_rate": 16000,
                "clip_seconds": 2.0,
                "best_val_f1_stress": best_f1,
            }
            torch.save(ckpt, best_path)

    report = {
        "best_val_f1_stress": best_f1,
        "checkpoint": best_path.as_posix(),
        "labels": LABEL_TO_IDX,
    }
    (out_dir / "train_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"Saved: {best_path}")
    print(f"Saved: {(out_dir / 'train_report.json')}")


if __name__ == "__main__":
    main()

