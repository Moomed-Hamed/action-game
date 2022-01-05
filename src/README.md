# code structure
This codebase is designed to be as simple & efficient as possible.

## structure & rules
- main.cpp is the only source file
- each header file can only be #included once; no header guards
- boilerplate.h is the only file allowed to #inlcude external code

## files
- window.h   : boilerplate, opening a window, key & mouse input, context instantiation
- renderer.h : mesh loading, OpenGL rendering, animation, 3D camera
- physics.h  : colliders, particle system, collider rendering
- networking.h : server & client interaction & functionality

## refreshers
### how meshes are loaded & rendered:
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
#### physics & particles
- basic collider types eg. 'Cube_Collider'
- some collision test functions eg. point_in_sphere() or sphere_plane_intersect()
- dynamic colliders are for things that move around, eg. prop barrel
- fixed colliders are for geometry that is fixed in place eg. a wall
- a 'Particle_Emitter' emits particles through an emit() funtion eg. emit_fire()