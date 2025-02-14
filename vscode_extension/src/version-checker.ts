import * as vscode from 'vscode';
import * as semver from 'semver';

export class VersionChecker {
    private static readonly VERSION_KEY = 'lastSeenVersion';
    private context: vscode.ExtensionContext;

    constructor(context: vscode.ExtensionContext) {
        this.context = context;
    }

    public async checkForMajorUpdate(): Promise<void> {
        const extension = vscode.extensions.getExtension(
            this.context.extension.id
        );
        if (!extension) {
            return;
        }

        const currentVersion = extension.packageJSON.version;
        const lastSeenVersion = this.context.globalState.get<string>(
            VersionChecker.VERSION_KEY
        );

        // If this is the first time running the extension
        if (!lastSeenVersion) {
            await this.context.globalState.update(
                VersionChecker.VERSION_KEY,
                currentVersion
            );
            return;
        }

        // Check if there's a major version change
        if (semver.major(currentVersion) > semver.major(lastSeenVersion)) {
            const action = await vscode.window.showWarningMessage(
                `KCD2 Lua Runner has been updated to version ${currentVersion}. ` +
                    'Please update your vscodelua.asi file to ensure compatibility.',
                'Download Latest ASI',
                'Dismiss'
            );

            if (action === 'Download Latest ASI') {
                vscode.env.openExternal(
                    vscode.Uri.parse(
                        'https://github.com/yobson1/kcd2lua/releases/latest'
                    )
                );
            }
        }

        // Update the stored version
        await this.context.globalState.update(
            VersionChecker.VERSION_KEY,
            currentVersion
        );
    }
}
