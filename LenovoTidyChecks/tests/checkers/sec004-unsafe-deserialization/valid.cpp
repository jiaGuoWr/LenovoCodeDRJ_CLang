// RUN: %check_clang_tidy %s lenovo-sec004-unsafe-deserialization %t

#include <string>

// JSON / Protobuf are considered safer (schema-validated) by default and not flagged.
namespace nlohmann { template<typename> struct basic_json {}; using json = basic_json<int>; }

void Ok() {
  std::string s = "{}";
  (void)s;
}

// CHECK-MESSAGES-NOT: warning:
