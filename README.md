# cov

## 系統需求

### 依賴函式庫
- ICU (Unicode 處理)
- OpenCV (圖像處理)
- libarchive (檔案壓縮)
- wxWidgets (GUI，僅 macOS)

### 建置環境
- CMake 3.20+
- C++20 編譯器
- Clang

## 快速開始

### 建置 (recommand)

提供 `bu` 腳本，**建議使用此腳本進行所有建置操作**，除非你明確知道自己在執行什麼指令。

```bash
./bu

./bu build

# 指定平台建置
./bu mac      # macOS
./bu linux    # Linux

# 執行測試
./bu test

# Docker 建置 (cross build)
./bu docker
```

### 手動建置 (進階用途)

**初次建置** (必須先建構 aacgain 子專案):
```bash
cmake --build ./build --target aacgain
```

**標準建置流程**:
```bash
cd build
cmake ..
make VERBOSE=1
```

**Linux 環境手動建置**:
```bash
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ..
cmake --build . --target cov
```


## 測試

**使用建置腳本 (推薦)**:
```bash
./bu test

./bu test test_base
```

**手動執行測試**:
```bash
# 建置測試
cmake -DBUILD_TEST=ON ..
make

# 執行測試
./test_base
```
