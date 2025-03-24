import { pipe, inheritedFds, spawnCell } from "@ckb-js-std/bindings";
import { Pipe } from "./pipe";
import { Channel } from "./channel";
import { RequestHandler } from "./channel";

/**
 * Spawns a new IPC server process using the provided code hash and hash type.
 *
 * @param codeHash - A byte array representing the code hash of the cell to spawn.
 * @param hashType - The hash type of the cell (e.g., `SCRIPT_HASH_TYPE_TYPE`).
 * @param argv - An array of strings representing the arguments to pass to the new process.
 *
 * @returns A tuple of two `Pipe` objects representing the read and write pipes
 * for the parent process, or throws an `Error` if an error occurs.
 *
 * @example
 * ```typescript
 * import { spawnCellServer } from "@ckb-js-std/ipc";
 * import * as bindings from "@ckb-js-std/core";
 * const codeHash = new Uint8Array([...]);
 * const [readPipe, writePipe] = spawnCellServer(
 *   codeHash,
 *   bindings.SCRIPT_HASH_TYPE_TYPE,
 *   ["demo"]
 * );
 * ```
 */
export function spawnCellServer(
  codeHash: ArrayBuffer,
  hashType: number,
  argv: string[],
): [Pipe, Pipe] {
  const [r1, w1] = pipe();
  const [r2, w2] = pipe();
  const inheritedFds = [r2, w1];

  spawnCell(codeHash, hashType, 0, 0, {
    argv,
    inherited_fds: inheritedFds,
  });

  return [new Pipe(r1), new Pipe(w2)];
}

/**
 * Runs the server with the provided service implementation.
 *
 * This function listens for incoming requests, processes them using the provided service,
 * and sends back the responses. It uses the inherited file descriptors for communication.
 *
 * @param handler - The service implementation that handles the requests and
 *   generates the responses.
 *
 * @returns Never returns unless an error occurs, in which case it throws an `Error`.
 */
export function runServer(handler: RequestHandler): void {
  const fds = inheritedFds();
  if (fds.length !== 2) {
    throw new Error("Expected exactly 2 inherited file descriptors");
  }

  const reader = new Pipe(fds[0]);
  const writer = new Pipe(fds[1]);
  const channel = new Channel(reader, writer);

  channel.execute(handler);
}
