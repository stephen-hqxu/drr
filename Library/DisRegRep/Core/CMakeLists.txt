add_subdirectory(System)
add_subdirectory(View)

drrTargetSource(
HEADER
	Exception
	ThreadPool
	Type
	UninitialisedAllocator
	XXHash
SOURCE
	Exception
	ThreadPool
	XXHash
)