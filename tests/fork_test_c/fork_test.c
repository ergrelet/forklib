#include <Windows.h>
#include <forklib.h>
#include <stdio.h>

int main() {
  HANDLE test_event = CreateEventA(NULL, FALSE, FALSE, "TestEvent");
  MarkAllHandlesInheritable();

  ForkSettings fork_settings = {true, true};
  PROCESS_INFORMATION process_info = {0};
  const auto res = Fork(fork_settings, &process_info);
  if (res == -1) {
    printf("fork failed\n");
    return 1;
  }

  if (res != 0) {
    // Parent
    printf("forked process's PID is %lu\n", res);
    getc(stdin);
    if (test_event != NULL) {
      printf("Notifying event\n");
      SetEvent(test_event);
    }
  } else {
    // Child
    printf("Child's alive\n");
    if (test_event != NULL) {
      printf("Waiting for event to be notified...\n");
      WaitForSingleObject(test_event, INFINITE);
      printf("Event notified by parent!\n");
    }
    getc(stdin);
  }
}
