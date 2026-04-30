// RUN: %check_clang_tidy %s lenovo-dll003-unsafe-dll-load %t

typedef void* HMODULE;
extern "C" HMODULE LoadLibraryA(const char*);
extern "C" void* dlopen(const char*, int);

void Ok1() {
  HMODULE m = LoadLibraryA("C:\\Windows\\System32\\user32.dll");
  (void)m;
}

void Ok2() {
  void* h = dlopen("/usr/lib/libfoo.so", 2);
  (void)h;
}

// CHECK-MESSAGES-NOT: warning:
