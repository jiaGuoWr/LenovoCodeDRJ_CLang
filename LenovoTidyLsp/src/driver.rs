//! Spawns clang-tidy, requesting machine-readable diagnostics via
//! `--export-fixes`. Returns the YAML payload for the parser to consume.

use std::path::Path;
use std::process::Stdio;

use tokio::process::Command;

use crate::config::Config;

#[derive(Debug, thiserror::Error)]
pub enum DriverError {
    #[error("failed to spawn clang-tidy: {0}")]
    Spawn(#[from] std::io::Error),
    #[error("clang-tidy exited with status {0:?} and message: {1}")]
    ClangTidyFailed(Option<i32>, String),
}

pub struct DriverOutput {
    /// Raw bytes from `--export-fixes` YAML file; empty when no diagnostics.
    pub yaml: Vec<u8>,
    /// Stderr collected for diagnostics in case of unexpected failures.
    pub stderr: String,
}

pub async fn run_clang_tidy(file: &Path, cfg: &Config) -> Result<DriverOutput, DriverError> {
    let tmp = tempfile::Builder::new()
        .prefix("lenovo-tidy-fixes-")
        .suffix(".yaml")
        .tempfile()?;
    let fixes_path = tmp.path().to_path_buf();

    let mut cmd = Command::new(&cfg.clang_tidy_path);
    if let Some(plugin) = &cfg.plugin_path {
        // Only relevant on Linux/macOS where the upstream `clang-tidy`
        // binary still supports plugin loading. On Windows we ship a
        // self-contained `lenovo-clang-tidy.exe` instead and leave this unset.
        cmd.arg(format!("-load={}", plugin.display()));
    }
    cmd.arg(format!("--checks=-*,{}", cfg.checks))
        .arg(format!("--export-fixes={}", fixes_path.display()))
        .arg("--quiet");

    // Resolve compile_commands.json directory. Priority:
    //   1. Explicit `cfg.compile_commands_dir` (from initializationOptions
    //      or LENOVO_COMPILE_DB env var) - the user knows best.
    //   2. Auto-discovery: walk up from the source file looking for
    //      build/, out/, cmake-build-*/, .vscode/, or the workspace root.
    // Either way we pass `-p <dir>` to clang-tidy.
    let compile_db_dir = cfg.compile_commands_dir.clone().or_else(|| {
        file.parent().and_then(|parent| {
            crate::discovery::find_compile_db(parent, &cfg.compile_commands_search_paths)
        })
    });
    if let Some(p) = &compile_db_dir {
        cmd.arg("-p").arg(p);
    } else {
        tracing::warn!(
            ?file,
            "no compile_commands.json found via initializationOptions or auto-discovery; \
             clang-tidy may produce limited diagnostics"
        );
    }
    for extra in &cfg.extra_args {
        cmd.arg(extra);
    }
    cmd.arg(file)
        .stdin(Stdio::null())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());

    tracing::debug!(?cmd, "running clang-tidy");
    let output = cmd.output().await?;
    let stderr = String::from_utf8_lossy(&output.stderr).into_owned();

    if !output.status.success() && output.status.code().unwrap_or(0) > 1 {
        // exit code 1 typically just means "diagnostics found"; >1 is a real
        // failure (driver crash, file not found, etc.).
        return Err(DriverError::ClangTidyFailed(output.status.code(), stderr));
    }

    let yaml = std::fs::read(&fixes_path).unwrap_or_default();
    Ok(DriverOutput { yaml, stderr })
}
