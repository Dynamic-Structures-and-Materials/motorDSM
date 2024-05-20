# motorDSM
EPICS motor drivers for the following [Dynamic Structures and Materials](https://www.dynamic-structures.com/) motor controllers: MD-90

[![Build Status](https://github.com/Binary-Coalescence/motorDSM/actions/workflows/ci-scripts-build.yml/badge.svg)](https://github.com/Binary-Coalescence/motorDSM/actions/workflows/ci-scripts-build.yml)

motorDSM is a submodule of [motor](https://github.com/epics-modules/motor).  When motorDSM is built in the ``motor/modules`` directory, no manual configuration is needed.

motorDSM can also be built outside of motor by copying it's ``EXAMPLE_RELEASE.local`` file to ``RELEASE.local`` and defining the paths to ``MOTOR`` and itself.

motorDSM contains an example IOC that is built if ``CONFIG_SITE.local`` sets ``BUILD_IOCS = YES``.  The example IOC can be built outside of driver module.
