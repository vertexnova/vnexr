# Replacement for the FindPythonInterp module that CMake removed (policy CMP0148). The pinned OpenXR SDK still calls
# find_package(PythonInterp 3); it only needs Python to regenerate headers that ship pre-generated in its source tree.
find_package(Python3 ${PythonInterp_FIND_VERSION} QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
    set(PYTHONINTERP_FOUND TRUE)
    set(PythonInterp_FOUND TRUE)
else()
    set(PYTHONINTERP_FOUND FALSE)
    set(PythonInterp_FOUND FALSE)
endif()
