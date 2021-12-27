set(WOW64EXT_SRC_FILES
  "CMemPtr.h"
  "internal.h"
  "resource.h"
  "wow64ext.h"
  "wow64ext.cpp"
  "wow64ext.rc"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  add_library(wow64ext SHARED ${WOW64EXT_SRC_FILES})
  target_include_directories(wow64ext PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(wow64ext PRIVATE "WOW64EXT_EXPORTS")
endif()