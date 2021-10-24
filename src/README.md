# code structure
This codebase is designed to be as simple & efficient as possible.

## rules & structure
- main.cpp is the only source file
- each header file can only be #included once; no header guards
- intermediary.h is the only file allowed to #inlcude external code

## files
- window.h   : opening a window, keyboard & mouse input, context instantiation
- renderer.h : model loading, OpenGL rendering, animation, 3D camera
- physics.h  : colliders, particle system, collider rendering
- networking.h : server & client interaction & functionality

## refresher
- ok you fucking idiot, you keep forgetting how this shit works so here you go:

### how meshes are loaded and rendered:
#### blender
- export your mesh as obj(w/ normals & optionally texture coords)
- or export as collada for animations(1 mesh at a time)
#### file converter
- run your mesh or animated mesh through file converter
- it will output a .mesh, .mesh_uv, .mesh_anim, or .mesh_anim_uv file
- animated meshes will also output a .anim file that contains keyframes & skeleton
#### renderer
- game renderer will load your .mesh into a Mesh_Data structure
- then it gets put into vertex buffers in the load(Drawable_Mesh) function
- the load() function will do some GL stuff and return a Drawable_Mesh that you can render

### how things are added into the game:
#### props
- you wrote a prop renderer, just go read the code dummy
#### everything else
- go read the code, if it's hard to understand, git gudder at programming idk
- the punishment for writing garbage should be rewritting & refactoring