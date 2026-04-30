// RUN: %check_clang_tidy %s lenovo-sec011-insecure-ipc %t

#include <memory>

namespace grpc {
class ServerCredentials {};
inline std::shared_ptr<ServerCredentials> SslServerCredentials(int) { return {}; }
}

void Ok() {
  auto creds = grpc::SslServerCredentials(0);
  (void)creds;
}

// CHECK-MESSAGES-NOT: warning:
