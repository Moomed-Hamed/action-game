# codebase
This codebase is designed to be as simple & efficient as possible.

### rules & structure
- main.cpp is the only source file
- each header file can only be #included once; no header guards
- intermediary.h is the only file allowed to #inlcude external code

### files
- window.h   : opening a window, keyboard & mouse input, context instantiation
- renderer.h : model loading, OpenGL rendering, animation, 3D camera
- physics.h  : colliders, particle system, collider rendering
- networking.h : server & client interaction & functionality