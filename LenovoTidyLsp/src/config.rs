//! Runtime configuration assembled from the LSP `initializationOptions` plus
//! environment variable fallbacks.

use std::path::PathBuf;

use serde::Deserialize;

/// Raw shape of the `initializationOptions` JSON sent by the LSP client.
#[derive(Debug, Default, Clone, Deserialize)]
#[serde(default, rename_all = "camelCase")]
pub struct InitOptions {
    pub clang_tidy_path: Option<String>,
    pub plugin_path: Option<String>,
    pub compile_commands_dir: Option<String>,
    pub checks: Option<String>,
    /// Extra arguments forwarded verbatim to clang-tidy.
    pub extra_args: Option<Vec<String>>,
}

/// Resolved configuration used by the driver. Required fields must be present
/// after construction, otherwise the server publishes an informative error.
#[derive(Debug, Clone)]
pub struct Config {
    /// Path to the clang-tidy-compatible executable. May be either the
    /// official `clang-tidy[.exe]` or our bundled `lenovo-clang-tidy[.exe]`
    /// which already has the lenovo-* rules statically linked in.
    pub clang_tidy_path: PathBuf,
    /// Optional path to a `libLenovoTidyChecks.{so,dll,dylib}` to load via
    /// `-load=`. None means "do not pass -load" - which is what we want when
    /// `clang_tidy_path` already points at the bundled lenovo-clang-tidy.
    pub plugin_path: Option<PathBuf>,
    pub compile_commands_dir: Option<PathBuf>,
    pub checks: String,
    pub extra_args: Vec<String>,
}

impl Config {
    pub fn from_options(opts: InitOptions) -> Result<Self, ConfigError> {
        // Look for a `lenovo-clang-tidy[.exe]` shipped next to the LSP server
        // executable; this is what the VS Code / VS 2022 extensions stage
        // under `server-bin/win32-x64/`.
        let bundled_lenovo = std::env::current_exe()
            .ok()
            .and_then(|p| p.parent().map(|p| p.to_path_buf()))
            .and_then(|dir| {
                ["lenovo-clang-tidy.exe", "lenovo-clang-tidy"]
                    .iter()
                    .map(|name| dir.join(name))
                    .find(|p| p.exists())
            });

        let clang_tidy_path: PathBuf = opts
            .clang_tidy_path
            .filter(|s| !s.is_empty())
            .map(PathBuf::from)
            .or_else(|| std::env::var("LENOVO_CLANG_TIDY").ok().map(PathBuf::from))
            .or(bundled_lenovo)
            .or_else(|| which::which("lenovo-clang-tidy").ok())
            .or_else(|| which::which("clang-tidy-18").ok())
            .or_else(|| which::which("clang-tidy").ok())
            .ok_or(ConfigError::MissingClangTidy)?;

        // Plugin is optional: required only when clang_tidy_path is the
        // upstream clang-tidy on Linux/macOS (where -load actually works).
        //
        // SAFETY: When the executable is our bundled `lenovo-clang-tidy[.exe]`,
        // the rules are already statically linked into the binary AND the
        // binary contains its own copy of LLVM's ManagedStatic globals. Loading
        // a separate `LenovoTidyChecks.dll` (which also embeds those globals)
        // into the same process can crash on Windows due to double-init of the
        // shared LLVM state. So we ignore plugin_path entirely in that case,
        // even if the IDE client supplied one.
        let is_bundled_lenovo = clang_tidy_path
            .file_stem()
            .and_then(|s| s.to_str())
            .map(|s| s.eq_ignore_ascii_case("lenovo-clang-tidy"))
            .unwrap_or(false);

        let plugin_path = if is_bundled_lenovo {
            None
        } else {
            opts.plugin_path
                .filter(|s| !s.is_empty())
                .or_else(|| std::env::var("LENOVO_TIDY_PLUGIN").ok())
                .map(PathBuf::from)
        };

        let compile_commands_dir = opts
            .compile_commands_dir
            .filter(|s| !s.is_empty())
            .or_else(|| std::env::var("LENOVO_COMPILE_DB").ok())
            .map(PathBuf::from);

        Ok(Config {
            clang_tidy_path,
            plugin_path,
            compile_commands_dir,
            checks: opts.checks.unwrap_or_else(|| "lenovo-*".to_owned()),
            extra_args: opts.extra_args.unwrap_or_default(),
        })
    }
}

#[derive(Debug, thiserror::Error)]
pub enum ConfigError {
    #[error(
        "no clang-tidy executable found; bundle lenovo-clang-tidy next to the \
         LSP server, set initializationOptions.clangTidyPath, or put clang-tidy \
         on PATH"
    )]
    MissingClangTidy,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn defaults_to_lenovo_check_filter() {
        let cfg = Config::from_options(InitOptions {
            clang_tidy_path: Some("/usr/bin/clang-tidy".into()),
            plugin_path: Some("/tmp/lib.so".into()),
            ..Default::default()
        })
        .unwrap();
        assert_eq!(cfg.checks, "lenovo-*");
        assert_eq!(cfg.plugin_path.as_deref(), Some(std::path::Path::new("/tmp/lib.so")));
    }

    #[test]
    fn plugin_path_optional_when_using_bundled_lenovo_clang_tidy() {
        let cfg = Config::from_options(InitOptions {
            clang_tidy_path: Some("/opt/lenovo-clang-tidy".into()),
            plugin_path: None,
            ..Default::default()
        })
        .unwrap();
        assert!(cfg.plugin_path.is_none());
    }

    #[test]
    fn empty_string_treated_as_unset() {
        let cfg = Config::from_options(InitOptions {
            clang_tidy_path: Some("/usr/bin/clang-tidy".into()),
            plugin_path: Some(String::new()),
            ..Default::default()
        })
        .unwrap();
        assert!(cfg.plugin_path.is_none());
    }

    #[test]
    fn bundled_lenovo_clang_tidy_ignores_plugin_path() {
        // Even when the IDE client passes a plugin_path, we MUST NOT propagate
        // it to lenovo-clang-tidy because that would double-init LLVM's
        // ManagedStatic globals in the same process and likely crash.
        for tidy_path in [
            "/opt/lenovo-clang-tidy",
            "C:\\extension\\server-bin\\win32-x64\\lenovo-clang-tidy.exe",
            "lenovo-clang-tidy.exe",
            "LENOVO-CLANG-TIDY.EXE", // case-insensitive on Windows
        ] {
            let cfg = Config::from_options(InitOptions {
                clang_tidy_path: Some(tidy_path.into()),
                plugin_path: Some("/should/be/ignored.dll".into()),
                ..Default::default()
            })
            .unwrap();
            assert!(
                cfg.plugin_path.is_none(),
                "plugin_path should be suppressed for bundled binary at {tidy_path:?}"
            );
        }
    }
}
