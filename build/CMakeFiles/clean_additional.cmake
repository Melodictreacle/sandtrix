# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\sandtrix_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\sandtrix_autogen.dir\\ParseCache.txt"
  "sandtrix_autogen"
  )
endif()
