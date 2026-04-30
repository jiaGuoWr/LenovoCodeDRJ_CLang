using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.VisualStudio.LanguageServer.Client;
using Microsoft.VisualStudio.Threading;
using Microsoft.VisualStudio.Utilities;

namespace LenovoTidy
{
    // Attach to whatever ContentType VS may assign to a C/C++ source file.
    // VS's C/C++ project system unilaterally tags .cpp/.h buffers as
    // ContentType="C/C++" (regardless of any FileExtensionToContentTypeDefinition
    // we register), so [ContentType("LenovoTidyCpp")] alone would never fire.
    // We attach to the canonical name AND a few aliases that exist in
    // different VS configurations (older betas used "c" and "cpp"). The
    // [ContentType] attribute allows multiple, all are OR'd by the LSP
    // framework when matching a buffer.
    [ContentType(LenovoTidyContentDefinition.ContentTypeName)]  // LenovoTidyCpp
    [ContentType("C/C++")]
    [ContentType("c")]
    [ContentType("cpp")]
    [Export(typeof(ILanguageClient))]
    public sealed class LenovoTidyClient : ILanguageClient
    {
        public string Name => "Lenovo Tidy";

        public IEnumerable<string> ConfigurationSections => new[] { "lenovoTidy" };

        public object? InitializationOptions
        {
            get
            {
                return new
                {
                    clangTidyPath = Environment.GetEnvironmentVariable("LENOVO_CLANG_TIDY"),
                    pluginPath = ResolveBundledPlugin(),
                    compileCommandsDir = Environment.GetEnvironmentVariable("LENOVO_COMPILE_DB"),
                    checks = "lenovo-*",
                };
            }
        }

        public IEnumerable<string>? FilesToWatch => null;

        public bool ShowNotificationOnInitializeFailed => true;

        public event AsyncEventHandler<EventArgs>? StartAsync;
        public event AsyncEventHandler<EventArgs>? StopAsync;

        public async Task<Connection?> ActivateAsync(CancellationToken token)
        {
            var binDir = Path.Combine(
                Path.GetDirectoryName(typeof(LenovoTidyClient).Assembly.Location)!,
                "server-bin", "win32-x64");
            var serverPath = Path.Combine(binDir, "lenovo-tidy-lsp.exe");
            if (!File.Exists(serverPath))
            {
                throw new FileNotFoundException(
                    "lenovo-tidy-lsp.exe was not bundled with the extension. " +
                    "Rebuild the VSIX with the correct server binary.",
                    serverPath);
            }

            var info = new ProcessStartInfo
            {
                FileName = serverPath,
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };

            var process = new Process { StartInfo = info };
            if (!process.Start())
            {
                throw new InvalidOperationException("Failed to start lenovo-tidy-lsp");
            }
            await Task.Yield();
            return new Connection(process.StandardOutput.BaseStream, process.StandardInput.BaseStream);
        }

        public Task OnLoadedAsync()
        {
            return StartAsync?.InvokeAsync(this, EventArgs.Empty) ?? Task.CompletedTask;
        }

        public Task<InitializationFailureContext?> OnServerInitializeFailedAsync(ILanguageClientInitializationInfo info)
        {
            return Task.FromResult<InitializationFailureContext?>(null);
        }

        public Task OnServerInitializedAsync()
        {
            return Task.CompletedTask;
        }

        private static string? ResolveBundledPlugin()
        {
            var binDir = Path.Combine(
                Path.GetDirectoryName(typeof(LenovoTidyClient).Assembly.Location)!,
                "server-bin", "win32-x64");
            var dll = Path.Combine(binDir, "LenovoTidyChecks.dll");
            return File.Exists(dll) ? dll : null;
        }
    }

    /// <summary>
    /// Registration helpers for our LSP-aware ContentType plus file-extension
    /// mappings. The canonical MEF shape is to export a *static field* typed
    /// as <see cref="ContentTypeDefinition"/> - the class itself is
    /// <c>sealed</c> in the VS utility assembly so a class-level <c>[Export]</c>
    /// silently drops the registration (IContentTypeRegistryService only
    /// picks up exports whose contract is ContentTypeDefinition).
    /// </summary>
    internal static class LenovoTidyContentDefinition
    {
        public const string ContentTypeName = "LenovoTidyCpp";

        // Register our LSP-aware ContentType. Derives from "C/C++" so we
        // inherit editor features, and from "code-languageserver-preview" so
        // VS's LSP framework will actually dispatch to ILanguageClients.
        [Export]
        [Name(ContentTypeName)]
        [BaseDefinition("C/C++")]
        [BaseDefinition(CodeRemoteContentDefinition.CodeRemoteContentTypeName)]
        internal static ContentTypeDefinition LenovoTidyContentTypeDefinition = null!;

        // Defensive augmentation of the built-in "C/C++" ContentType: VS's
        // C++ project system unilaterally assigns "C/C++" to any file inside
        // a vcxproj, bypassing the FileExtensionRegistry. Without this entry
        // the LSP framework refuses to dispatch because "C/C++" has no
        // code-languageserver-preview ancestor. MEF aggregates BaseDefinitions
        // across multiple exports of the same Name, so this just adds a base
        // type to VS's existing definition.
        [Export]
        [Name("C/C++")]
        [BaseDefinition(CodeRemoteContentDefinition.CodeRemoteContentTypeName)]
        internal static ContentTypeDefinition CCppLspBaseAugmentation = null!;

        [Export]
        [FileExtension(".cpp")]
        [ContentType(ContentTypeName)]
        internal static FileExtensionToContentTypeDefinition CppDefinition = null!;

        [Export]
        [FileExtension(".h")]
        [ContentType(ContentTypeName)]
        internal static FileExtensionToContentTypeDefinition HDefinition = null!;

        [Export]
        [FileExtension(".cc")]
        [ContentType(ContentTypeName)]
        internal static FileExtensionToContentTypeDefinition CcDefinition = null!;

        [Export]
        [FileExtension(".hpp")]
        [ContentType(ContentTypeName)]
        internal static FileExtensionToContentTypeDefinition HppDefinition = null!;

        [Export]
        [FileExtension(".cxx")]
        [ContentType(ContentTypeName)]
        internal static FileExtensionToContentTypeDefinition CxxDefinition = null!;

        [Export]
        [FileExtension(".hxx")]
        [ContentType(ContentTypeName)]
        internal static FileExtensionToContentTypeDefinition HxxDefinition = null!;

        [Export]
        [FileExtension(".c")]
        [ContentType(ContentTypeName)]
        internal static FileExtensionToContentTypeDefinition CDefinition = null!;
    }
}
