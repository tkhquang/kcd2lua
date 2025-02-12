import * as vscode from 'vscode';
import * as net from 'net';
import { minify } from 'luamin';

let socket: net.Socket | null = null;
let outputChannel: vscode.OutputChannel | null = null;

function connectToGame(): Promise<boolean> {
    return new Promise((resolve) => {
        if (socket) {
            resolve(true);
            return;
        }

        socket = new net.Socket();

        socket.connect(28771, '127.0.0.1', () => {
            outputChannel = vscode.window.createOutputChannel('Lua Output');
            outputChannel.show();
            resolve(true);
        });

        socket.on('error', (error) => {
            vscode.window.showErrorMessage('Failed to connect to game.');
            socket = null;
            outputChannel?.appendLine('Connection error: ' + error.message);
            resolve(false);
        });

        socket.on('end', () => {
            socket = null;
            outputChannel?.appendLine('Disconnected from game.');
        });

        socket.on('data', (data) => {
            const response = data.toString();
            if (response.startsWith('Lua Error')) {
                outputChannel?.appendLine(`[Error]: ${response}`);
            } else {
                outputChannel?.appendLine(`[Output]: ${response}`);
            }
        });
    });
}

async function sendLuaCode(code: string): Promise<void> {
    return new Promise((resolve, reject) => {
        if (!socket) {
            reject(new Error('No socket connection'));
            return;
        }

        // Add length prefix to the code and null terminator
        const codeWithNull = code + '\0';
        const length = Buffer.byteLength(codeWithNull);
        const lengthBuffer = Buffer.alloc(4);
        lengthBuffer.writeUInt32LE(length, 0);

        // Create a single buffer with length prefix and null-terminated code
        const finalBuffer = Buffer.concat([
            lengthBuffer,
            Buffer.from(codeWithNull)
        ]);

        // Send as a single write
        socket.write(finalBuffer, (err) => {
            if (err) {
                reject(err);
            } else {
                resolve();
            }
        });
    });
}

async function runLua(document?: vscode.TextDocument) {
    // If document is not provided, get it from active editor
    if (!document) {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            return;
        }
        document = editor.document;
    }

    // Only process Lua files
    if (document.languageId !== 'lua') {
        return;
    }

    const luaCode = document.getText();
    if (!luaCode) {
        vscode.window.showErrorMessage('No Lua code found in the document.');
        return;
    }

    if (!socket && !(await connectToGame())) {
        return;
    }

    try {
        const minifiedCode = minify(luaCode);
        await sendLuaCode(minifiedCode);
    } catch (error) {
        const errorMessage = error instanceof Error ? error.message : 'Unknown error';
        vscode.window.showErrorMessage('Failed to send Lua code: ' + errorMessage);
    }
}

export function activate(context: vscode.ExtensionContext) {
    // Register the run command
    const runCommand = vscode.commands.registerCommand('kcd2-lua.run', () => runLua());
    context.subscriptions.push(runCommand);

    // Register the onDidSaveTextDocument event handler
    const onSave = vscode.workspace.onDidSaveTextDocument((document) => {
        const config = vscode.workspace.getConfiguration('kcd2-lua');
        if (config.get<boolean>('runOnSave')) {
            runLua(document);
        }
    });
    context.subscriptions.push(onSave);
}

export function deactivate() {
    if (socket) {
        socket.end();
        socket = null;
    }
}
