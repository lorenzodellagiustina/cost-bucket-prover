# Command interface of the cost-bucket prover image (entrypoint: make -f $(PREFIX)/Makefile).
# Each target is one External Service Executor command; all paths are container paths.
#
#   instrument  SRCS= TOP_FUNCTION= OUTPUT= [INCLUDES= CFLAGS= WEIGHTS= REPORT=]
#               C sources -> one linked .ll with the cost counter.
#   prove       HARNESS= IR= LO= OUTPUT= [HI= INCLUDES= TIMEOUT= CEX=]
#               Is cost in [LO, HI) reachable? YAML verdict + counterexample harness.
#   replay      HARNESS= IR= CEX= OUTPUT= [INCLUDES= TIMEOUT=]
#               Native run of a counterexample: measured cost and executed inputs.
#   sample      HARNESS= IR= ROWS= OUTPUT= [INCLUDES= TIMEOUT= PRINT_INPUTS= EXE=]
#               Native run of every row of input values: measured cost of each row.
#
# See README.md for the harness contract and the output formats.

PREFIX ?= /opt/cost-bucket-prover

.EXPORT_ALL_VARIABLES:
.PHONY: instrument prove replay sample

instrument:
	@$(PREFIX)/scripts/instrument.sh

prove:
	@$(PREFIX)/scripts/prove.sh

replay:
	@$(PREFIX)/scripts/replay.sh

sample:
	@$(PREFIX)/scripts/sample.sh
