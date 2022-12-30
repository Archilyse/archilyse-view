# Archilyse View

Archilyse View is a simulation software to evaluate the visibile space in 3d geometry. It works by creating a cubemap around each point
and pass it to different GLSL compute shaders:
 * volume: Computes the volume of the visible space
 * area: Computes the area of the visible space (minimum = 0, maximum = 4*pi)
 * groups: Computes the area of the visible space by color groups
 * cubeMap: Creates render images

## Examples

Examples can be found under the /test directory. The software accepts a single json file and writes its output to a specified json file.

## Installation

### Depedencencies
The package depends on:
 * libvulkan-dev (Make sure to have installed your vendor's graphics drivers)
 
### Building
For building the software, run `cmake . && make`

### Packaging
For packing a .deb file, run `cmake . && cpack`
