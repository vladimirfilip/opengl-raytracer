# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/runner/work/opengl-raytracer/opengl-raytracer/build/_deps/glm-src")
  file(MAKE_DIRECTORY "/home/runner/work/opengl-raytracer/opengl-raytracer/build/_deps/glm-src")
endif()
file(MAKE_DIRECTORY
  "/home/runner/work/opengl-raytracer/opengl-raytracer/build/_deps/glm-build"
  "/home/runner/work/opengl-raytracer/opengl-raytracer/build/_deps/glm-subbuild/glm-populate-prefix"
  "/home/runner/work/opengl-raytracer/opengl-raytracer/build/_deps/glm-subbuild/glm-populate-prefix/tmp"
  "/home/runner/work/opengl-raytracer/opengl-raytracer/build/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp"
  "/home/runner/work/opengl-raytracer/opengl-raytracer/build/_deps/glm-subbuild/glm-populate-prefix/src"
  "/home/runner/work/opengl-raytracer/opengl-raytracer/build/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/runner/work/opengl-raytracer/opengl-raytracer/build/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/runner/work/opengl-raytracer/opengl-raytracer/build/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
