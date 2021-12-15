set("CPACK_COMPONENT_LIBRARY_DESCRIPTION" "libScanBytes is a library for scanning a file for occurence of certain bytes. Supports multiple backends to achieve optimal performance in multiple common cases. On some platforms JIT-ed backend is available.")

buildAndPackageLib(${PROJECT_NAME}
	TARGET_NAME_WITH_LIB_PREFIX
	COMPONENT "library"
	DESCRIPTION "${PROJECT_DESCRIPTION}"
	PUBLIC_INCLUDES ${Include_dir}
)
