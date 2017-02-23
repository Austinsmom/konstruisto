<img src="graphics/konstruisto.svg" width="360"/>

Simple city-builder using OpenGL. For Windows and Linux.

## Building

Just go with `make rebuild run`. To build release configuration: `make rebuild run CONFIG=RELEASE`.

`make help` will guide you.

Makefile is written for clang++ using MinGW headers.

## Author and <img src="https://opensource.org/files/osi_symbol.png" height="20" alt="Open Source" /> license

Copyright &copy; 2017 Krzysztof Antoniak

Contents of this repository is licensed under GNU General Public License, version 3.0 (GPL-3.0) with additional term according to the section 7(c) of the license: should you make a derivative of this repository, please do not use the name "Konstruisto", Konstruisto logo or use similar naming for your game what could misguide users. See [LICENSE.txt]() for details.

### Used libraries

* [GLFW 3.2.1](http://www.glfw.org/) under the zlib/libpng license
* [GLEW 2.0.0](http://glew.sourceforge.net/) under the Modified BSD License, the Mesa 3-D License (MIT) and the Khronos License (MIT)
* [GLM 0.9.8.4](http://glm.g-truc.net/0.9.8/index.html) under the MIT License