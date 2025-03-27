import { Read, Write } from "./io";
import { RequestPacket, ResponsePacket } from "./packet";

/**
 * Interface for a service that can handle requests and generate responses
 */
export interface RequestHandler {
  serve(req: RequestPacket): ResponsePacket;
}

/**
 * The `Channel` class facilitates IPC communication between a client and a server.
 * It handles the transmission of requests from the client to the server and the reception
 * of responses from the server to the client.
 */
export class Channel {
  private reader: Read;
  private writer: Write;

  /**
   * Creates a new Channel instance
   *
   * @param reader - Responsible for reading data from the channel
   * @param writer - Responsible for writing data to the channel
   */
  constructor(reader: Read, writer: Write) {
    this.reader = reader;
    this.writer = writer;
  }

  /**
   * Executes the server loop, processing incoming requests and sending responses.
   *
   * @param handler - A service that handles requests and generates responses
   */
  execute(handler: RequestHandler): void {
    while (true) {
      const req = this.receiveRequest();
      const resp = handler.serve(req);
      this.sendResponse(resp);
    }
  }

  /**
   * Sends a request to the server and waits for a response.
   *
   * This method handles the complete request-response cycle by sending the request
   * to the server and then waiting for and returning the server's response.
   *
   * @param req - The request packet to send to the server
   * @returns The response packet received from the server
   */
  call(req: RequestPacket): ResponsePacket {
    this.sendRequest(req);
    return this.receiveResponse();
  }

  /**
   * Sends a request to the server
   *
   * @param req - The request to send
   */
  sendRequest(req: RequestPacket): void {
    const bytes = req.serialize();
    this.writer.write(bytes);
    this.writer.flush();
  }

  /**
   * Sends a response to the client
   *
   * @param resp - The response to send
   */
  sendResponse(resp: ResponsePacket): void {
    const bytes = resp.serialize();
    this.writer.write(bytes);
    this.writer.flush();
  }

  /**
   * Sends an error code to the client
   *
   * @param errorCode - The error code to send
   */
  sendErrorCode(errorCode: number): void {
    const packet = new ResponsePacket(errorCode, new Uint8Array(0));
    console.log("Sending error code:", errorCode);

    const bytes = packet.serialize();
    this.writer.write(bytes);
    this.writer.flush();
  }

  /**
   * Receives a request from the client
   *
   * @returns The deserialized request
   */
  receiveRequest(): RequestPacket {
    return RequestPacket.readFrom(this.reader);
  }

  /**
   * Receives a response from the server
   *
   * @returns The deserialized response
   */
  receiveResponse(): ResponsePacket {
    return ResponsePacket.readFrom(this.reader);
  }
}
