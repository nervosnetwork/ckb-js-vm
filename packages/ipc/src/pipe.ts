import { Read, Write } from "./io";
import { read, write } from "@ckb-js-std/bindings";

export class Pipe implements Read, Write {
  private id: number;

  constructor(id: number) {
    this.id = id;
  }

  fd(): number {
    return this.id;
  }

  readable(): boolean {
    return this.id % 2 === 0;
  }

  writable(): boolean {
    return this.id % 2 === 1;
  }

  read(length: number): Uint8Array {
    const buffer = read(this.id, length);
    return new Uint8Array(buffer);
  }

  readExact(length: number): Uint8Array {
    const buffer = new Uint8Array(length);
    let offset = 0;

    while (offset < length) {
      const remainingBytes = length - offset;
      const chunk = this.read(remainingBytes);
      if (chunk.length === 0) {
        throw new Error("Unexpected end of stream");
      }
      buffer.set(chunk, offset);
      offset += chunk.length;
    }

    return buffer;
  }

  write(buffer: Uint8Array): number {
    // The underlying syscall write operation guarantees atomic writes,
    // so we can write the entire buffer at once without needing to loop
    // or handle partial writes like in traditional I/O systems
    write(this.id, buffer.buffer);
    return buffer.length;
  }

  flush(): void {
    // No-op flush operation
  }

  static from(id: number): Pipe {
    return new Pipe(id);
  }
}
