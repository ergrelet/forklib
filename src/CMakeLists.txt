set(FORKLIB_SRC_FILES
  "dllmain.cpp"
  "forklib.cpp"
  "fork.h"
  "fork.cpp"
  "csrss.h"
  "csrss.cpp"
  "csrss_offsets.h"
  "csrss_offsets.cpp"
  "handle_inherit.cpp"
  "targetver.h"
  "windows_phnt.h"
  "wow64ext.h"
  "logging.h"
)

add_library(forklib SHARED ${FORKLIB_SRC_FILES})
target_include_directories(forklib PUBLIC "../include")
target_include_directories(forklib PRIVATE "phnt/include")
target_compile_definitions(forklib PRIVATE "_USRDLL")
target_link_libraries(forklib PRIVATE dbghelp)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  target_link_libraries(forklib PRIVATE wow64ext)
endif()

add_executable(fork_test "fork_test/fork_test.cpp")
target_link_libraries(fork_test PRIVATE forklib)