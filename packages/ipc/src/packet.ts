import { encode, decode } from "./vlq";
import { Read } from "./io";

export function readNextVlq(reader: Read): number {
  const peek = new Uint8Array(1);
  const buf: number[] = [];

  while (true) {
    reader.readExact(peek);
    buf.push(peek[0]);
    if ((peek[0] & 0x80) === 0) {
      break;
    }
  }
  const result = decode(new Uint8Array(buf));
  return result.value;
}

export interface Packet {
  version(): number;
  payload(): Uint8Array;
  serialize(): Uint8Array;
}

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
    const payload = new Uint8Array(payloadLength);

    reader.readExact(payload);

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
    return `RequestPacket, ${this.payload_.length} bytes payload: ${new TextDecoder().decode(this.payload_)}`;
  }
}

// Response packet implementation
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
    const payload = new Uint8Array(payloadLength);

    reader.readExact(payload);

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
    return `ResponsePacket, error_code: ${this.errorCode_}, ${this.payload_.length} bytes payload: ${new TextDecoder().decode(this.payload_)}`;
  }
}
