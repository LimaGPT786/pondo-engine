# Pondo Engine
A small game engine

# How to Contribute
1. Fork and clone the repository on your local machine.
2. Install [premake](https://premake.github.io/). Use it to run premake5.lua to build the project. We recommend adding premake5 to your system PATH variables so you can simply call `premake5 premake5.lua` in the project :)
3. Update submodules by calling `git submodule update --init --recursive`



To-dos:
Add OpenGL, GLM, and GLFW (Graphics Library Framework)
Math library (may belong to GLFW so math is taken care of)
Create pipeline between OpenGL and GPU
Begin drawing vertices and triangles

Possible folder structure:
Engine
  Core
  Renderer
  Math
  Scene
  Platform
Game
