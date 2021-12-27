#include <Windows.h>
#include <forklib.h>

#include <cstdio>

int main() {
  PROCESS_INFORMATION process_info = {0};
  const auto res = fork(&process_info);
  if (res == -1) {
    ::printf("fork failed\n");
    return 1;
  }

  HANDLE test_event = ::CreateEventA(nullptr, FALSE, FALSE, "TestEvent");
  if (res != 0) {
    // Parent
    ::printf("forked process's PID is %lu\n", res);
    ::getc(stdin);
    if (test_event != nullptr) {
      ::printf("Notifying event\n");
      ::SetEvent(test_event);
    }
  } else {
    // Child
    ::printf("Child's alive\n");
    if (test_event != nullptr) {
      ::printf("Waiting for event to be notified...\n");
      ::WaitForSingleObject(test_event, INFINITE);
      ::printf("Event notified by parent!\n");
    }
    ::getc(stdin);
  }
}
