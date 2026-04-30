// RUN: %check_clang_tidy %s lenovo-name001-naming-convention %t

class UserService {
public:
  virtual ~UserService() = default;
  virtual void DoWork() = 0;
};
// CHECK-MESSAGES: :[[@LINE-5]]:7: warning: interface-like class 'UserService' should start with 'I' (e.g. 'IUserService') [lenovo-name001-naming-convention]

class Logger {
public:
  virtual ~Logger() = default;
  virtual void Log(const char* message) = 0;
  virtual void Flush() = 0;
};
// CHECK-MESSAGES: :[[@LINE-6]]:7: warning: interface-like class 'Logger' should start with 'I' (e.g. 'ILogger') [lenovo-name001-naming-convention]
