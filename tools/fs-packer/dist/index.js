"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.pack = pack;
exports.unpack = unpack;
var fs = require("fs");
var path = require("path");
function getFileSize(filePath) {
    var stats = fs.statSync(filePath);
    return stats.size;
}
function appendToStream(data, stream) {
    stream.write(data);
}
function appendFileToStream(filePath, stream) {
    var content = fs.readFileSync(filePath);
    appendToStream(content, stream);
}
function appendStringNullToStream(str, stream) {
    var buffer = Buffer.from(str + '\0');
    appendToStream(buffer, stream);
}
function appendIntegerToStream(num, stream) {
    var buffer = Buffer.alloc(4);
    buffer.writeInt32LE(num);
    appendToStream(buffer, stream);
}
function pack(files, outputStream) {
    var numFiles = Object.keys(files).length;
    appendIntegerToStream(numFiles, outputStream);
    var offset = 0;
    var length = 0;
    // Write metadata
    for (var _i = 0, _a = Object.entries(files); _i < _a.length; _i++) {
        var _b = _a[_i], name_1 = _b[0], filePath = _b[1];
        console.log("packing file ".concat(filePath, " to ").concat(name_1));
        appendIntegerToStream(offset, outputStream);
        length = Buffer.byteLength(name_1) + 1; // include null terminator
        appendIntegerToStream(length, outputStream);
        offset += length;
        appendIntegerToStream(offset, outputStream);
        length = getFileSize(filePath);
        appendIntegerToStream(length, outputStream);
        offset += length;
    }
    // Write actual file data
    for (var _c = 0, _d = Object.entries(files); _c < _d.length; _c++) {
        var _e = _d[_c], name_2 = _e[0], filePath = _e[1];
        appendStringNullToStream(name_2, outputStream);
        appendFileToStream(filePath, outputStream);
    }
}
function createDirectory(dir) {
    fs.mkdirSync(dir, { recursive: true });
}
function unpack(directory, fileContent) {
    var position = 0;
    function readInteger() {
        var value = fileContent.readInt32LE(position);
        position += 4;
        return value;
    }
    function readStringNull(length) {
        var value = fileContent.toString('utf8', position, position + length).replace(/\0$/, '');
        position += length;
        return value;
    }
    function copyToFile(directory, filename, offset, length) {
        var normalizedFilename = filename.replace(/\//g, path.sep);
        var filePath = path.join(directory, normalizedFilename);
        var dir = path.dirname(filePath);
        createDirectory(dir);
        console.log("unpacking file ".concat(filename, " to ").concat(filePath));
        var content = fileContent.slice(offset, offset + length);
        fs.writeFileSync(filePath, content);
    }
    var numFiles = readInteger();
    var metadata = [];
    // Read metadata
    for (var i = 0; i < numFiles; i++) {
        metadata.push({
            fileNameOffset: readInteger(),
            fileNameLength: readInteger(),
            fileContentOffset: readInteger(),
            fileContentLength: readInteger()
        });
    }
    var blobStart = position;
    for (var _i = 0, metadata_1 = metadata; _i < metadata_1.length; _i++) {
        var metadatum = metadata_1[_i];
        position = blobStart + metadatum.fileNameOffset;
        var filename = readStringNull(metadatum.fileNameLength);
        position = blobStart + metadatum.fileContentOffset;
        copyToFile(directory, filename, position, metadatum.fileContentLength);
    }
}
function isIdealPath(path) {
    return !path.startsWith('/') &&
        !path.startsWith('..') &&
        !/^[a-zA-Z]:/.test(path);
}
function normalizeRelativePath(path) {
    return path.replace(/^\.\//, '')
        .replace(/\/\.\//g, '/');
}
function mustNormalizePath(path) {
    if (!isIdealPath(path)) {
        throw new Error("Either not a relative path or a relative path referring to ..: ".concat(path));
    }
    return normalizeRelativePath(path);
}
// Add CLI functionality
function usage(msg) {
    if (msg)
        console.log(msg);
    console.log("".concat(process.argv[1], " pack output_file [files] | ").concat(process.argv[1], " unpack input_file [directory]"));
}
function doPack() {
    if (process.argv.length === 2) {
        usage('You must specify the output file.');
        process.exit(1);
    }
    var outfile = process.argv[3];
    var stream = fs.createWriteStream(outfile);
    var files = {};
    var n = 0;
    if (process.argv.length !== 3) {
        // Read files from command line arguments
        for (var i = 4; i < process.argv.length; i++) {
            n++;
            var file = process.argv[i];
            files[mustNormalizePath(file)] = file;
        }
    }
    else {
        // Read files from stdin
        var input = fs.readFileSync(0, 'utf-8'); // 0 is stdin
        for (var _i = 0, _a = input.split('\n').filter(Boolean); _i < _a.length; _i++) {
            var file = _a[_i];
            n++;
            files[mustNormalizePath(file)] = file;
        }
    }
    if (n === 0) {
        usage('You must at least specify one file to pack');
        process.exit(1);
    }
    pack(files, stream);
    stream.end();
}
function doUnpack() {
    if (process.argv.length === 2) {
        usage('You must specify the input file when unpacking.');
        process.exit(1);
    }
    var infile = process.argv[3];
    if (!fs.existsSync(infile)) {
        console.error("Error: File '".concat(infile, "' does not exist"));
        process.exit(1);
    }
    var fileContent = fs.readFileSync(infile);
    var directory = process.argv.length !== 3 ? process.argv[4] : '.';
    unpack(directory, fileContent);
}
// Main program
function main() {
    if (process.argv.length <= 2 || !['pack', 'unpack'].includes(process.argv[2])) {
        usage('Please specify whether to pack or unpack');
        process.exit(1);
    }
    if (process.argv[2] === 'pack') {
        doPack();
    }
    else {
        doUnpack();
    }
}
// Only run if this is the main module
if (require.main === module) {
    main();
}
