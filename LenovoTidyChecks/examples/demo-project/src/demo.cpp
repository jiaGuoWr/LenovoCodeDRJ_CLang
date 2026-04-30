// 故意违规示例文件，展示 LenovoTidyChecks v0.2 全部 15 条规则。
// 运行：clang-tidy -load=.../libLenovoTidyChecks.so -p build src/demo.cpp

#include <cstdio>
#include <cstdlib>
#include <regex>
#include <stdexcept>
#include <string>

// CHN001: 中文注释
// SEC001: 硬编码密码
const char* password = "MySecretPassword123";

// SEC005: 不安全随机数
unsigned RandomToken() { return std::rand(); }

// SEC008: 不安全临时文件
void MakeTemp(char* tpl) { ::mktemp(tpl); }

// SEC010: 可变全局
int g_counter = 0;

// NAME001: 类名不是 PascalCase
class user_service {
 public:
  void getUser();      // NAME001: 方法名
};

// NAME001: 接口缺少 'I' 前缀
class Logger {
 public:
  virtual ~Logger() = default;
  virtual void Log(const std::string&) = 0;
};

// NAME001: 常量不是 UPPER_SNAKE_CASE
const int MaxRetry = 3;

// SEC002: 路径拼接
std::string OpenFile(const std::string& userInput) {
  std::string filePath = "/var/log/" + userInput;
  return filePath;
}

// SEC003: SQL 拼接
std::string Query(const std::string& name) {
  std::string sql = "SELECT * FROM Users WHERE Name = '" + name + "'";
  return sql;
}

// SEC006: 无超时 std::regex
void Match(const std::string& p) { std::regex r(p); (void)r; }

// SEC007: 裸 new
struct Widget {};
Widget* Bad() { return new Widget(); }

// EXC001: catch 中打印 stderr
void Run() {
  try { throw std::runtime_error("boom"); }
  catch (const std::exception& ex) { std::fprintf(stderr, "%s", ex.what()); }
}

// CODE001: 注释里的代码
// var oldImpl = new OldThing();
// if (x > 0) { return x; }

// DLL003: 短名加载
extern "C" void* dlopen(const char*, int);
void LoadPlugin() { dlopen("libfoo.so", 2); }

// SEC004: 不安全反序列化（伪造的 boost::archive 类，便于演示）
namespace boost { namespace archive {
class binary_iarchive { public: binary_iarchive(int) {} };
}}
void Deserialize() { boost::archive::binary_iarchive ar(0); (void)ar; }

// SEC011: 不安全 IPC（伪造的 grpc）
namespace grpc {
class ServerCredentials {};
inline std::shared_ptr<ServerCredentials> InsecureServerCredentials() { return {}; }
}
void Serve() { auto c = grpc::InsecureServerCredentials(); (void)c; }
