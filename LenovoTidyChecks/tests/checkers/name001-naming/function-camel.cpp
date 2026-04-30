// RUN: %check_clang_tidy %s lenovo-name001-naming-convention %t

class MyService {
public:
  void getUserData();
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: public method 'getUserData' should use PascalCase (e.g. 'GetUserData') [lenovo-name001-naming-convention]

  int ProcessRequest();
};

void top_level_free_func() {}
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: public method 'top_level_free_func' should use PascalCase (e.g. 'TopLevelFreeFunc') [lenovo-name001-naming-convention]
