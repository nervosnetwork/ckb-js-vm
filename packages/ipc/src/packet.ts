import { encode, decode } from "./vlq";
import { Read } from "./io";

export function readNextVlq(reader: Read): number {
  const buf: number[] = [];

  while (true) {
    const peek = reader.readExact(1);
    buf.push(peek[0]);
    if ((peek[0] & 0x80) === 0) {
      break;
    }
  }
  const result = decode(new Uint8Array(buf));
  return result.value;
}

/**
 * Represents a generic packet in the IPC communication protocol.
 *
 * This interface defines the common methods that all packet types must implement.
 * It includes methods for accessing the version, payload, and serializing the packet.
 */
export interface Packet {
  version(): number;
  payload(): Uint8Array;
  serialize(): Uint8Array;
}

/**
 * RequestPacket represents a client request to the server.
 *
 * Format: [version][method_id][payload_length][payload]
 *
 * See [wire format](https://github.com/XuJiandong/ckb-script-ipc?tab=readme-ov-file#wire-format) for detailed definitions.
 */
export class RequestPacket implements Packet {
  private version_: number;
  private methodId_: number;
  private payload_: Uint8Array;

  constructor(payload: Uint8Array, methodId: number = 0, version: number = 0) {
    this.version_ = version;
    this.methodId_ = methodId;
    this.payload_ = payload;
  }

  static readFrom(reader: Read): RequestPacket {
    const version = readNextVlq(reader) as number;
    const methodId = readNextVlq(reader);
    const payloadLength = readNextVlq(reader);

    const payload = reader.readExact(payloadLength);

    return new RequestPacket(payload, methodId, version);
  }

  version(): number {
    return this.version_;
  }

  payload(): Uint8Array {
    return this.payload_;
  }

  methodId(): number {
    return this.methodId_;
  }

  serialize(): Uint8Array {
    const versionBytes = encode(this.version_);
    const methodIdBytes = encode(this.methodId_);
    const payloadLengthBytes = encode(this.payload_.length);

    const result = new Uint8Array(
      versionBytes.length +
        methodIdBytes.length +
        payloadLengthBytes.length +
        this.payload_.length,
    );

    let offset = 0;
    result.set(versionBytes, offset);
    offset += versionBytes.length;

    result.set(methodIdBytes, offset);
    offset += methodIdBytes.length;

    result.set(payloadLengthBytes, offset);
    offset += payloadLengthBytes.length;

    result.set(this.payload_, offset);

    return result;
  }

  toString(): string {
    return `RequestPacket, method_id: ${this.methodId_}, ${this.payload_.length} bytes payload`;
  }
}

/**
 * Represents a response packet in the IPC communication protocol.
 *
 * ResponsePacket contains the server's response to a client request, including
 * an error code and payload data. The error code indicates success (0) or
 * various error conditions.
 *
 * Format: [version][error_code][payload_length][payload]
 *
 * See [wire format](https://github.com/XuJiandong/ckb-script-ipc?tab=readme-ov-file#wire-format) for detailed definitions.
 */
export class ResponsePacket implements Packet {
  private version_: number;
  private errorCode_: number;
  private payload_: Uint8Array;

  constructor(errorCode: number, payload: Uint8Array, version: number = 0) {
    this.version_ = version;
    this.errorCode_ = errorCode;
    this.payload_ = payload;
  }

  static readFrom(reader: Read): ResponsePacket {
    const version = readNextVlq(reader) as number;
    const errorCode = readNextVlq(reader);
    const payloadLength = readNextVlq(reader);

    const payload = reader.readExact(payloadLength);

    return new ResponsePacket(errorCode, payload, version);
  }

  version(): number {
    return this.version_;
  }

  payload(): Uint8Array {
    return this.payload_;
  }

  errorCode(): number {
    return this.errorCode_;
  }

  serialize(): Uint8Array {
    const versionBytes = encode(this.version_);
    const errorCodeBytes = encode(this.errorCode_);
    const payloadLengthBytes = encode(this.payload_.length);

    const result = new Uint8Array(
      versionBytes.length +
        errorCodeBytes.length +
        payloadLengthBytes.length +
        this.payload_.length,
    );

    let offset = 0;
    result.set(versionBytes, offset);
    offset += versionBytes.length;

    result.set(errorCodeBytes, offset);
    offset += errorCodeBytes.length;

    result.set(payloadLengthBytes, offset);
    offset += payloadLengthBytes.length;

    result.set(this.payload_, offset);

    return result;
  }

  toString(): string {
    return `ResponsePacket, error_code: ${this.errorCode_}, ${this.payload_.length} bytes payload`;
  }
}
