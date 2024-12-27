import * as fs from "fs";
import * as path from "path";

interface FileMetadata {
  fileNameOffset: number;
  fileNameLength: number;
  fileContentOffset: number;
  fileContentLength: number;
}

interface FileMap {
  [key: string]: string;
}

function getFileSize(filePath: string): number {
  const stats = fs.statSync(filePath);
  return stats.size;
}

function appendToStream(data: Buffer | string, stream: fs.WriteStream): void {
  stream.write(data);
}

function appendFileToStream(filePath: string, stream: fs.WriteStream): void {
  const content = fs.readFileSync(filePath);
  appendToStream(content, stream);
}

function appendStringNullToStream(str: string, stream: fs.WriteStream): void {
  const buffer = Buffer.from(str + "\0");
  appendToStream(buffer, stream);
}

function appendIntegerToStream(num: number, stream: fs.WriteStream): void {
  const buffer = Buffer.alloc(4);
  buffer.writeInt32LE(num);
  appendToStream(buffer, stream);
}

function pack(files: FileMap, outputStream: fs.WriteStream): void {
  const numFiles = Object.keys(files).length;
  appendIntegerToStream(numFiles, outputStream);

  let offset = 0;
  let length = 0;

  // Write metadata
  for (const [name, filePath] of Object.entries(files)) {
    console.log(`packing file ${filePath} to ${name}`);
    appendIntegerToStream(offset, outputStream);
    length = Buffer.byteLength(name) + 1; // include null terminator
    appendIntegerToStream(length, outputStream);
    offset += length;
    appendIntegerToStream(offset, outputStream);
    length = getFileSize(filePath);
    appendIntegerToStream(length, outputStream);
    offset += length;
  }

  // Write actual file data
  for (const [name, filePath] of Object.entries(files)) {
    appendStringNullToStream(name, outputStream);
    appendFileToStream(filePath, outputStream);
  }
}

function createDirectory(dir: string): void {
  fs.mkdirSync(dir, { recursive: true });
}

function unpack(directory: string, fileContent: Buffer): void {
  let position = 0;

  function readInteger(): number {
    const value = fileContent.readInt32LE(position);
    position += 4;
    return value;
  }

  function readStringNull(length: number): string {
    const value = fileContent
      .toString("utf8", position, position + length)
      .replace(/\0$/, "");
    position += length;
    return value;
  }

  function copyToFile(
    directory: string,
    filename: string,
    offset: number,
    length: number,
  ): void {
    const normalizedFilename = filename.replace(/\//g, path.sep);
    const filePath = path.join(directory, normalizedFilename);
    const dir = path.dirname(filePath);

    createDirectory(dir);
    console.log(`unpacking file ${filename} to ${filePath}`);

    const content = fileContent.slice(offset, offset + length);
    fs.writeFileSync(filePath, content);
  }

  const numFiles = readInteger();
  const metadata: FileMetadata[] = [];

  // Read metadata
  for (let i = 0; i < numFiles; i++) {
    metadata.push({
      fileNameOffset: readInteger(),
      fileNameLength: readInteger(),
      fileContentOffset: readInteger(),
      fileContentLength: readInteger(),
    });
  }

  const blobStart = position;

  for (const metadatum of metadata) {
    position = blobStart + metadatum.fileNameOffset;
    const filename = readStringNull(metadatum.fileNameLength);
    position = blobStart + metadatum.fileContentOffset;
    copyToFile(directory, filename, position, metadatum.fileContentLength);
  }
}

function isIdealPath(path: string): boolean {
  return (
    !path.startsWith("/") && !path.startsWith("..") && !/^[a-zA-Z]:/.test(path)
  );
}

function normalizeRelativePath(path: string): string {
  return path.replace(/^\.\//, "").replace(/\/\.\//g, "/");
}

function mustNormalizePath(path: string): string {
  if (!isIdealPath(path)) {
    throw new Error(
      `Either not a relative path or a relative path referring to ..: ${path}`,
    );
  }
  return normalizeRelativePath(path);
}

// Add CLI functionality
function usage(msg?: string): void {
  if (msg) console.log(msg);
  console.log(
    `${process.argv[1]} pack output_file [files] | ${process.argv[1]} unpack input_file [directory]`,
  );
}

function doPack(): void {
  if (process.argv.length === 2) {
    usage("You must specify the output file.");
    process.exit(1);
  }

  const outfile = process.argv[3];
  const stream = fs.createWriteStream(outfile);
  const files: FileMap = {};
  let n = 0;

  if (process.argv.length !== 3) {
    // Read files from command line arguments
    for (let i = 4; i < process.argv.length; i++) {
      n++;
      const file = process.argv[i];
      files[mustNormalizePath(file)] = file;
    }
  } else {
    // Read files from stdin
    const input = fs.readFileSync(0, "utf-8"); // 0 is stdin
    for (const file of input.split("\n").filter(Boolean)) {
      n++;
      files[mustNormalizePath(file)] = file;
    }
  }

  if (n === 0) {
    usage("You must at least specify one file to pack");
    process.exit(1);
  }

  pack(files, stream);
  stream.end();
}

function doUnpack(): void {
  if (process.argv.length === 2) {
    usage("You must specify the input file when unpacking.");
    process.exit(1);
  }

  const infile = process.argv[3];
  if (!fs.existsSync(infile)) {
    console.error(`Error: File '${infile}' does not exist`);
    process.exit(1);
  }
  const fileContent = fs.readFileSync(infile);

  const directory = process.argv.length !== 3 ? process.argv[4] : ".";
  unpack(directory, fileContent);
}

// Main program
function main(): void {
  if (
    process.argv.length <= 2 ||
    !["pack", "unpack"].includes(process.argv[2])
  ) {
    usage("Please specify whether to pack or unpack");
    process.exit(1);
  }

  if (process.argv[2] === "pack") {
    doPack();
  } else {
    doUnpack();
  }
}

// Only run if this is the main module
if (require.main === module) {
  main();
}

// Export the functions for use as a module
export { pack, unpack, FileMap, FileMetadata };
