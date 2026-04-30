// RUN: %check_clang_tidy %s lenovo-sec011-insecure-ipc %t

#include <memory>
#include <string>

namespace grpc {
class ServerCredentials {};
inline std::shared_ptr<ServerCredentials> InsecureServerCredentials() { return {}; }
inline std::shared_ptr<ServerCredentials> SslServerCredentials(int) { return {}; }
}

void Bad() {
  auto creds = grpc::InsecureServerCredentials();
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: insecure IPC API 'InsecureServerCredentials'; use TLS-enabled credentials [lenovo-sec011-insecure-ipc]
  (void)creds;
}
