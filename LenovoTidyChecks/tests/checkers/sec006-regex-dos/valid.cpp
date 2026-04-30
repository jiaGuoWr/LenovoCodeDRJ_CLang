// RUN: %check_clang_tidy %s lenovo-sec006-regex-dos-risk %t

// We do not flag uses of regex2 / re2 / RE2 (which are linear-time).
namespace re2 {
class RE2 {
public:
  RE2(const char*) {}
};
}

void Ok() {
  re2::RE2 re("(a+)+");
  (void)re;
}

// CHECK-MESSAGES-NOT: warning:
