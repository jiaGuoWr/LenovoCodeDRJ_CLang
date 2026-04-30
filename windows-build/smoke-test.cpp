// Smoke test: should trigger lenovo-sec001 (hardcoded sensitive info),
// lenovo-chn001 (Chinese in comments), and lenovo-name001 (naming).

const char* password = "MySecretPassword123";  // SEC001
const char* api_key  = "sk-1234567890abcdef";  // SEC001

// 这是一个中文注释 - should trigger CHN001

class user_service {  // NAME001: should be PascalCase
public:
    void doWork() {}
};

int main() { return 0; }
