# regoClient
Client software for Serial communication to heatpump controllers based on the Rego6XX control unit

## Building
The Makefile is currently setup for cross-compilation onto a router running OpenWRT. For the compile to work, you need the OpenWRT Toolchain. Before running the Makefile, make sure you set the `PATH` and `STAGING_DIR` environment variables to match the location of the staging dir in your cross-compilation toolchain.
