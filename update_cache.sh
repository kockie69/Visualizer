	if [ "$1" == 1 ]; then
		sed -i 's/CMAKE_CXX_STANDARD_LIBRARIES:STRING=/CMAKE_CXX_STANDARD_LIBRARIES:STRING=-lpsapi /g' dep/projectm/build/CMakeCache.txt; 
	fi