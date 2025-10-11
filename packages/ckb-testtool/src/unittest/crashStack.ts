
import { ScriptVerificationResult, Verifier, } from "./core"
import path from "path";
import { readFileSync, existsSync, accessSync } from "fs";
import {
    TraceMap, originalPositionFor,
    GREATEST_LOWER_BOUND, LEAST_UPPER_BOUND
} from "@jridgewell/trace-mapping";

export function printCrashStack(result: ScriptVerificationResult, verifier: Verifier,) {
    if (result.status == 0) {
        console.log("Script Success, no crash stack");
        return;
    }

    const scriptPath = getScriptPath(result, verifier);
    if (scriptPath == undefined) {
        throw "Unknow Script"
    }
    let crashStack = result.stdout.trim();

    crashStack = crashStack.replace(/<run_from_file>/g, scriptPath);

    const map = getScriptMap(scriptPath);
    if (map == undefined) {
        console.log(`(Unknow Map, Inaccurate printing stack)\n${crashStack}`);
        return;
    }

    let dStack = remapStackString(crashStack, scriptPath, map);

    console.log(`dirname: ${path.dirname(scriptPath)}`);
    const projectRoot = findPackageRoot(path.dirname(scriptPath));

    const crashStack2 = replacePathToRootRelative(
        dStack,
        "../src/index.ts",
        scriptPath,
        projectRoot
    );

    console.log(`------------------------------------------------------------
${crashStack2}
------------------------------------------------------------`);
}

function getScriptPath(result: ScriptVerificationResult, verifier: Verifier): string {
    let script;
    if (result.cellType == "input") {
        const cell = verifier.tx.inputs[result.index];
        let inputCell = verifier.resource.cells.get(cell.previousOutput.toBytes().toString());
        if (inputCell == undefined) {
            throw "Unknow Error: not found input cell";
        }

        if (result.groupType == "lock") {
            script = inputCell.cellOutput.lock;
        } else {
            script = inputCell.cellOutput.type;
            if (script == undefined) {
                throw "Unknow Error!";
            }
        }
    } else {
        script = verifier.tx.outputs[result.index].type;
        if (script == undefined) {
            throw "Unknow Error!";
        }
    }
    const scriptPath = verifier.resource.jSScriptsMap.get(script);
    if (scriptPath == undefined) {
        throw "Unknow Script"
    }

    return scriptPath;
}

function getScriptMap(scriptPath: string): TraceMap | undefined {
    const mapPath = scriptPath + ".map";
    if (!existsSync(mapPath)) {
        console.log(`map path: ${mapPath}`);
        return undefined;
    }
    const map = new TraceMap(JSON.parse(readFileSync(mapPath, "utf8")));

    return map;
}

function remapStackString(
    stack: string,
    targetBase: string,
    traceMap: TraceMap,
): string {
    const esc = (s: string) => s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
    const re = new RegExp(
        `(at (?:[^(\n]*\\()?)` +
        `([^()\\s]*${esc(targetBase)}):(\\d+)(?::(\\d+))?` +
        `(\\)?)`,
        "g"
    );

    return stack.replace(
        re,
        (_m, pre: string, _file: string, line: string, col?: string, closer?: string) => {
            const l = parseInt(line, 10);
            const hasCol = col != null && col !== "";
            let pos = originalPositionFor(traceMap, {
                line: l,
                column: hasCol ? parseInt(col!, 10) : Number.MAX_SAFE_INTEGER,
                bias: GREATEST_LOWER_BOUND,
            });
            if (!pos.source || pos.line == null || pos.column == null) {
                pos = originalPositionFor(traceMap, {
                    line: l,
                    column: hasCol ? parseInt(col!, 10) : 0,
                    bias: LEAST_UPPER_BOUND,
                });
            }
            if (!pos.source || pos.line == null || pos.column == null) {
                return `${pre}${_file}:${line}${hasCol ? `:${col}` : ""}${closer ?? ""}`;
            }
            return `${pre}${pos.source}:${pos.line}:${pos.column}${closer ?? ""}`;
        }
    );
}

function findPackageRoot(startDir: string): string {
    const exists = (p: string) => { try { accessSync(p); return true; } catch { return false; } };
    const readJSON = <T = any>(p: string): T | undefined => { try { return JSON.parse(readFileSync(p, "utf8")); } catch { return undefined; } };


    let dir = path.resolve(startDir);
    const fsRoot = path.parse(dir).root;

    let nearestPackageRoot: string | null = null;
    let workspaceRoot: string | null = null;

    while (true) {
        const pkgPath = path.join(dir, "package.json");
        if (exists(pkgPath)) {
            nearestPackageRoot ??= dir;
            const pkg = readJSON<any>(pkgPath);
            if (pkg && (pkg.workspaces || pkg?.packages)) workspaceRoot = dir; // yarn/pnpm workspaces
        }
        if (
            exists(path.join(dir, "pnpm-workspace.yaml")) ||
            exists(path.join(dir, "pnpm-workspace.yml")) ||
            exists(path.join(dir, "lerna.json")) ||
            exists(path.join(dir, "nx.json"))
        ) {
            workspaceRoot = dir;
        }

        if (dir === fsRoot) break;
        dir = path.dirname(dir);
    }

    if (!workspaceRoot) workspaceRoot = nearestPackageRoot ?? path.resolve(startDir);

    return workspaceRoot;

}



function replacePathToRootRelative(
    text: string,
    oldRelFromAnchor: string,
    anchorFileAbs: string,
    rootDirAbs: string
) {
    function toRootRelative(
        targetRelFromAnchor: string,
        anchorFileAbs: string,
        rootDirAbs: string
    ) {
        const targetAbs = path.resolve(path.dirname(anchorFileAbs), targetRelFromAnchor);
        return path.relative(rootDirAbs, targetAbs).split(path.sep).join("/");
    }

    function escapeRegExp(s: string) {
        return s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
    }

    const newRel = toRootRelative(oldRelFromAnchor, anchorFileAbs, rootDirAbs);
    const re = new RegExp(escapeRegExp(oldRelFromAnchor), "g");
    return text.replace(re, newRel);
}
