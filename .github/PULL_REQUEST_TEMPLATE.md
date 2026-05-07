<!--
  Lenovo DRJ C-Lang PR template.
  Fill out the relevant sections; delete the ones that don't apply.
-->

## Summary

<!-- 1-3 sentence description of WHAT changed and WHY. Avoid restating the diff. -->

## Type of change

- [ ] Bug fix (non-breaking)
- [ ] New rule / new feature (non-breaking)
- [ ] Breaking change (rule rename, config schema change, public API change)
- [ ] CI / build / packaging only
- [ ] Docs / refactor / dependency bump only

## Affected components

- [ ] `LenovoTidyChecks/` (C++ Clang-Tidy module + lenovo-clang-tidy.exe)
- [ ] `LenovoTidyLsp/` (Rust LSP server)
- [ ] `LenovoTidyVscode/` (VS Code extension)
- [ ] `LenovoTidyVs2022/` (VS 2022 extension)
- [ ] `windows-build/` (PowerShell build pipeline)
- [ ] `.github/workflows/` (CI)
- [ ] Docs only

## Rule-author checklist (only if you added/modified a `lenovo-*` rule)

- [ ] `LenovoTidyChecks/src/LenovoTidyModule/<group>/<Name>Check.{h,cpp}` added/updated
- [ ] Registered in `LenovoTidyModule.cpp`
- [ ] Listed in `LenovoTidyMain/CMakeLists.txt` only if it requires a new module library
- [ ] Test fixture under `LenovoTidyChecks/tests/checkers/<id>-<slug>/` (one negative + one valid)
- [ ] Documentation page `LenovoTidyChecks/docs/rules/<id>.md`
- [ ] `AnalyzerRules.md` updated
- [ ] `python3 LenovoTidyChecks/scripts/generate_rules_index.py` runs clean
- [ ] `lenovo-clang-tidy --checks=-*,lenovo-* --list-checks` lists the new rule (smoke test in `build-test.yml` enforces a minimum count)

## Test plan

<!-- How did you verify? Paste commands and key outputs. -->

```bash
# example
cd LenovoTidyChecks && cmake --build build -j && ctest --test-dir build --output-on-failure
```

- [ ] Unit tests pass locally on at least one platform
- [ ] CI is green (build-test, lsp-build, extension-build as applicable)
- [ ] Manual smoke: opened the IDE extension on a real project (only required for IDE-affecting changes)

## Risk / rollback

<!-- What's the worst-case impact if this misbehaves in the field? How does
     a user disable the new behaviour? -->

## Related issues / ADRs

<!-- "Closes #123", "Refs ADR-0003", etc. -->

## Reviewer notes

<!-- Anything subtle you want the reviewer to focus on. Delete if N/A. -->
