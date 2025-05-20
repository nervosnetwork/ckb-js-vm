# @ckb-js-std/ipc

This is an on-chain script IPC (Inter-Process Communication) implementation in TypeScript, ported from the
[Rust version](https://github.com/XuJiandong/ckb-script-ipc).

## Overview

The package provides IPC functionality for CKB on-chain scripts, allowing communication between different scripts on the blockchain.

## Installation

```bash
  pnpm install @ckb-js-std/ipc
```

## Server Example

```ts
import {
  runServer,
  RequestHandler,
  RequestPacket,
  ResponsePacket,
} from "@ckb-js-std/ipc";

class Serve implements RequestHandler {
  serve(req: RequestPacket): ResponsePacket {
    return new ResponsePacket(0, new Uint8Array([42]));
  }
}

function main() {
  runServer(new Serve());
}

main();
```

## Client Example

```ts
import { SCRIPT_HASH_TYPE_TYPE } from "@ckb-js-std/bindings";
import { Channel, RequestPacket, spawnCellServer } from "@ckb-js-std/ipc";

function main() {
  let jsVmCodeHash = ... // Your ckb-js-vm code hash
  let serverCellLocation = ... // server `code hash` + `hash type` in hex

  // spawn server
  let [readPipe, writePipe] = spawnCellServer(
    jsVmCodeHash.buffer,
    SCRIPT_HASH_TYPE_TYPE,
    ["-t", serverCellLocation],
  );

  let channel = new Channel(readPipe, writePipe);
  let req = new RequestPacket(new Uint8Array([1, 2, 3]));
  let res = channel.call(req);
}

main();

```

## API Reference

### Server

- `runServer(handler: RequestHandler)`: Starts an IPC server with the given request handler
- `RequestHandler`: Interface for handling IPC requests
- `ResponsePacket`: Class representing the response data structure

### Client

- `spawnCellServer`: Spawns a new server cell and returns communication pipes
- `Channel`: Handles communication between client and server
- `RequestPacket`: Class representing the request data structure
