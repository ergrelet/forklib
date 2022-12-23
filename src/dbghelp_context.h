#pragma once

class DbgHelpContext {
 public:
  DbgHelpContext() {
    // Ignore errors, subsequent calls to Sym* APIs will fail graciously down
    // the road.
    InitializeSymbols();
  }
  ~DbgHelpContext() { CleanupSymbols(); }

 private:
  bool InitializeSymbols();
  void CleanupSymbols();
};
