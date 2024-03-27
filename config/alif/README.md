# Alif sdk build and configuration files

This directory contains build scripts and common configuration files used by
CHIP Alif SDK. It is structured as follows:

| File/Folder | Contents                                                                                                                            |
| ----------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| chip-gn     | GN project used to build selected CHIP libraries with the Alif SDK platform integration layer                                       |
| chip-module | CMake wrapper for the GN project defined in `chip-gn` directory, and other components that allow one to use CHIP as a Zephyr module |
