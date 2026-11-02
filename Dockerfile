# Cost-bucket prover: SeaHorn (LLVM 14) + the cost-counter pass + the make interface.
# SeaHorn images exist for amd64 only: build with --platform linux/amd64 (emulated on Apple Silicon).
FROM seahorn/seahorn-llvm14:nightly

ENV PREFIX=/opt/cost-bucket-prover
USER root

COPY pass /tmp/pass
RUN cmake -S /tmp/pass -B /tmp/pass/build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=clang-14 -DCMAKE_CXX_COMPILER=clang++-14 \
      -DLLVM_DIR=/usr/lib/llvm-14/lib/cmake/llvm \
      -DCMAKE_INSTALL_PREFIX=${PREFIX} \
    && cmake --build /tmp/pass/build \
    && cmake --install /tmp/pass/build \
    && rm -rf /tmp/pass

COPY Makefile ${PREFIX}/Makefile
COPY include ${PREFIX}/include
COPY runtime ${PREFIX}/runtime
COPY scripts ${PREFIX}/scripts
RUN chmod +x ${PREFIX}/scripts/*.sh

# `sea` must be on PATH for the scripts.
ENV PATH="/home/usea/seahorn/bin:${PATH}"

RUN mkdir /data && chown usea /data
USER usea
WORKDIR /data

ENTRYPOINT ["make", "-f", "/opt/cost-bucket-prover/Makefile"]
