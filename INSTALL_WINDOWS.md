# Lenovo Tidy on Windows — Installation & Usage

> One-page guide for end users (no toolchain knowledge required) and for
> rebuilding the artefacts from scratch.

---

## For end users: just install the VSIX

Pre-built artefacts live in `D:\LenovoDRJ_CLang\dist\`:

| File | Where to install |
|---|---|
| `lenovo-tidy-vscode.vsix` (~46 MB) | VS Code |
| `LenovoTidy.vsix` (~46 MB) | Visual Studio 2022 |
| `win32-x64\lenovo-clang-tidy.exe` (~98 MB) | Optional standalone CLI |

### VS Code

```powershell
code --install-extension D:\LenovoDRJ_CLang\dist\lenovo-tidy-vscode.vsix
```

Or: open VS Code → `Ctrl+Shift+P` → `Extensions: Install from VSIX…`.

### Visual Studio 2022

1. **Close VS 2022** completely (the installer requires the IDE to be idle and
   any running `devenv.exe` will block both uninstall and install). Check the
   system tray too.
2. **If you previously installed an older 0.x build**, uninstall it first:
   - Open `Extensions → Manage Extensions → Installed`, find **Lenovo Tidy**,
     click **Uninstall**, follow the prompts, **close VS 2022 again**.
3. Double-click `D:\LenovoDRJ_CLang\dist\LenovoTidy.vsix` → click **Install**.
4. **(Important when upgrading)** Force VS to rebuild its MEF component cache
   so the new `ContentType` registration is picked up. From PowerShell:

   ```powershell
   & "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.com" /updateconfiguration
   ```

   This finishes in a few seconds with no UI; without it VS may still serve
   the old MEF cache and the LSP server never starts. (Skip this step on a
   first-time install.)
5. Reopen VS 2022 normally.

> **Why /updateconfiguration is needed.** VS 2022 caches the MEF component
> graph at `%LocalAppData%\Microsoft\VisualStudio\17.0_*\ComponentModelCache\`.
> When an extension changes its exported `ContentType` definitions, the cache
> entry for that ContentType becomes stale and VS keeps using the previous
> wiring (which doesn't include the new client). `/updateconfiguration`
> rebuilds the cache from scratch.

### Use it

1. Open any C/C++ project.
2. The LSP server needs `compile_commands.json` to know how to compile each
   file. There are two ways it gets that:
   - **VS Code (interactive)**: the extension pops up notifications for every
     project type — auto-runs `cmake -S . -B build
     -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` for CMake projects, or copies the
     `compdb` / `bear` command to your clipboard for MSBuild / Make projects.
   - **VS 2022 (silent)** and as a server-side safety net for any client: when
     the LSP server receives the first `.cpp` / `.h` file, it walks up from
     the file's directory looking for `compile_commands.json` in `build/`,
     `out/`, `cmake-build-debug/`, `cmake-build-release/`,
     `cmake-build-relwithdebinfo/`, `.vscode/`, or the workspace root. The
     first match wins.
3. If neither path finds the database, only text-level rules (CHN001 /
   CODE001 / SEC001) will fire. Generate a `compile_commands.json` (typical
   CMake recipe: `cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`)
   and save the file again.
4. Save any `.cpp` / `.h` file.
5. Diagnostics appear in:
   - VS Code → **Problems** panel (`Ctrl+Shift+M`)
   - VS 2022 → **Error List**

Both prefix every diagnostic with the rule id, e.g. `lenovo-sec001-hardcoded-sensitive`.
Full rule reference: see [`AnalyzerRules.md`](AnalyzerRules.md).

**Useful commands** (VS Code command palette, `Ctrl+Shift+P`):
- `Lenovo Tidy: Regenerate compile_commands.json` — rerun the detection chain.
- `Lenovo Tidy: Show Server Output` — open the LSP log window for troubleshooting.
- `Lenovo Tidy: Restart Language Server` — restart the LSP (useful after editing settings).

### Quick sanity check (no IDE needed)

```powershell
D:\LenovoDRJ_CLang\dist\win32-x64\lenovo-clang-tidy.exe `
    --checks="-*,lenovo-*" `
    D:\LenovoDRJ_CLang\windows-build\smoke-test.cpp -- -std=c++17
```

You should see ~7 warnings (sec001, chn001, name001, …). If yes, the
toolchain is healthy.

---

## For maintainers: rebuilding everything

Source tree is under `D:\LenovoDRJ_CLang\`. The build uses a self-contained
toolchain in `D:\dev-tools\` plus the existing system Node 24 / .NET SDK 9.

### One-shot rebuild

```powershell
# Phase 3 — recompile the C++ rules + lenovo-clang-tidy.exe (~5 min)
powershell -ExecutionPolicy Bypass -File D:\LenovoDRJ_CLang\windows-build\build-checks.ps1

# Phase 4 — recompile the LSP server (~1-2 min)
powershell -ExecutionPolicy Bypass -File D:\LenovoDRJ_CLang\windows-build\build-lsp.ps1

# Phase 5 + 6 — repackage both VSIX files (~1 min each)
powershell -ExecutionPolicy Bypass -File D:\LenovoDRJ_CLang\windows-build\build-vscode.ps1
powershell -ExecutionPolicy Bypass -File D:\LenovoDRJ_CLang\windows-build\build-vs2022.ps1
```

All phases write logs into `D:\LenovoDRJ_CLang\windows-build\logs\phase*.log`.

### From a brand-new Windows machine

1. Run `windows-build\run-elevated-prereqs.ps1` (one UAC prompt, ~15 min):
   installs LLVM 18.1.8, VS Build Tools 2022 (C++ workload), CMake, Ninja,
   Python 3.12 + lit into `D:\dev-tools\`.
2. Run `windows-build\install-rust.ps1` (~3 min): installs the
   `stable-x86_64-pc-windows-msvc` Rust toolchain into `D:\dev-tools\rust\`.
3. Run `windows-build\install-llvm-dev.ps1` (~5 min): replaces the runtime-only
   LLVM with the full SDK tarball (`clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz`)
   AND auto-patches `LLVMExports.cmake` to fix the hard-coded VS 2019 Pro
   `diaguids.lib` path.
4. Run the four build scripts above.

### Why two flavours of the rule library?

| Artefact | When it’s used | How it works |
|---|---|---|
| `LenovoTidyChecks.{so,dll,dylib}` | Linux / macOS | classic clang-tidy plugin loaded via `-load=` |
| `lenovo-clang-tidy.exe` | **Windows** | self-contained executable, rules statically linked |

The reason for the second flavour is that LLVM’s official Windows binaries
ship with plugin loading effectively disabled — the `ClangTidyModuleRegistry`
inside any `-load=...dll` is a separate copy that the executable never reads.
See [LLVM #159710](https://github.com/llvm/llvm-project/issues/159710).

The LSP server auto-detects which flavour to use:

- It first checks for `lenovo-clang-tidy[.exe]` next to itself (this is what
  the VSIX bundles into `server-bin/win32-x64/`).
- Falls back to `clang-tidy-18` / `clang-tidy` on `PATH`.
- Whenever the resolved binary is `lenovo-clang-tidy*`, it **ignores any
  `pluginPath` from the IDE client**, even if the user configured one. This
  prevents a fatal double-init of LLVM’s `ManagedStatic` globals.

---

## Troubleshooting

### "No diagnostics appear in VS Code / VS 2022"

1. Confirm the IDE actually loaded the extension:
   - VS Code: `View → Output → "Lenovo Tidy"` channel.
   - VS 2022: `View → Output` and select the LSP source.
2. The most common cause is **missing `compile_commands.json`** in your project.
   Since v0.2, VS Code will auto-offer to generate one for CMake projects — if
   you declined the prompt, rerun **Lenovo Tidy: Regenerate compile_commands.json**
   from the command palette.
3. If auto-configure failed (e.g. your `cmake` isn't on PATH), set
   `lenovoTidy.cmakeExecutable` to an absolute path, or generate the database
   manually and point `lenovoTidy.compileCommandsDir` at its directory.
4. If the project isn't CMake-based, produce `compile_commands.json` with
   `compdb` or `bear`, then **Lenovo Tidy: Regenerate compile_commands.json**
   or simply reload the window.

### "Lenovo Tidy: lenovo-tidy-lsp binary not found"

The VSIX was repackaged without staging the binaries. Rerun
`build-vscode.ps1` / `build-vs2022.ps1` and reinstall.

### "VS 2022: extension installs but `lenovo-tidy-lsp.exe` never appears in Task Manager"

This is the symptom of a broken MEF activation path. The extension DLL loads
fine, VS sees the type in the MEF catalog, but VS never calls
`ActivateAsync()` so the LSP process is never spawned. Over the lifetime of
this project three independent root causes produced exactly this symptom.
Run them top-to-bottom; the first one that reports wrong is the bug.

1. **Assembly strong-name mismatch.** Our DLL references
   `Microsoft.VisualStudio.LanguageServer.Client` with a version, and the
   system's DLL (shipped with VS) has a version. If they differ by even a
   patch number, the CLR fails to bind, MEF swallows the load exception,
   and our client is silently dropped from the graph. Check:

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File D:\LenovoDRJ_CLang\windows-build\verify-vs2022-host.ps1
   ```

   STEP 1 prints both versions; they must be identical. If not, bump the
   `PackageReference` in [`LenovoTidyVs2022/LenovoTidy.csproj`](LenovoTidyVs2022/LenovoTidy.csproj)
   to match the system's version, rebuild, reinstall.

2. **`ILanguageClient` missing `[ContentType("C/C++")]`.** VS 2022's C++
   project system stamps every `.cpp`/`.h` buffer with ContentType `"C/C++"`
   regardless of any `FileExtensionToContentTypeDefinition` we register.
   If our client only declares `[ContentType("LenovoTidyCpp")]`, the match
   never fires. [`LenovoTidyVs2022/LspClient.cs`](LenovoTidyVs2022/LspClient.cs)
   must declare *all four*: `LenovoTidyCpp`, `C/C++`, `c`, `cpp`.

3. **`ContentTypeDefinition` is `sealed` — class-level `[Export]` is a silent no-op.**
   The canonical MEF shape is to export a **static field** typed
   `ContentTypeDefinition`, NOT to put `[Export]/[Name]/[BaseDefinition]` on
   the class itself. The class pattern compiles cleanly but registers the
   class under its own type contract, which
   `IContentTypeRegistryService` never looks at. The symptom is that the
   MEF cache contains our `LenovoTidyClient` block but `LenovoTidyCpp` never
   appears as a registered ContentType (only as a `[ContentType]` attribute
   reference inside `LenovoTidyClient`'s metadata). Quick check:

   ```powershell
   $cache = 'C:\Users\<you>\AppData\Local\Microsoft\VisualStudio\17.0_*\ComponentModelCache\Microsoft.VisualStudio.Default.cache'
   $txt = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes((Resolve-Path $cache)))
   # Count raw string occurrences. After the fix you should see BOTH fields
   # `LenovoTidyContentTypeDefinition` and `CCppLspBaseAugmentation` in the cache.
   foreach ($s in 'LenovoTidyContentTypeDefinition','CCppLspBaseAugmentation','code-languageserver-preview') {
       Write-Host "  $s : $(([regex]::Matches($txt, [regex]::Escape($s))).Count)"
   }
   ```

After any of these three fixes you MUST run `devenv /updateconfiguration`
(see the upgrade step above) so VS rebuilds its MEF cache — otherwise VS
keeps serving the stale cached graph and your fix looks like it did nothing.

Fast smoke test (no GUI required): the
[`windows-build/smoke-activate.ps1`](windows-build/smoke-activate.ps1) script
launches VS with `/Log` + `smoke-test.cpp`, polls for `lenovo-tidy-lsp.exe`
in the process list for 120 s, and returns exit 0 only when the LSP actually
spawns.

### "lenovo-clang-tidy.exe is huge (~98 MB)"

That is expected: the binary statically links LLVM, Clang and every official
clang-tidy module so users get the entire ecosystem without depending on
external plugin loading.

### "I want to test a CLI command before installing the IDE extension"

```powershell
D:\LenovoDRJ_CLang\dist\win32-x64\lenovo-clang-tidy.exe --list-checks --checks="-*,lenovo-*"
```
Should print 15 rules. If you get only `clang-analyzer-*`, something is wrong
with the build (the static-anchor reference was stripped). Re-run
`build-checks.ps1`.

---

## Validation status

End-to-end validated as of 2026-04-30:

- [x] `lenovo-clang-tidy.exe --list-checks` shows all 15 lenovo-* rules
- [x] `lenovo-clang-tidy.exe smoke-test.cpp` emits 7 expected warnings
- [x] `cargo test` passes for the LSP server (9 unit + 8 integration tests)
- [x] Both VSIX files are well-formed and contain the three binaries
- [x] Installing `lenovo-tidy-vscode.vsix` into VS Code produces diagnostics
      in the Problems panel
- [x] Installing `LenovoTidy.vsix` into VS 2022 produces diagnostics in the
      Error List (activation path verified both by `smoke-activate.ps1`
      headless run and GUI confirmation)
