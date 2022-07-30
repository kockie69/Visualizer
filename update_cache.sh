    echo "Hello"	
	if [ "$1" == 1 ]; then
        echo "Hello"
		sed -i 's/CMAKE_CXX_STANDARD_LIBRARIES:STRING=/CMAKE_CXX_STANDARD_LIBRARIES:STRING=-lpsapi /g' src/dep/projectm/build/CMakeCache.txt; 
	fi