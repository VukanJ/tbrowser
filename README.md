# Terminal TBrowser application
This application makes it possible to browse the content of a ROOT file directly in the console.
It is meant to be fast (no GUI drawing) and to support basic `TTree->Draw(...)` commands.
1D and 2D Histograms are drawn with Unicode block characters directly to the console.

## Installation
```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/ROOT -DCMAKE_BUILD_TYPE=Release ..
make -j
```
