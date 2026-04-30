// RUN: %check_clang_tidy %s lenovo-name001-naming-convention %t

class UserService {
public:
  void GetUserData();
  int  ProcessRequest();
  const char* UserName();
};

struct Point {};
enum class Status { kOk, kErr };

class IUserService {
public:
  virtual ~IUserService() = default;
  virtual void DoWork() = 0;
};

const int MAX_SIZE = 100;
constexpr double PI = 3.14;
const int DEFAULT_TIMEOUT_MS = 5000;

// CHECK-MESSAGES-NOT: warning:
