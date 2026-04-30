// RUN: %check_clang_tidy %s lenovo-dll003-unsafe-dll-load %t

typedef void* HMODULE;
extern "C" HMODULE LoadLibraryA(const char*);
extern "C" HMODULE LoadLibraryW(const wchar_t*);
extern "C" void* dlopen(const char*, int);

void Bad1() {
  HMODULE m = LoadLibraryA("user32.dll");
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: dynamic library load 'LoadLibraryA' without an absolute path; vulnerable to DLL hijacking [lenovo-dll003-unsafe-dll-load]
  (void)m;
}

void Bad2() {
  void* h = dlopen("libfoo.so", 2);
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: dynamic library load 'dlopen' without an absolute path; vulnerable to DLL hijacking [lenovo-dll003-unsafe-dll-load]
  (void)h;
}
