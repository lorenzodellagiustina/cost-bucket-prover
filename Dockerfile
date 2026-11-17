# Cost-bucket prover: SeaHorn (LLVM 14) + the cost-counter pass + the make interface.
# SeaHorn images exist for amd64 only: build with --platform linux/amd64 (emulated on Apple
# Silicon).
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

# A newer Z3, beside SeaHorn's own (4.8.9), for `prove` with PROVE_SOLVER=gspacer or portfolio:
# Spacer with global guidance (`fp.spacer.global`, Z3 4.8.10 and later). 4.13.4 fails on some
# queries with global guidance ("mbp to-real"), 5.3.2 does not. Not on PATH, not to shadow
# SeaHorn's.
RUN pip3 install --no-cache-dir --prefix /opt/z3 z3-solver==5.3.2.0
ENV Z3_GSPACER=/opt/z3/local/bin/z3

# The solver of `prove`: `sea` (SeaHorn's Spacer, the default), `gspacer` or `portfolio` (read
# scripts/prove.sh).
# A build with `--build-arg PROVE_SOLVER=gspacer` changes the default, so a workflow that calls
# `prove` can compare the two by image tag, unchanged.
ARG PROVE_SOLVER=sea
ENV PROVE_SOLVER=${PROVE_SOLVER}

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
