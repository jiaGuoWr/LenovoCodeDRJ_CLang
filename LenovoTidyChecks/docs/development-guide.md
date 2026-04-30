# 开发指南

## 一、环境准备

### Linux / WSL

```bash
sudo apt update
sudo apt install \
    llvm-18-dev libclang-18-dev clang-tools-18 \
    libclang-cpp18-dev \
    cmake ninja-build git python3-pip
pip3 install lit
```

### macOS

```bash
brew install llvm@18 cmake ninja
export PATH="$(brew --prefix llvm@18)/bin:$PATH"
pip3 install lit
```

### Windows

1. 从 <https://github.com/llvm/llvm-project/releases> 下载 LLVM 18.x Windows x64 安装包
2. 安装时勾选 "Add LLVM to the system PATH"
3. 安装 [CMake](https://cmake.org/download/) 与 [Ninja](https://ninja-build.org/)
4. `py -m pip install lit`

## 二、克隆与构建

```bash
git clone https://github.com/your-org/LenovoTidyChecks.git
cd LenovoTidyChecks
cmake --preset linux-release
cmake --build --preset linux-release -j
```

产物位于 `build/linux-release/src/LenovoTidyModule/libLenovoTidyChecks.so`。

## 三、运行测试

```bash
# 全量（单元 + lit）
ctest --preset linux-release --output-on-failure

# 只跑单元
ctest --preset linux-release -L unit

# 只跑 lit
ctest --preset linux-release -L lit
```

## 四、新增一条规则

```bash
python3 scripts/new_check.py \
    --id SEC002 \
    --name path-traversal \
    --category security
```

脚本会生成：

- `src/LenovoTidyModule/security/PathTraversalCheck.{h,cpp}`
- `tests/checkers/sec002-path-traversal/{main,valid}.cpp`
- `docs/rules/sec002.md`
- 在 `LenovoTidyModule.cpp` 末尾追加注册行

## 五、目录速览

```
src/LenovoTidyModule/
├── LenovoTidyModule.cpp   # 注册入口
├── common/                # 纯函数工具（可被 unit test 直接测）
├── security/              # SEC 类规则
├── localization/          # CHN 类规则
└── naming/                # NAME 类规则
```

每类目录下的规则互相独立，**禁止跨目录 include**——需要共享的逻辑请下沉到 `common/`。

## 六、编码规约

- C++17；不用 C++20 feature（LLVM 18 API 仍有 17 保守约束）
- 一切类进入 `clang::tidy::lenovo::<category>` 命名空间
- 消息格式：`<kind> 'NAME' <verb> <suggestion>`，保持与 C# 一致
- 诊断 ID：`lenovo-<catXXX>-<kebab-name>`，全小写

## 七、本地自检

```bash
# 跑项目自己的 clang-tidy（用官方规则 + 自家规则）
cmake --build --preset linux-release --target LenovoTidyChecks
clang-tidy-18 -p build/linux-release src/LenovoTidyModule/*.cpp
```
