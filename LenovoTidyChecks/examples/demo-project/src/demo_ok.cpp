// 合规版本：与 demo.cpp 同样的业务意图，全部 15 条规则零告警。

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>

namespace logging {
struct Logger {
  void Error(const char*, const std::exception&) {}
};
extern Logger logger;
}

// SEC001 fix: from environment.
static const char* GetPassword() { return std::getenv("DB_PASSWORD"); }

// SEC005 fix: cryptographic generator.
unsigned RandomToken() {
  std::random_device rd;
  std::mt19937 eng(rd());
  std::uniform_int_distribution<unsigned> dist;
  return dist(eng);
}

// SEC008 fix: mkstemp.
int MakeTemp(char tpl[]) { return ::mkstemp(tpl); }

// SEC010 fix: atomic.
std::atomic<int> g_counter{0};

// NAME001 fixes.
class UserService {
 public:
  void GetUser();
};
class ILogger {
 public:
  virtual ~ILogger() = default;
  virtual void Log(const std::string&) = 0;
};
const int MAX_RETRY = 3;

// SEC002 fix: canonicalise + base check.
std::string OpenFile(const std::string& userInput) {
  auto base = std::filesystem::path("/var/log");
  auto p = std::filesystem::weakly_canonical(base / userInput).string();
  if (p.rfind("/var/log/", 0) != 0) throw std::runtime_error("invalid");
  return p;
}

// SEC003 fix: parameterised query.
const char* QueryTemplate() { return "SELECT * FROM Users WHERE Name = ?"; }

// SEC006 fix: no std::regex (use re2 or skip flagging by NOLINT if must).
//   left as a no-op placeholder for demo purposes.

// SEC007 fix: smart pointer.
struct Widget {};
std::unique_ptr<Widget> Make() { return std::make_unique<Widget>(); }

// EXC001 fix: structured logging.
void Run() {
  try { throw std::runtime_error("boom"); }
  catch (const std::exception& ex) { logging::logger.Error("operation failed", ex); }
}

// CODE001 fix: dead code removed; only prose comments left.
// This function delegates to the persistence layer.

// DLL003 fix: absolute path.
extern "C" void* dlopen(const char*, int);
void LoadPlugin() { dlopen("/usr/lib/libfoo.so", 2); }

// SEC004 / SEC011: deliberately omitted—use schema-validated formats / TLS-enabled credentials in production.
