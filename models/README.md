# Bundled model location

Place the production EGM-Det export here as:

```text
models/egm_det.onnx
```

The application discovers this file automatically; end users only select the dataset. A model binary is not included yet because no trained EGM-Det ONNX export was available in the project workspace. Do not rename an unrelated ONNX model: its two-input/output contract must match the decoder.

For GitHub files above 100 MiB, install Git LFS and track the model before committing:

```bash
git lfs install
git lfs track "models/*.onnx"
git add .gitattributes models/egm_det.onnx
```

## Tiếng Việt

Đặt model EGM-Det thật tại `models/egm_det.onnx`. Ứng dụng sẽ tự tìm file này nên người dùng chỉ cần chọn dataset. Hiện chưa đính kèm model nhị phân vì workspace không có bản ONNX đã huấn luyện; không nên dùng một model ONNX không đúng kiến trúc chỉ để thay thế.
