# Developer Setup
* Fork and clone the repository on your local machine.
* Install premake. Use it to run premake5.lua to build the project. We recommend adding premake5 to your system PATH variables so you can simply call `premake5 premake5.lua` in the project :)
* Update submodules by calling git submodule update --init --recursive

# General Todos 
* Add OpenGL, GLM, and GLFW (Graphics Library Framework) 
* Math library (may belong to GLFW so math is taken care of) 
* Create pipeline between OpenGL and GPU Begin drawing vertices and triangles

# C++ Style Guide
| Type | Case |
| --- | --- |
| Classes | PascalCase (e.g. KeyEvent, WindowResizeEvent) |
| Member Variables | PascalCase with leading `m_` prefix <br>(e.g. m_KeyCode, m_Height) |
| Methods | PascalCase (e.g. GetKeyCode(), GetWidth()) |
| Enums & Constants | PascalCase (e.g. EventType, EventCategory) |
| Macros | SCREAMING_SNAKE_CASE (e.g. PONDO_API, EVENT_CLASS_TYPE) |

## Member Access
| Type | Style |
| --- | --- |
| Getters | Inline, const-qualified |
| Setters | Avoid public mutation, prefer initialization via constructor |
| Private data | Use `m_` prefix, keep as a private member (bottom of class) |
| Protected data | Use `m_` prefix for base classes that subclasses access (e.g. m_KeyCode)` |
| Static data | Use `s_` prefix for static variables |

## Other Tips
* `#pragma once` is preferred over `#ifndef` in header files
* Include order: Engine headers, then standard library
* All code should be contained in `namespace Pondo {...}`
* Mark all public-facing classes with the PONDO_API macro so it's defined in the library API
