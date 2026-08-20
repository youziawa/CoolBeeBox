from __future__ import annotations

import argparse
import inspect
from pathlib import Path

import torch

from model import TinyAudioCNN


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--checkpoint", type=str, default="../artifacts/best.pt")
    ap.add_argument("--out", type=str, default="../artifacts/model.onnx")
    ap.add_argument(
        "--use_dynamo",
        action="store_true",
        help="Use the new torch.onnx dynamo exporter (may require onnxscript).",
    )
    args = ap.parse_args()

    ckpt = torch.load(args.checkpoint, map_location="cpu")

    model = TinyAudioCNN(num_classes=2)
    model.load_state_dict(ckpt["state_dict"])
    model.eval()

    input_shape = ckpt.get("input_shape", [1, 1, 128, 128])
    dummy = torch.randn(*input_shape, dtype=torch.float32)

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    export_kwargs = {
        "export_params": True,
        "opset_version": 13,
        "do_constant_folding": True,
        "input_names": ["input"],
        "output_names": ["logits"],
        "dynamic_axes": {"input": {0: "batch"}, "logits": {0: "batch"}},
    }

    # Prefer legacy exporter by default to avoid hard dependency on onnxscript.
    if "dynamo" in inspect.signature(torch.onnx.export).parameters:
        export_kwargs["dynamo"] = args.use_dynamo

    try:
        torch.onnx.export(
            model,
            dummy,
            out_path.as_posix(),
            **export_kwargs,
        )
    except ModuleNotFoundError as e:
        if "onnxscript" in str(e):
            raise ModuleNotFoundError(
                "Missing dependency: onnxscript. Install with `python -m pip install onnxscript`, "
                "or rerun without --use_dynamo to use legacy ONNX export."
            ) from e
        raise
    print(f"Saved ONNX: {out_path}")


if __name__ == "__main__":
    main()

