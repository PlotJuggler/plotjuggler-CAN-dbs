# - try to find PlotJuggler library
#
# Cache Variables: (probably not for direct use in your scripts)
#  PlotJuggler_INCLUDE_DIR
#  PlotJuggler_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  PlotJuggler_FOUND
#  PlotJuggler_INCLUDE_DIRS
#  PlotJuggler_LIBRARIES
#  PlotJuggler_RUNTIME_LIBRARIES - aka the dll for installing
#  PlotJuggler_RUNTIME_LIBRARY_DIRS
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#

set(PlotJuggler_ROOT_DIR
        "${PlotJuggler_ROOT_DIR}"
	CACHE
	PATH
	"Directory to search")

if(CMAKE_SIZEOF_VOID_P MATCHES "8")
	set(_LIBSUFFIXES /lib64 /lib)
else()
	set(_LIBSUFFIXES /lib)
endif()

find_library(PlotJuggler_LIBRARY
	NAMES
        plotjuggler_plugin_base
	PATHS
        "${PlotJuggler_ROOT_DIR}"
	PATH_SUFFIXES
	"${_LIBSUFFIXES}")

# Might want to look close to the library first for the includes.
get_filename_component(_libdir "${PlotJuggler_LIBRARY}" PATH)

find_path(PlotJuggler_INCLUDE_DIR
	NAMES
        pj_plugin.h
        plotdata.h
	HINTS
	"${_libdir}" # the library I based this on was sometimes bundled right next to its include
	"${_libdir}/.."
	PATHS
        "${PlotJuggler_ROOT_DIR}"
	PATH_SUFFIXES
        include/PlotJuggler)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PlotJuggler
	DEFAULT_MSG
        PlotJuggler_LIBRARY
        PlotJuggler_INCLUDE_DIR
	${_deps_check})

if(PlotJuggler_FOUND)
        set(PlotJuggler_LIBRARIES "${PlotJuggler_LIBRARY}")
        set(PlotJuggler_INCLUDE_DIRS "${PlotJuggler_INCLUDE_DIR}/..")
        mark_as_advanced(PlotJuggler_ROOT_DIR)
endif()

mark_as_advanced(PlotJuggler_INCLUDE_DIR
        PlotJuggler_LIBRARY
        PlotJuggler_RUNTIME_LIBRARY)

