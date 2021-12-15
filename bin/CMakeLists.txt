file(GLOB_RECURSE SRCFILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

find_package(HydrArgs)  # https://github.com/HydrArgs/HydrArgs
find_package(HydrArgs_discoverer)  # https://github.com/HydrArgs/HydrArgs
find_package(mio)  # https://github.com/mandreyel/mio


add_executable(ScanBytes "${SRCFILES}")
target_include_directories(ScanBytes PUBLIC "${Include_dir}")
set_target_properties(ScanBytes PROPERTIES OUTPUT_NAME "ScanBytes")
harden(ScanBytes)

target_link_libraries(ScanBytes PRIVATE libScanBytes HydrArgs::HydrArgs HydrArgs_discoverer::HydrArgs_discoverer mio::mio)

cpack_add_component(bin
	DISPLAY_NAME "binary"
	DESCRIPTION "The CLI tool scanning a binary for certain bytes"
	DEPENDS "library" "library_sdk"
)

install(TARGETS "ScanBytes"
	RUNTIME
	COMPONENT "bin"
)

set("CPACK_COMPONENT_BIN_DESCRIPTION" "${PROJECT_DESCRIPTION}")
set("CPACK_DEBIAN_BIN_PACKAGE_NAME" "ScanBytes")
set("CPACK_RPM_BIN_PACKAGE_NAME" "ScanBytes")

set("CPACK_DEBIAN_BIN_PACKAGE_DEPENDS" "${CPACK_DEBIAN_LIBRARY_PACKAGE_NAME} (>= ${CPACK_PACKAGE_VERSION}), libhydrargs_discoverer")
set("CPACK_RPM_BIN_PACKAGE_DEPENDS" "${CPACK_RPM_LIBRARY_PACKAGE_NAME}")


list(APPEND CPACK_COMPONENTS_ALL "bin")  # strangely, not populated automatically correctly

pass_through_cpack_vars()
