import * as cp from "child_process";
import * as fs from "fs";
import * as path from "path";
import {
  commands,
  env,
  ExtensionContext,
  ProgressLocation,
  Uri,
  window,
  workspace,
} from "vscode";
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  TransportKind,
} from "vscode-languageclient/node";

let client: LanguageClient | undefined;
let extensionContext: ExtensionContext | undefined;

const SKIP_AUTO_KEY = "lenovoTidy.skipAutoConfigureForWorkspace";

// Directories that hold compile_commands.json by convention, in priority order.
// First match wins.
const DB_SEARCH_DIRS = [
  "build",
  "out",
  "cmake-build-debug",
  "cmake-build-release",
  "cmake-build-relwithdebinfo",
  ".vscode",
  "", // workspace root itself
];

export async function activate(context: ExtensionContext): Promise<void> {
  extensionContext = context;

  const serverBinary = resolveServerBinary(context);
  if (!serverBinary) {
    window.showErrorMessage(
      "Lenovo Tidy: lenovo-tidy-lsp binary not found. Rebuild the extension with the correct server-bin/ contents."
    );
    return;
  }

  context.subscriptions.push(
    commands.registerCommand(
      "lenovoTidy.regenerateCompileCommands",
      async () => {
        await context.workspaceState.update(SKIP_AUTO_KEY, false);
        const dir = await ensureCompileCommandsDir(context, /*force*/ true);
        if (dir) {
          window.showInformationMessage(
            `Lenovo Tidy: using compile_commands.json at ${dir}.`
          );
          await restartClient(context, serverBinary);
        }
      }
    ),
    commands.registerCommand("lenovoTidy.showOutput", () => {
      client?.outputChannel.show(true);
    }),
    commands.registerCommand("lenovoTidy.restartServer", async () => {
      await restartClient(context, serverBinary);
    })
  );

  await startClient(context, serverBinary);
}

export async function deactivate(): Promise<void> {
  if (client) {
    await client.stop();
    client = undefined;
  }
}

async function startClient(
  context: ExtensionContext,
  serverBinary: string
): Promise<void> {
  const compileDbDir = await ensureCompileCommandsDir(context, /*force*/ false);

  const config = workspace.getConfiguration("lenovoTidy");
  const initOptions = {
    clangTidyPath: stringOrUndefined(config.get<string>("clangTidyPath")),
    pluginPath:
      stringOrUndefined(config.get<string>("pluginPath")) ??
      bundledPluginPath(context),
    compileCommandsDir: compileDbDir,
    checks: config.get<string>("checks") ?? "lenovo-*",
    extraArgs: config.get<string[]>("extraArgs") ?? [],
  };

  const serverOptions: ServerOptions = {
    run: { command: serverBinary, transport: TransportKind.stdio },
    debug: { command: serverBinary, transport: TransportKind.stdio },
  };
  const clientOptions: LanguageClientOptions = {
    documentSelector: [
      { scheme: "file", language: "c" },
      { scheme: "file", language: "cpp" },
      { scheme: "file", language: "objective-c" },
      { scheme: "file", language: "objective-cpp" },
    ],
    initializationOptions: initOptions,
    diagnosticCollectionName: "lenovo-tidy",
  };

  client = new LanguageClient(
    "lenovoTidy",
    "Lenovo Tidy",
    serverOptions,
    clientOptions
  );
  await client.start();
}

async function restartClient(
  context: ExtensionContext,
  serverBinary: string
): Promise<void> {
  if (client) {
    await client.stop();
    client = undefined;
  }
  await startClient(context, serverBinary);
}

// ---------------------------------------------------------------------------
// compile_commands.json resolution
// ---------------------------------------------------------------------------

/**
 * Resolves the directory that holds compile_commands.json for the current
 * workspace. Order of preference:
 *   1. explicit user setting `lenovoTidy.compileCommandsDir` (if it points
 *      to an existing file);
 *   2. scan of common build output directories;
 *   3. auto-generate for CMake projects (with user confirmation);
 *   4. fallback: show a friendly notification and return undefined so the
 *      LSP still starts for text-only checks (CHN001/SEC001/NAME001/CODE001).
 */
async function ensureCompileCommandsDir(
  context: ExtensionContext,
  force: boolean
): Promise<string | undefined> {
  const wsRoot = workspace.workspaceFolders?.[0]?.uri.fsPath;
  const config = workspace.getConfiguration("lenovoTidy");

  // Step 1 — explicit setting (respect the user if non-empty).
  const userSetting = config.get<string>("compileCommandsDir") ?? "";
  if (userSetting.length > 0) {
    const resolved = userSetting.replace("${workspaceFolder}", wsRoot ?? "");
    if (fileExists(path.join(resolved, "compile_commands.json"))) {
      return resolved;
    }
    // User asked for this path but the file isn't there — fall through
    // to auto-detect so the extension still does something useful.
  }

  if (!wsRoot) return undefined;

  // Step 2 — scan common locations.
  const scanned = scanForDb(wsRoot);
  if (scanned) return scanned;

  // Step 3 — offer automated generation (unless the user opted out
  // for this workspace, except when `force` overrides that).
  const skipped = context.workspaceState.get<boolean>(SKIP_AUTO_KEY, false);
  if (!force && skipped) return undefined;

  return await offerAutoGenerate(context, wsRoot);
}

function scanForDb(wsRoot: string): string | undefined {
  for (const sub of DB_SEARCH_DIRS) {
    const dir = sub === "" ? wsRoot : path.join(wsRoot, sub);
    if (fileExists(path.join(dir, "compile_commands.json"))) {
      return dir;
    }
  }
  return undefined;
}

type ProjectKind = "cmake" | "msbuild" | "make" | "unknown";

function detectProjectKind(wsRoot: string): ProjectKind {
  if (fileExists(path.join(wsRoot, "CMakeLists.txt"))) return "cmake";
  let entries: string[] = [];
  try {
    entries = fs.readdirSync(wsRoot);
  } catch {
    return "unknown";
  }
  if (entries.some((e) => e.toLowerCase().endsWith(".sln"))) return "msbuild";
  if (entries.some((e) => e.toLowerCase().endsWith(".vcxproj"))) {
    return "msbuild";
  }
  if (
    entries.some((e) => e.toLowerCase() === "makefile" || e === "GNUmakefile")
  ) {
    return "make";
  }
  return "unknown";
}

async function offerAutoGenerate(
  context: ExtensionContext,
  wsRoot: string
): Promise<string | undefined> {
  const kind = detectProjectKind(wsRoot);
  const config = workspace.getConfiguration("lenovoTidy");

  switch (kind) {
    case "cmake": {
      if (config.get<boolean>("autoConfigureCmake") === false) {
        return undefined;
      }
      const choice = await window.showInformationMessage(
        "Lenovo Tidy: no compile_commands.json found. Run `cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` now?",
        "Configure",
        "Not now",
        "Never for this workspace"
      );
      if (choice === "Never for this workspace") {
        await context.workspaceState.update(SKIP_AUTO_KEY, true);
        return undefined;
      }
      if (choice !== "Configure") return undefined;
      return await runCmakeConfigure(wsRoot);
    }

    case "msbuild": {
      const choice = await window.showWarningMessage(
        "Lenovo Tidy: detected a Visual Studio Solution/vcxproj. " +
          "compile_commands.json is not produced natively by MSBuild. " +
          "Use the `compdb` tool or migrate to CMake to enable semantic checks.",
        "Copy compdb command",
        "Dismiss"
      );
      if (choice === "Copy compdb command") {
        await env.clipboard.writeText(
          "pip install compiledb && compiledb -n make  # adapt for your build command"
        );
        window.showInformationMessage(
          "Lenovo Tidy: command copied to clipboard."
        );
      }
      return undefined;
    }

    case "make": {
      const choice = await window.showWarningMessage(
        "Lenovo Tidy: Makefile project detected. compile_commands.json must " +
          "be produced with `bear` (Linux/macOS) or `compiledb` (cross-platform).",
        "Copy compiledb command",
        "Dismiss"
      );
      if (choice === "Copy compiledb command") {
        await env.clipboard.writeText("pip install compiledb && compiledb make");
        window.showInformationMessage(
          "Lenovo Tidy: command copied to clipboard."
        );
      }
      return undefined;
    }

    default: {
      window.showWarningMessage(
        "Lenovo Tidy: no compile_commands.json found and no supported build " +
          "system detected. Only text-level rules (CHN001/CODE001/SEC001) " +
          "will fire. Set `lenovoTidy.compileCommandsDir` to point at your " +
          "database or run `Lenovo Tidy: Regenerate compile_commands.json`."
      );
      return undefined;
    }
  }
}

/**
 * Runs `cmake -S <ws> -B <ws>/build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
 * under a progress notification. Returns the build dir on success.
 */
async function runCmakeConfigure(wsRoot: string): Promise<string | undefined> {
  const config = workspace.getConfiguration("lenovoTidy");
  const cmakeExe = config.get<string>("cmakeExecutable") || "cmake";
  const buildDir = path.join(wsRoot, "build");

  try {
    fs.mkdirSync(buildDir, { recursive: true });
  } catch (e) {
    window.showErrorMessage(`Lenovo Tidy: cannot create ${buildDir}: ${e}`);
    return undefined;
  }

  const args = [
    "-S",
    wsRoot,
    "-B",
    buildDir,
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
  ];

  const output = window.createOutputChannel("Lenovo Tidy: cmake configure");
  output.show(true);
  output.appendLine(`$ ${cmakeExe} ${args.map(quoteIfNeeded).join(" ")}`);

  return await window.withProgress(
    {
      location: ProgressLocation.Notification,
      title: "Lenovo Tidy: running cmake configure...",
      cancellable: true,
    },
    async (_progress, token) => {
      return await new Promise<string | undefined>((resolve) => {
        const proc = cp.spawn(cmakeExe, args, { cwd: wsRoot, shell: false });
        token.onCancellationRequested(() => {
          proc.kill();
          output.appendLine("[cancelled]");
          resolve(undefined);
        });
        proc.stdout.on("data", (d: Buffer) => output.append(d.toString()));
        proc.stderr.on("data", (d: Buffer) => output.append(d.toString()));
        proc.on("error", (err) => {
          output.appendLine(`[spawn error] ${err.message}`);
          window
            .showErrorMessage(
              `Lenovo Tidy: failed to launch \`${cmakeExe}\`. Is CMake installed and on PATH?`,
              "Open settings"
            )
            .then((c) => {
              if (c === "Open settings") {
                commands.executeCommand(
                  "workbench.action.openSettings",
                  "lenovoTidy.cmakeExecutable"
                );
              }
            });
          resolve(undefined);
        });
        proc.on("close", (code) => {
          output.appendLine(`[exit ${code}]`);
          if (
            code === 0 &&
            fileExists(path.join(buildDir, "compile_commands.json"))
          ) {
            window.showInformationMessage(
              `Lenovo Tidy: compile_commands.json generated in ${buildDir}.`
            );
            resolve(buildDir);
          } else {
            window.showErrorMessage(
              `Lenovo Tidy: cmake configure exited with code ${code}. See output for details.`,
              "Open output"
            ).then((c) => {
              if (c === "Open output") output.show();
            });
            resolve(undefined);
          }
        });
      });
    }
  );
}

// ---------------------------------------------------------------------------
// bundled binaries
// ---------------------------------------------------------------------------

function resolveServerBinary(ctx: ExtensionContext): string | undefined {
  const platform = process.platform;
  const arch = process.arch;
  const dir = path.join(
    ctx.extensionPath,
    "server-bin",
    `${platform}-${arch}`
  );
  const exe = platform === "win32" ? "lenovo-tidy-lsp.exe" : "lenovo-tidy-lsp";
  const full = path.join(dir, exe);
  if (fileExists(full)) {
    return full;
  }
  const fallback = path.join(ctx.extensionPath, "server-bin", exe);
  return fileExists(fallback) ? fallback : undefined;
}

function bundledPluginPath(ctx: ExtensionContext): string | undefined {
  const platform = process.platform;
  const arch = process.arch;
  const ext =
    platform === "win32" ? "dll" : platform === "darwin" ? "dylib" : "so";
  const dir = path.join(ctx.extensionPath, "server-bin", `${platform}-${arch}`);
  const candidate = path.join(
    dir,
    platform === "win32"
      ? `LenovoTidyChecks.${ext}`
      : `libLenovoTidyChecks.${ext}`
  );
  return fileExists(candidate) ? candidate : undefined;
}

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

function fileExists(p: string): boolean {
  try {
    return fs.existsSync(p);
  } catch {
    return false;
  }
}

function stringOrUndefined(value: string | undefined): string | undefined {
  return value && value.length > 0 ? value : undefined;
}

function quoteIfNeeded(arg: string): string {
  return /\s/.test(arg) ? `"${arg}"` : arg;
}

// Silence "unused" warning in case a future refactor drops this export.
export const __internal__ = { extensionContext, scanForDb, detectProjectKind };
