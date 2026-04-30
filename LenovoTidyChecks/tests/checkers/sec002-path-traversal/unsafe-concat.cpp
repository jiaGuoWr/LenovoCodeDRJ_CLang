// RUN: %check_clang_tidy %s lenovo-sec002-path-traversal %t

#include <string>

std::string ReadUserFile(const std::string& userInput) {
  std::string filePath = "/base/" + userInput;
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: variable 'filePath' built by concatenation may allow directory traversal [lenovo-sec002-path-traversal]
  return filePath;
}

void OpenLogDir(const std::string& sub) {
  std::string dirPath = std::string("/var/log/") + sub;
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: variable 'dirPath' built by concatenation may allow directory traversal [lenovo-sec002-path-traversal]
  (void)dirPath;
}
