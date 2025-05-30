FROM ubuntu:22.04

RUN sed -i 's|http://archive.ubuntu.com|http://mirror.yandex.ru|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirror.yandex.ru|g' /etc/apt/sources.list

    
RUN apt-get update || apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    python3 \
    python3-pip \
    python3-venv \
    nlohmann-json3-dev \
    gnuplot \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxcb-xinput0 \
    libxcb-xkb1 \
    qt5-qmake \
    qtbase5-dev \
    libqt5gui5 \
    libqt5core5a \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"
ENV QT_QPA_PLATFORM=offscreen

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

RUN git clone https://github.com/libcpr/cpr.git && \
    cd cpr && mkdir build && cd build && \
    cmake .. -DCPR_USE_SYSTEM_CURL=ON && \
    make && make install && \
    cd ../.. && rm -rf cpr

RUN git clone https://github.com/alandefreitas/matplotplusplus.git && \
    cd matplotplusplus && mkdir build && cd build && \
    cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF && \
    make && make install && \
    cd ../.. && rm -rf matplotplusplus

COPY CMakeLists.txt /app/
COPY src/cpp /app/src/cpp

RUN mkdir -p /app/bin && \
    mkdir -p /app/build && cd /app/build && \
    cmake .. && make && \
    cp src/cpp/parsers/parser /app/bin/ && \
    cp src/cpp/visualization/visualization /app/bin/

COPY . /app
WORKDIR /app

CMD ["bash", "scripts/run_pipeline.sh"]