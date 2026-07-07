# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\FlightTicketServer_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\FlightTicketServer_autogen.dir\\ParseCache.txt"
  "FlightTicketServer_autogen"
  )
endif()
