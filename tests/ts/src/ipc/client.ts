import { SCRIPT_HASH_TYPE_TYPE } from "@ckb-js-std/bindings";
import { log } from "@ckb-js-std/core";
import { Channel, RequestPacket, spawnCellServer } from "@ckb-js-std/ipc";

function main() {
  log.setLevel(log.LogLevel.Debug);
  log.debug("[CLIENT]: start client");
  let jsVmCodeHash = new Uint8Array([
    0xb9, 0x51, 0x23, 0xc7, 0x1a, 0x87, 0x0e, 0x3f, 0x0f, 0x74, 0xa7, 0xee,
    0x1d, 0xab, 0x82, 0x68, 0xdb, 0xfb, 0xc1, 0x40, 0x7b, 0x46, 0x73, 0x3e,
    0xbd, 0x1b, 0x41, 0xf8, 0x54, 0xb4, 0x32, 0x4a,
  ]);
  // hash_type: type, 33 bytes
  let serverCodeHash =
    "874aeda01948b3d1de6790d36ffab3cc6abe2ca5049b2b9762500fd0202a343d01";

  let [readPipe, writePipe] = spawnCellServer(
    jsVmCodeHash.buffer,
    SCRIPT_HASH_TYPE_TYPE,
    ["-t", serverCodeHash],
  );

  let channel = new Channel(readPipe, writePipe);
  let req = new RequestPacket(new Uint8Array([1, 2, 3]));
  for (let i = 0; i < 3; i++) {
    log.debug("[CLIENT]: send request %s", req.toString());
    let res = channel.call(req);
    log.debug("[CLIENT]: receive response %s", res.toString());
    console.assert(res.payload()[0] === 42);
  }
}

main();
