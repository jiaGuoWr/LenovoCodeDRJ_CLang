//! lenovo-tidy-lsp entry point.
//!
//! Wires up tracing, then hands stdin/stdout to tower-lsp.

use tower_lsp::{LspService, Server};

mod config;
mod diagnostic;
mod discovery;
mod driver;
mod parser;
mod server;

#[tokio::main(flavor = "multi_thread")]
async fn main() {
    tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_env("LENOVO_TIDY_LOG")
                .unwrap_or_else(|_| tracing_subscriber::EnvFilter::new("info")),
        )
        .with_writer(std::io::stderr)
        .with_target(false)
        .with_ansi(false)
        .init();

    tracing::info!(
        version = env!("CARGO_PKG_VERSION"),
        "starting lenovo-tidy-lsp",
    );

    let stdin = tokio::io::stdin();
    let stdout = tokio::io::stdout();

    let (service, socket) = LspService::new(server::Backend::new);
    Server::new(stdin, stdout, socket).serve(service).await;
}
