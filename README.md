# regoClient

Client software for serial communication with heat-pump controllers based on the Rego6XX control unit.

## Project structure

The repository is organised into a few small directories:

- `src/` – command-line client and helper libraries for the protocol and serial I/O.
- `include/` – header files shared between modules.
- `tests/` – unit tests for low-level serial packet helpers.

## Usage

The `regoClient` binary communicates with the controller over a serial port. It can read individual registers, dump known register ranges or display the controller's LCD contents. Run the program without arguments to see a full list of commands and options.

## Building

The Makefile is configured for cross-compiling to an OpenWRT router. Before running `make`, set the `PATH` and `STAGING_DIR` environment variables to point at your OpenWRT toolchain.

To build and run the tests on a development machine:

```sh
make test
```

