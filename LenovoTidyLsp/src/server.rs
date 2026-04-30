//! tower-lsp `Backend` implementation. Holds the resolved [`Config`] plus a
//! per-document cache of the latest text so we can map byte offsets reported
//! by clang-tidy back to LSP positions.

use std::collections::HashMap;
use std::sync::Arc;

use tokio::sync::Mutex;
use tower_lsp::jsonrpc::Result;
use tower_lsp::lsp_types::*;
use tower_lsp::{Client, LanguageServer};

use crate::config::{Config, InitOptions};
use crate::driver;
use crate::parser;

pub struct Backend {
    client: Client,
    config: Mutex<Option<Config>>,
    documents: Mutex<HashMap<Url, String>>,
}

impl Backend {
    pub fn new(client: Client) -> Self {
        Self {
            client,
            config: Mutex::new(None),
            documents: Mutex::new(HashMap::new()),
        }
    }

    async fn analyse(&self, uri: Url) {
        let cfg = match self.config.lock().await.clone() {
            Some(c) => c,
            None => return,
        };
        let path = match uri.to_file_path() {
            Ok(p) => p,
            Err(_) => return,
        };
        let document_text = self
            .documents
            .lock()
            .await
            .get(&uri)
            .cloned()
            .unwrap_or_default();

        match driver::run_clang_tidy(&path, &cfg).await {
            Ok(out) => {
                let lenovo_diags = parser::parse(&out.yaml, uri.as_str());
                let lsp_diags: Vec<Diagnostic> = lenovo_diags
                    .iter()
                    .map(|d| d.to_lsp(&document_text))
                    .collect();
                self.client.publish_diagnostics(uri, lsp_diags, None).await;
            }
            Err(err) => {
                tracing::error!(?err, "clang-tidy run failed");
                self.client
                    .show_message(MessageType::ERROR, format!("lenovo-tidy: {err}"))
                    .await;
            }
        }
    }
}

#[tower_lsp::async_trait]
impl LanguageServer for Backend {
    async fn initialize(&self, params: InitializeParams) -> Result<InitializeResult> {
        let opts = params
            .initialization_options
            .and_then(|v| serde_json::from_value::<InitOptions>(v).ok())
            .unwrap_or_default();

        match Config::from_options(opts) {
            Ok(cfg) => {
                tracing::info!(?cfg.clang_tidy_path, ?cfg.plugin_path, "loaded config");
                *self.config.lock().await = Some(cfg);
            }
            Err(err) => {
                tracing::warn!(%err, "configuration incomplete; will surface on first save");
            }
        }

        Ok(InitializeResult {
            server_info: Some(ServerInfo {
                name: "lenovo-tidy-lsp".to_owned(),
                version: Some(env!("CARGO_PKG_VERSION").to_owned()),
            }),
            capabilities: ServerCapabilities {
                text_document_sync: Some(TextDocumentSyncCapability::Kind(
                    TextDocumentSyncKind::FULL,
                )),
                ..Default::default()
            },
        })
    }

    async fn initialized(&self, _: InitializedParams) {
        self.client
            .log_message(MessageType::INFO, "lenovo-tidy-lsp ready")
            .await;
    }

    async fn did_open(&self, params: DidOpenTextDocumentParams) {
        let uri = params.text_document.uri.clone();
        self.documents
            .lock()
            .await
            .insert(uri.clone(), params.text_document.text);
        self.analyse(uri).await;
    }

    async fn did_change(&self, params: DidChangeTextDocumentParams) {
        let uri = params.text_document.uri.clone();
        if let Some(change) = params.content_changes.into_iter().next() {
            self.documents.lock().await.insert(uri, change.text);
        }
    }

    async fn did_save(&self, params: DidSaveTextDocumentParams) {
        let uri = params.text_document.uri;
        if let Some(text) = params.text {
            self.documents.lock().await.insert(uri.clone(), text);
        }
        self.analyse(uri).await;
    }

    async fn did_close(&self, params: DidCloseTextDocumentParams) {
        let uri = params.text_document.uri;
        self.documents.lock().await.remove(&uri);
        self.client
            .publish_diagnostics(uri, Vec::new(), None)
            .await;
    }

    async fn shutdown(&self) -> Result<()> {
        Ok(())
    }
}

// Provide a constructor signature compatible with `LspService::new`.
impl Backend {
    pub fn new_arc(client: Client) -> Arc<Self> {
        Arc::new(Self::new(client))
    }
}
