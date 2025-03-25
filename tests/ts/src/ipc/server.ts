import { log } from "@ckb-js-std/core";
import {
  runServer,
  RequestHandler,
  RequestPacket,
  ResponsePacket,
} from "@ckb-js-std/ipc";

class Serve implements RequestHandler {
  serve(req: RequestPacket): ResponsePacket {
    log.debug("[SERVER]: receive request %s", req.toString());
    return new ResponsePacket(0, new Uint8Array([42]));
  }
}

function main() {
  log.setLevel(log.LogLevel.Debug);
  log.debug("[SERVER]: start server");
  runServer(new Serve());
}

main();
