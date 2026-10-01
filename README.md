# Test EGM-Det

**English:** A desktop Qt 6/C++ benchmark workbench for evaluating dual-modality RGB–infrared object detectors, designed around EGM-Det. It discovers paired images, visualizes oriented boxes, evaluates rotated-IoU precision/recall and COCO-style AP, and exports CSV/PDF reports without blocking the UI.

**Tiếng Việt:** Ứng dụng desktop Qt 6/C++ để kiểm thử mô hình phát hiện vật thể hai luồng RGB–hồng ngoại, thiết kế cho EGM-Det. Công cụ tự ghép cặp ảnh, hiển thị hộp xoay OBB, tính Precision/Recall và AP kiểu COCO bằng rotated IoU, rồi xuất báo cáo CSV/PDF mà không làm treo giao diện.

![Qt 6](https://img.shields.io/badge/Qt-6.4%2B-41CD52) ![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C) ![License](https://img.shields.io/badge/license-MIT-blue)

## Features / Tính năng

- Hand-composed Qt Widgets interface inspired by the supplied dark mockup / giao diện Qt Widgets tối được bố trí thủ công theo ảnh mẫu.
- Batch evaluation and single-pair debug modes / chế độ đánh giá tập dữ liệu và debug một cặp ảnh.
- Paired RGB/IR browsing, synchronized zoom and pan, OBB overlay / duyệt ảnh RGB/IR, zoom đồng bộ và vẽ OBB.
- Rotated IoU, class-aware one-to-one matching, 101-point `AP50`, `AP75`, and `mAP50–95`.
- Background worker with Qt signals/slots; responsive UI during long runs / worker chạy nền, giao diện không bị đóng băng.
- CSV and PDF report export / xuất báo cáo CSV và PDF.
- Weight-free RGB/IR reliability analysis using local entropy, contrast, exposure quality, and modality preference mapping.
- Automatic bundled-model discovery at `models/egm_det.onnx`; users do not browse for a model.

> Weight-free results describe image reliability only. They are not detections, mAP, or EGM-Det inference. Production inference still requires an official trained checkpoint plus ONNX Runtime/TensorRT decoding and rotated NMS.

## Dataset layout / Cấu trúc dữ liệu

```text
dataset/
├── rgb/                 # or RGB, visible, images/rgb
│   ├── img_0001.jpg
│   └── img_0002.jpg
├── ir/                  # or IR, infrared, images/ir
│   ├── img_0001.jpg
│   └── img_0002.jpg
└── labels/              # or annotations, gt, ground_truth
    ├── img_0001.txt
    └── img_0002.txt
```

Each label line is `class cx cy width height angle_degrees`. Coordinates may be pixels or normalized to `[0,1]`:

```text
car 0.42 0.51 0.12 0.08 -14.5
truck 721 404 130 76 7.0
```

RGB and IR files are paired by their base filename. Supported images: JPG, JPEG, PNG and BMP.

LLVIP's native nested layout is also detected automatically. The benchmark prefers `test`, then `val`, then `train` when both modalities contain the same split:

```text
llvip/
├── visible/{train,test}/
└── infrared/{train,test}/
```

You may select the `llvip` root, a modality folder, or its `test` folder; the loader resolves the dataset root automatically.

## Build / Biên dịch

Requirements: CMake 3.21+, a C++17 compiler, and Qt 6.4+ with Widgets and PrintSupport.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/egm-det-benchmark
```

On macOS with Homebrew Qt:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build -j
open build/egm-det-benchmark.app
```

## Production model integration / Tích hợp mô hình thật

Expected model inputs are `input_rgb` and `input_ir`, shaped `[N,C,H,W]`; expected decoded outputs are `[cx,cy,w,h,angle,class scores…]`. Add ONNX Runtime or TensorRT in `CMakeLists.txt`, then add production inference alongside the weight-free analyzer:

1. letterbox preprocessing at 640 or 1024;
2. dual-input inference;
3. output decoding, confidence filtering, and class-aware rotated NMS;
4. optional Modality Gate extraction.

The loader, worker lifecycle, visualizer, evaluator, and exporters do not depend on a particular inference runtime.

## Metric notes / Ghi chú chỉ số

Detections are sorted by confidence and matched once to the highest-IoU unmatched ground truth of the same class and image. AP is sampled at 101 recall points for IoU thresholds `0.50:0.05:0.95`. `Overall` is a macro average across classes; empty-class behavior should be adapted to the exact evaluation protocol used by the target paper.

## Architecture / Kiến trúc

```text
MainWindow (UI thread)
   ├── ImageView × 2 (RGB / IR)
   ├── metrics table + export
   └── signals / slots
          ↓
BenchmarkWorker (QThread)
   ├── DataLoader
   ├── weight-free ReliabilityAnalyzer
   ├── future inference adapter (ONNX/TensorRT)
   └── Evaluator (rotated IoU + COCO AP)
```

## License / Giấy phép

MIT — see [LICENSE](LICENSE).
