# Shaders
## core
### plain
- for meshes that do not rotate or scale
- non textured meshes also take in a vec3 color
### transform
- transform shaders have an extra mat4 for transformations
### 2D
- same as core shaders for 3D meshes except position is 2D
- non textured quads also take in a vec3 color
## extra
- lighting shaders use PBR
- terrain shaders displace terrain heightmnap
- ocean shaders render fft water from height & normal maps
- the pond shader is a vertex shader for gerstner waves