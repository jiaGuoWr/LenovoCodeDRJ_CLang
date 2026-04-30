# CI 集成

## 一、GitHub Actions（推荐）

`ci/github-actions/build-test.yml` 已预置三平台矩阵。业务项目接入 `lenovo-*` 规则的典型 workflow：

```yaml
name: lenovo-tidy
on: [pull_request]

jobs:
  tidy:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - uses: actions/download-artifact@v4
        with:
          name: LenovoTidyChecks-linux
          repository: your-org/LenovoTidyChecks
          path: tidy-plugin
      - name: Install toolchain
        run: |
          sudo apt install -y clang-tools-18 ninja-build cmake
      - name: Configure
        run: cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
      - name: Run clang-tidy
        run: |
          find src -name '*.cpp' -print0 | xargs -0 -P4 \
            clang-tidy-18 \
              -load=$PWD/tidy-plugin/libLenovoTidyChecks.so \
              -p build \
              --warnings-as-errors='lenovo-*'
```

`--warnings-as-errors='lenovo-*'` 会把 Lenovo 规则的告警升级为错误，阻止合入。

## 二、GitLab CI

```yaml
lenovo-tidy:
  image: registry.lenovo.com/drj-tidy/runner:llvm18
  stage: test
  script:
    - cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    - clang-tidy-18
        -load=/opt/tidy-plugin/libLenovoTidyChecks.so
        -p build src/*.cpp
        --warnings-as-errors='lenovo-*'
```

## 三、Jenkins

借助 `clang-tidy` 的 SARIF 输出，可用 [Warnings NG Plugin](https://plugins.jenkins.io/warnings-ng/) 直接消费：

```groovy
stage('Lint') {
  steps {
    sh '''
      clang-tidy-18 \
        -load=${TIDY_PLUGIN} \
        -p build \
        --export-fixes=tidy.yaml \
        src/*.cpp
    '''
    recordIssues tool: clangTidy(pattern: 'tidy.yaml')
  }
}
```

## 四、结果呈现

- GitHub：借助 [`reviewdog/action-clang-tidy`](https://github.com/reviewdog/action-suggester) 把违规变成 PR 内联评论
- SonarQube：导入 clang-tidy 输出（`-export-fixes`）的 YAML
- CodeChecker：在 `CodeChecker analyze` 前指定 `--tidy-config` 和 `-load`

## 五、门禁策略

推荐分级：

| 阶段 | 严格度 | 做法 |
|---|---|---|
| 引入第 1 个月 | 只警告 | `-warnings-as-errors=''` |
| 第 2～3 个月 | 按类别升级 | `-warnings-as-errors='lenovo-sec*'`（只把安全规则当错） |
| 稳定后 | 全量错误 | `-warnings-as-errors='lenovo-*'` |

这样给团队一段适应期，同时不会让新规则一上线就挡住整条流水线。
