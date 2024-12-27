import * as fs from "node:fs/promises";
import * as fsSync from "node:fs";
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

async function getFileSize(filePath: string): Promise<number> {
  const stats = await fs.stat(filePath);
  return stats.size;
}

async function writeToFile(
  data: Buffer | string,
  writeStream: fsSync.WriteStream,
): Promise<void> {
  return new Promise((resolve, reject) => {
    const canContinue = writeStream.write(data);
    if (canContinue) {
      resolve();
    } else {
      writeStream.once("drain", resolve);
      writeStream.once("error", reject);
    }
  });
}

async function appendFileToStream(
  filePath: string,
  stream: fsSync.WriteStream,
): Promise<void> {
  const content = await fs.readFile(filePath);
  await writeToFile(content, stream);
}

async function appendStringNullToStream(
  str: string,
  stream: fsSync.WriteStream,
): Promise<void> {
  const buffer = Buffer.from(str + "\0");
  await writeToFile(buffer, stream);
}

async function appendIntegerToStream(
  num: number,
  stream: fsSync.WriteStream,
): Promise<void> {
  const buffer = Buffer.alloc(4);
  buffer.writeInt32LE(num);
  await writeToFile(buffer, stream);
}

async function pack(
  files: FileMap,
  outputStream: fsSync.WriteStream,
): Promise<void> {
  const numFiles = Object.keys(files).length;
  await appendIntegerToStream(numFiles, outputStream);

  let offset = 0;
  let length = 0;

  // Write metadata
  for (const [name, filePath] of Object.entries(files)) {
    console.log(`packing file ${filePath} to ${name}`);
    await appendIntegerToStream(offset, outputStream);
    length = Buffer.byteLength(name) + 1;
    await appendIntegerToStream(length, outputStream);
    offset += length;
    await appendIntegerToStream(offset, outputStream);
    length = await getFileSize(filePath);
    await appendIntegerToStream(length, outputStream);
    offset += length;
  }

  // Write actual file data
  for (const [name, filePath] of Object.entries(files)) {
    await appendStringNullToStream(name, outputStream);
    await appendFileToStream(filePath, outputStream);
  }
}

async function createDirectory(dir: string): Promise<void> {
  await fs.mkdir(dir, { recursive: true });
}

async function unpack(directory: string, fileContent: Buffer): Promise<void> {
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

  async function copyToFile(
    directory: string,
    filename: string,
    offset: number,
    length: number,
  ): Promise<void> {
    const normalizedFilename = filename.replace(/\//g, path.sep);
    const filePath = path.join(directory, normalizedFilename);
    const dir = path.dirname(filePath);

    await createDirectory(dir);
    console.log(`unpacking file ${filename} to ${filePath}`);

    const content = fileContent.slice(offset, offset + length);
    await fs.writeFile(filePath, content);
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
    await copyToFile(
      directory,
      filename,
      position,
      metadatum.fileContentLength,
    );
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

async function doPack(): Promise<void> {
  if (process.argv.length === 2) {
    usage("You must specify the output file.");
    process.exit(1);
  }

  const outfile = process.argv[3];
  const stream = fsSync.createWriteStream(outfile);
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
    const chunks: Buffer[] = [];
    for await (const chunk of process.stdin) {
      chunks.push(Buffer.from(chunk));
    }
    const input = Buffer.concat(chunks).toString("utf-8");
    for (const file of input.split("\n").filter(Boolean)) {
      n++;
      files[mustNormalizePath(file)] = file;
    }
  }

  if (n === 0) {
    usage("You must at least specify one file to pack");
    process.exit(1);
  }

  await pack(files, stream);
  stream.end();
}

async function doUnpack(): Promise<void> {
  if (process.argv.length === 2) {
    usage("You must specify the input file when unpacking.");
    process.exit(1);
  }

  const infile = process.argv[3];
  try {
    await fs.access(infile);
  } catch {
    console.error(`Error: File '${infile}' does not exist`);
    process.exit(1);
  }

  const fileContent = await fs.readFile(infile);
  const directory = process.argv.length !== 3 ? process.argv[4] : ".";
  await unpack(directory, fileContent);
}

// Main program
async function main(): Promise<void> {
  if (
    process.argv.length <= 2 ||
    !["pack", "unpack"].includes(process.argv[2])
  ) {
    usage("Please specify whether to pack or unpack");
    process.exit(1);
  }

  if (process.argv[2] === "pack") {
    await doPack();
  } else {
    await doUnpack();
  }
}

// Only run if this is the main module
if (require.main === module) {
  main();
}

// Export the functions for use as a module
export { pack, unpack, FileMap, FileMetadata };
