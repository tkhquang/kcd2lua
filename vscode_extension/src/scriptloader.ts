import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';

interface ScriptInfo {
    filepath: string;
    dependencies: string[];
}

interface StartupScript {
    filepath: string;
}

export class ScriptLoader {
    private processedScripts: Set<string> = new Set();
    private scriptCache: Map<string, ScriptInfo> = new Map();
    private outputChannel: vscode.OutputChannel;

    constructor(private workspaceRoot: string, outputChannel: vscode.OutputChannel) {
        this.outputChannel = outputChannel;
    }

    private log(message: string) {
        this.outputChannel.appendLine(`[ScriptLoader] ${message}`);
    }

    private async findFileInWorkspace(targetPath: string): Promise<string | null> {
        targetPath = targetPath.replace(/\\/g, '/').toLowerCase();
        const files = await vscode.workspace.findFiles('**/*.lua');

        this.log(`Searching for: ${targetPath}`);

        for (const file of files) {
            const relativePath = path.relative(this.workspaceRoot, file.fsPath)
                .replace(/\\/g, '/')
                .toLowerCase();

            if (relativePath.endsWith(targetPath)) {
                this.log(`Found match: ${file.fsPath}`);
                return file.fsPath;
            }
        }

        this.log(`No match found for: ${targetPath}`);
        return null;
    }

    private extractDependencies(content: string, scriptPath: string): string[] {
        const dependencies: string[] = [];
        const regex = /^(?!.*--.*$).*?Script\.ReloadScript\s*(?:\(\s*)?["']([^"']+)["']\s*(?:\))?\s*;?\s*\n?/gm;;
        let match;

        this.log(`Extracting dependencies from: ${path.basename(scriptPath)}`);
        while ((match = regex.exec(content)) !== null) {
            this.log(`Found dependency in ${path.basename(scriptPath)}: ${match[0]}`);
            const dep = match[1].replace(/\\/g, '/');
            dependencies.push(dep);
            this.log(`Found dependency in ${path.basename(scriptPath)}: ${dep}`);
        }

        return dependencies;
    }

    private async loadScript(scriptPath: string, content?: string): Promise<ScriptInfo> {
        if (this.scriptCache.has(scriptPath)) {
            this.log(`Using cached version of: ${path.basename(scriptPath)}`);
            return this.scriptCache.get(scriptPath)!;
        }

        this.log(`Loading script: ${path.basename(scriptPath)}`);

        // If content is provided, use it; otherwise read from file
        const scriptContent = content ?? await fs.promises.readFile(scriptPath, 'utf8');

        const dependencies = this.extractDependencies(scriptContent, scriptPath);

        const scriptInfo: ScriptInfo = {
            filepath: scriptPath,
            dependencies
        };

        this.scriptCache.set(scriptPath, scriptInfo);
        return scriptInfo;
    }

    private async getStartupScripts(): Promise<StartupScript[]> {
        const startupPatterns = [
            '**/[mM][oO][dD][sS]/**/*.lua',
            '**/[sS][tT][aA][rR][tT][uU][pP]/**/*.lua'
        ];

        this.log('Searching for startup scripts...');
        const startupFiles = await vscode.workspace.findFiles(
            `{${startupPatterns.join(',')}}`
        );

		// I'm assuming the game loads them in alphabetical order
        startupFiles.sort((a, b) => a.fsPath.localeCompare(b.fsPath));

        this.log(`Found ${startupFiles.length} startup scripts:`);
        startupFiles.forEach(file => {
            this.log(`- ${path.relative(this.workspaceRoot, file.fsPath)}`);
        });

        return startupFiles.map(file => ({ filepath: file.fsPath }));
    }

    private async processScript(scriptPath: string, accumulator: ScriptInfo[], content?: string): Promise<void> {
        if (this.processedScripts.has(scriptPath)) {
            this.log(`Script already processed, skipping: ${path.basename(scriptPath)}`);
            return;
        }

        this.log(`Processing script: ${path.basename(scriptPath)}`);
        this.processedScripts.add(scriptPath);

        const scriptInfo = await this.loadScript(scriptPath, content);
        accumulator.push(scriptInfo);

        if (scriptInfo.dependencies.length > 0) {
            this.log(`Processing dependencies for ${path.basename(scriptPath)}:`);
        }

        for (const dep of scriptInfo.dependencies) {
            const depPath = await this.findFileInWorkspace(dep);
            if (depPath) {
                await this.processScript(depPath, accumulator);
            } else {
                this.log(`⚠️ WARNING: Could not find dependency: ${dep}`);
            }
        }
    }

    public async processScripts(startupScript?: StartupScript): Promise<ScriptInfo[]> {
        this.log('Starting script processing');
        this.log(`Workspace root: ${this.workspaceRoot}`);

        // Clear caches for fresh processing
        this.processedScripts.clear();
        this.scriptCache.clear();

        const scripts: ScriptInfo[] = [];

        if (startupScript) {
            // Process single script mode
            this.log('Processing single startup script');
            await this.processScript(startupScript.filepath, scripts);
        } else {
            // Process all startup scripts from workspace
            const startupScripts = await this.getStartupScripts();
            for (const script of startupScripts) {
                await this.processScript(script.filepath, scripts);
            }
        }

        this.log('\nFinal script execution order:');
        scripts.forEach((script, index) => {
            this.log(`${index + 1}. ${path.relative(this.workspaceRoot, script.filepath)}`);
        });

        return scripts;
    }
}
