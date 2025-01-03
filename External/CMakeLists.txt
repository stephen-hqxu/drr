##############
### xxHash ###
##############
add_library(xxHash INTERFACE)
add_library(${PROJECT_NAME}::xxHash ALIAS xxHash)
target_link_libraries(xxHash INTERFACE xxHash::xxhash)
target_compile_definitions(xxHash INTERFACE
	XXH_NO_STREAM
)

# There is a bug in xxHash's CMake config that they erroneously set the export macro in the config.
# Export macro should only be defined during compilation and is not needed by the application.
# Need to manually replace export with import macro to sure correct symbol visibility.
get_target_property(xxHashDef xxHash::xxhash INTERFACE_COMPILE_DEFINITIONS)
list(TRANSFORM xxHashDef
	REPLACE EXPORT IMPORT
	REGEX XXH_EXPORT
)
set_target_properties(xxHash::xxhash PROPERTIES
	INTERFACE_COMPILE_DEFINITIONS "${xxHashDef}"
)