set(FORKLIB_SRC_FILES
  "dllmain.cpp"
  "forklib.cpp"
  "fork.h"
  "fork.cpp"
  "csrss.h"
  "csrss.cpp"
  "csr_region.h"
  "csr_region.cpp"
  "dbghelp_context.h"
  "dbghelp_context.cpp"
  "handle_inherit.h"
  "handle_inherit.cpp"
  "ntdll_apis.cpp"
  "targetver.h"
  "windows_phnt.h"
  "wow64ext.h"
  "logging.h"
)

if(FORKLIB_BUILD_SHARED_LIB)
  set(FORKLIB_TYPE SHARED)
else()
  set(FORKLIB_TYPE STATIC)
endif()

add_library(forklib ${FORKLIB_TYPE} ${FORKLIB_SRC_FILES})
target_include_directories(forklib PUBLIC "../include")
target_include_directories(forklib PRIVATE "phnt/include")
target_compile_definitions(forklib PRIVATE "_USRDLL")
target_link_libraries(forklib PRIVATE dbghelp ntdll)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  target_link_libraries(forklib PRIVATE wow64ext)
endif()

if(FORKLIB_BUILD_SHARED_LIB)
  target_compile_definitions(forklib PUBLIC "FORKLIB_SHARED_LIB")
endif()
