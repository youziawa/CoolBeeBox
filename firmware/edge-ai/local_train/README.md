# Local Training (PyTorch) for BeeGuard

This folder provides a minimal local training pipeline for 2-class audio classification:

- `normal` -> class `0`
- `stress` -> class `1`

It is designed to work with your existing dataset outputs under `source/`:

- `source/processed_2class/`
- `source/processed_2class_subset_2h/`

## 1) Install dependencies

```powershell
conda create -n EdgeAI-gpu-new python=3.11 -y
conda activate EdgeAI-gpu-new

cd "D:\Lab\ESP\EdgeAI\local_train"

# Install a newer CUDA build that supports RTX 50-series (sm_120)
python -m pip install --upgrade pip
python -m pip install --pre torch torchvision torchaudio --index-url https://download.pytorch.org/whl/nightly/cu128

# Then install the remaining Python packages (no torch in requirements.txt)
python -m pip install -r requirements.txt
```

Verify GPU is available:

```powershell
python -c "import torch; print(torch.__version__); print(torch.version.cuda); print(torch.cuda.is_available()); print(torch.cuda.get_device_name(0) if torch.cuda.is_available() else 'N/A')"
```

## 2) Run training (quick start)

```powershell
cd "D:\Lab\ESP\EdgeAI\local_train"
python train.py --data_root "..\source\processed_2class_subset_2h" --out_dir "..\artifacts" --epochs 10 --batch_size 32

# Full dataset (final training)
python train.py --data_root "..\source\processed_2class" --out_dir "..\artifacts_full" --epochs 30 --batch_size 32

# Force CPU if your CUDA build is not compatible with your GPU architecture
python train.py --data_root "..\source\processed_2class" --out_dir "..\artifacts_full" --epochs 30 --batch_size 16 --device cpu
```

Outputs:

- `..\artifacts\best.pt`
- `..\artifacts\train_report.json`

## 3) Export ONNX

```powershell
cd "D:\Lab\ESP\EdgeAI\local_train"
python export_onnx.py --checkpoint "..\artifacts\best.pt" --out "..\artifacts\model.onnx"
```

If you want to use the newer dynamo exporter explicitly:

```powershell
cd "D:\Lab\ESP\EdgeAI\local_train"
python export_onnx.py --checkpoint "..\artifacts\best.pt" --out "..\artifacts\model.onnx" --use_dynamo
```

Output:

- `..\artifacts\model.onnx`

## 4) Smoke test (synthetic data)

Runs one epoch on generated fake audio samples.

```powershell
cd "D:\Lab\ESP\EdgeAI\local_train"
python smoke_test.py
```

## 5) PyTorch vs ONNX consistency check

Run this after exporting ONNX to verify prediction consistency.

```powershell
cd "D:\Lab\ESP\EdgeAI\local_train"
python check_pt_onnx_consistency.py --checkpoint "..\artifacts_full_gpu\best.pt" --onnx "..\artifacts_full_gpu\model.onnx" --data_root "..\source\processed_2class" --num_samples 64 --out "..\artifacts_full_gpu\consistency_report.json"
```

Output:

- `..\artifacts_full_gpu\consistency_report.json`

## Notes

- Keep input assumptions fixed for deployment consistency: `16kHz`, `2s` clips.
- `train.py` uses source-id grouping split to reduce leakage from overlapped slices.
- Start with subset data to iterate quickly, then switch to full `source/processed_2class`.
- If you see `ModuleNotFoundError: onnxscript`, install dependencies again or run export without `--use_dynamo`.
- If CUDA reports `no kernel image is available`, run training with `--device cpu` or upgrade to a CUDA build that supports your GPU SM version.
- For RTX 50-series GPUs, prefer `torch nightly + cu128` because older `cu121` builds may not include `sm_120` kernels.
- Before ESP deployment, run consistency check and ensure `consistency_top1` is close to `1.0`.

