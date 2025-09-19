FROM --platform=linux/amd64 ubuntu:24.04

# 避免在安裝過程中出現交互式提示
ENV DEBIAN_FRONTEND=noninteractive

# 先修復證書問題
RUN apt-get update && apt-get install -y --reinstall ca-certificates && update-ca-certificates

# 安裝 Clang 和必要的開發工具
RUN apt-get install -y \
    clang build-essential cmake ninja-build cmake-extras \
    autoconf automake libtool \
    libc6-dev \
    libarchive-dev libopencv-dev libicu-dev

# 將 Clang 設置為預設編譯器
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100 \
    && update-alternatives --set cc /usr/bin/clang \
    && update-alternatives --set c++ /usr/bin/clang++

# 設置工作目錄
WORKDIR /app

# 設置容器啟動時執行的命令，使容器保持運行
CMD ["tail", "-f", "/dev/null"]