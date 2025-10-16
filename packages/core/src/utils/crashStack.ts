
export function logError(e: unknown): void {
    const root = e instanceof Error ? e : new Error(String(e));
    const lines: string[] = [];
    const dump = (err: any, prefix = "") => {
        lines.push(prefix + (err?.stack || `${err?.name || "Error"}: ${err?.message || String(err)}`));
        if (err instanceof AggregateError && Array.isArray(err.errors)) {
            for (let i = 0; i < err.errors.length; i++) dump(err.errors[i], prefix + `  [${i}] `);
        }
        if (err?.cause) dump(err.cause, prefix + "Caused by: ");
    };
    dump(root);
    console.log(lines.join("\n"));
}
