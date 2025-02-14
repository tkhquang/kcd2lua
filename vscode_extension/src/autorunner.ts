import * as vscode from 'vscode';

export class AutoRunner {
    private fileWatcher: vscode.Disposable | null = null;
    private readonly configSection = 'kcd2-lua';
    private readonly runOnSaveKey = 'runOnSave';

    constructor(
        private context: vscode.ExtensionContext,
        private runScript: () => Promise<void>
    ) {
        // Initial setup
        this.updateFileWatcher();

        // Listen for configuration changes
        context.subscriptions.push(
            vscode.workspace.onDidChangeConfiguration((e) => {
                if (
                    e.affectsConfiguration(
                        `${this.configSection}.${this.runOnSaveKey}`
                    )
                ) {
                    this.updateFileWatcher();
                }
            })
        );
    }

    private updateFileWatcher(): void {
        // Clean up existing file watcher if it exists
        if (this.fileWatcher) {
            this.fileWatcher.dispose();
            this.fileWatcher = null;
        }

        // Check current configuration
        const config = vscode.workspace.getConfiguration(this.configSection);
        const runOnSave = config.get<boolean>(this.runOnSaveKey, false);

        // Create new file watcher if enabled
        if (runOnSave) {
            this.fileWatcher = vscode.workspace.onDidSaveTextDocument(
                (document) => {
                    if (document.languageId === 'lua') {
                        this.runScript();
                    }
                }
            );

            // Add to subscriptions so it gets cleaned up on deactivate
            this.context.subscriptions.push(this.fileWatcher);
        }
    }
}
