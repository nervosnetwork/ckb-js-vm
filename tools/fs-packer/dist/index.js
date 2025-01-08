"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __generator = (this && this.__generator) || function (thisArg, body) {
    var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g = Object.create((typeof Iterator === "function" ? Iterator : Object).prototype);
    return g.next = verb(0), g["throw"] = verb(1), g["return"] = verb(2), typeof Symbol === "function" && (g[Symbol.iterator] = function() { return this; }), g;
    function verb(n) { return function (v) { return step([n, v]); }; }
    function step(op) {
        if (f) throw new TypeError("Generator is already executing.");
        while (g && (g = 0, op[0] && (_ = 0)), _) try {
            if (f = 1, y && (t = op[0] & 2 ? y["return"] : op[0] ? y["throw"] || ((t = y["return"]) && t.call(y), 0) : y.next) && !(t = t.call(y, op[1])).done) return t;
            if (y = 0, t) op = [op[0] & 2, t.value];
            switch (op[0]) {
                case 0: case 1: t = op; break;
                case 4: _.label++; return { value: op[1], done: false };
                case 5: _.label++; y = op[1]; op = [0]; continue;
                case 7: op = _.ops.pop(); _.trys.pop(); continue;
                default:
                    if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; }
                    if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; }
                    if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; }
                    if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; }
                    if (t[2]) _.ops.pop();
                    _.trys.pop(); continue;
            }
            op = body.call(thisArg, _);
        } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; }
        if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true };
    }
};
var __asyncValues = (this && this.__asyncValues) || function (o) {
    if (!Symbol.asyncIterator) throw new TypeError("Symbol.asyncIterator is not defined.");
    var m = o[Symbol.asyncIterator], i;
    return m ? m.call(o) : (o = typeof __values === "function" ? __values(o) : o[Symbol.iterator](), i = {}, verb("next"), verb("throw"), verb("return"), i[Symbol.asyncIterator] = function () { return this; }, i);
    function verb(n) { i[n] = o[n] && function (v) { return new Promise(function (resolve, reject) { v = o[n](v), settle(resolve, reject, v.done, v.value); }); }; }
    function settle(resolve, reject, d, v) { Promise.resolve(v).then(function(v) { resolve({ value: v, done: d }); }, reject); }
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.pack = pack;
exports.unpack = unpack;
var fs = require("node:fs/promises");
var fsSync = require("node:fs");
var path = require("path");
function getFileSize(filePath) {
    return __awaiter(this, void 0, void 0, function () {
        var stats;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0: return [4 /*yield*/, fs.stat(filePath)];
                case 1:
                    stats = _a.sent();
                    return [2 /*return*/, stats.size];
            }
        });
    });
}
function writeToFile(data, writeStream) {
    return __awaiter(this, void 0, void 0, function () {
        return __generator(this, function (_a) {
            return [2 /*return*/, new Promise(function (resolve, reject) {
                    var canContinue = writeStream.write(data);
                    if (canContinue) {
                        resolve();
                    }
                    else {
                        writeStream.once("drain", resolve);
                        writeStream.once("error", reject);
                    }
                })];
        });
    });
}
function appendFileToStream(filePath, stream) {
    return __awaiter(this, void 0, void 0, function () {
        var content;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0: return [4 /*yield*/, fs.readFile(filePath)];
                case 1:
                    content = _a.sent();
                    return [4 /*yield*/, writeToFile(content, stream)];
                case 2:
                    _a.sent();
                    return [4 /*yield*/, writeToFile(Buffer.from([0]), stream)];
                case 3:
                    _a.sent();
                    return [2 /*return*/];
            }
        });
    });
}
function appendStringNullToStream(str, stream) {
    return __awaiter(this, void 0, void 0, function () {
        var buffer;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0:
                    buffer = Buffer.from(str + "\0");
                    return [4 /*yield*/, writeToFile(buffer, stream)];
                case 1:
                    _a.sent();
                    return [2 /*return*/];
            }
        });
    });
}
function appendIntegerToStream(num, stream) {
    return __awaiter(this, void 0, void 0, function () {
        var buffer;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0:
                    buffer = Buffer.alloc(4);
                    buffer.writeInt32LE(num);
                    return [4 /*yield*/, writeToFile(buffer, stream)];
                case 1:
                    _a.sent();
                    return [2 /*return*/];
            }
        });
    });
}
function pack(files, outputStream) {
    return __awaiter(this, void 0, void 0, function () {
        var numFiles, offset, length, _i, _a, _b, name_1, filePath, _c, _d, _e, name_2, filePath;
        return __generator(this, function (_f) {
            switch (_f.label) {
                case 0:
                    numFiles = Object.keys(files).length;
                    return [4 /*yield*/, appendIntegerToStream(numFiles, outputStream)];
                case 1:
                    _f.sent();
                    offset = 0;
                    length = 0;
                    _i = 0, _a = Object.entries(files);
                    _f.label = 2;
                case 2:
                    if (!(_i < _a.length)) return [3 /*break*/, 9];
                    _b = _a[_i], name_1 = _b[0], filePath = _b[1];
                    console.log("packing file ".concat(filePath, " to ").concat(name_1));
                    return [4 /*yield*/, appendIntegerToStream(offset, outputStream)];
                case 3:
                    _f.sent();
                    length = Buffer.byteLength(name_1);
                    return [4 /*yield*/, appendIntegerToStream(length, outputStream)];
                case 4:
                    _f.sent();
                    offset += length;
                    offset += 1; // add trailing zero
                    return [4 /*yield*/, appendIntegerToStream(offset, outputStream)];
                case 5:
                    _f.sent();
                    return [4 /*yield*/, getFileSize(filePath)];
                case 6:
                    length = _f.sent();
                    return [4 /*yield*/, appendIntegerToStream(length, outputStream)];
                case 7:
                    _f.sent();
                    offset += length;
                    offset += 1; // add trailing zero
                    _f.label = 8;
                case 8:
                    _i++;
                    return [3 /*break*/, 2];
                case 9:
                    _c = 0, _d = Object.entries(files);
                    _f.label = 10;
                case 10:
                    if (!(_c < _d.length)) return [3 /*break*/, 14];
                    _e = _d[_c], name_2 = _e[0], filePath = _e[1];
                    return [4 /*yield*/, appendStringNullToStream(name_2, outputStream)];
                case 11:
                    _f.sent();
                    return [4 /*yield*/, appendFileToStream(filePath, outputStream)];
                case 12:
                    _f.sent();
                    _f.label = 13;
                case 13:
                    _c++;
                    return [3 /*break*/, 10];
                case 14: return [2 /*return*/];
            }
        });
    });
}
function createDirectory(dir) {
    return __awaiter(this, void 0, void 0, function () {
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0: return [4 /*yield*/, fs.mkdir(dir, { recursive: true })];
                case 1:
                    _a.sent();
                    return [2 /*return*/];
            }
        });
    });
}
function unpack(directory, fileContent) {
    return __awaiter(this, void 0, void 0, function () {
        function readInteger() {
            var value = fileContent.readInt32LE(position);
            position += 4;
            return value;
        }
        function readStringNull(length) {
            var value = fileContent
                .toString("utf8", position, position + length)
                .replace(/\0$/, "");
            return value;
        }
        function copyToFile(directory, filename, offset, length) {
            return __awaiter(this, void 0, void 0, function () {
                var normalizedFilename, filePath, dir, content;
                return __generator(this, function (_a) {
                    switch (_a.label) {
                        case 0:
                            normalizedFilename = filename.replace(/\//g, path.sep);
                            filePath = path.join(directory, normalizedFilename);
                            dir = path.dirname(filePath);
                            return [4 /*yield*/, createDirectory(dir)];
                        case 1:
                            _a.sent();
                            console.log("unpacking file ".concat(filename, " to ").concat(filePath));
                            content = fileContent.slice(offset, offset + length);
                            return [4 /*yield*/, fs.writeFile(filePath, content)];
                        case 2:
                            _a.sent();
                            return [2 /*return*/];
                    }
                });
            });
        }
        var position, numFiles, metadata, i, blobStart, _i, metadata_1, metadatum, filename;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0:
                    position = 0;
                    numFiles = readInteger();
                    metadata = [];
                    // Read metadata
                    for (i = 0; i < numFiles; i++) {
                        metadata.push({
                            fileNameOffset: readInteger(),
                            fileNameLength: readInteger(),
                            fileContentOffset: readInteger(),
                            fileContentLength: readInteger(),
                        });
                    }
                    blobStart = position;
                    _i = 0, metadata_1 = metadata;
                    _a.label = 1;
                case 1:
                    if (!(_i < metadata_1.length)) return [3 /*break*/, 4];
                    metadatum = metadata_1[_i];
                    position = blobStart + metadatum.fileNameOffset;
                    filename = readStringNull(metadatum.fileNameLength);
                    position = blobStart + metadatum.fileContentOffset;
                    return [4 /*yield*/, copyToFile(directory, filename, position, metadatum.fileContentLength)];
                case 2:
                    _a.sent();
                    _a.label = 3;
                case 3:
                    _i++;
                    return [3 /*break*/, 1];
                case 4: return [2 /*return*/];
            }
        });
    });
}
function isIdealPath(path) {
    return (!path.startsWith("/") && !path.startsWith("..") && !/^[a-zA-Z]:/.test(path));
}
function normalizeRelativePath(path) {
    return path.replace(/^\.\//, "").replace(/\/\.\//g, "/");
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
    return __awaiter(this, void 0, void 0, function () {
        var outfile, stream, files, n, i, file, chunks, _a, _b, _c, chunk, e_1_1, input, _i, _d, file;
        var _e, e_1, _f, _g;
        return __generator(this, function (_h) {
            switch (_h.label) {
                case 0:
                    if (process.argv.length === 2) {
                        usage("You must specify the output file.");
                        process.exit(1);
                    }
                    outfile = process.argv[3];
                    stream = fsSync.createWriteStream(outfile);
                    files = {};
                    n = 0;
                    if (!(process.argv.length !== 3)) return [3 /*break*/, 1];
                    // Read files from command line arguments
                    for (i = 4; i < process.argv.length; i++) {
                        n++;
                        file = process.argv[i];
                        files[mustNormalizePath(file)] = file;
                    }
                    return [3 /*break*/, 14];
                case 1:
                    chunks = [];
                    _h.label = 2;
                case 2:
                    _h.trys.push([2, 7, 8, 13]);
                    _a = true, _b = __asyncValues(process.stdin);
                    _h.label = 3;
                case 3: return [4 /*yield*/, _b.next()];
                case 4:
                    if (!(_c = _h.sent(), _e = _c.done, !_e)) return [3 /*break*/, 6];
                    _g = _c.value;
                    _a = false;
                    chunk = _g;
                    chunks.push(Buffer.from(chunk));
                    _h.label = 5;
                case 5:
                    _a = true;
                    return [3 /*break*/, 3];
                case 6: return [3 /*break*/, 13];
                case 7:
                    e_1_1 = _h.sent();
                    e_1 = { error: e_1_1 };
                    return [3 /*break*/, 13];
                case 8:
                    _h.trys.push([8, , 11, 12]);
                    if (!(!_a && !_e && (_f = _b.return))) return [3 /*break*/, 10];
                    return [4 /*yield*/, _f.call(_b)];
                case 9:
                    _h.sent();
                    _h.label = 10;
                case 10: return [3 /*break*/, 12];
                case 11:
                    if (e_1) throw e_1.error;
                    return [7 /*endfinally*/];
                case 12: return [7 /*endfinally*/];
                case 13:
                    input = Buffer.concat(chunks).toString("utf-8");
                    for (_i = 0, _d = input.split("\n").filter(Boolean); _i < _d.length; _i++) {
                        file = _d[_i];
                        n++;
                        files[mustNormalizePath(file)] = file;
                    }
                    _h.label = 14;
                case 14:
                    if (n === 0) {
                        usage("You must at least specify one file to pack");
                        process.exit(1);
                    }
                    return [4 /*yield*/, pack(files, stream)];
                case 15:
                    _h.sent();
                    stream.end();
                    return [2 /*return*/];
            }
        });
    });
}
function doUnpack() {
    return __awaiter(this, void 0, void 0, function () {
        var infile, _a, fileContent, directory;
        return __generator(this, function (_b) {
            switch (_b.label) {
                case 0:
                    if (process.argv.length === 2) {
                        usage("You must specify the input file when unpacking.");
                        process.exit(1);
                    }
                    infile = process.argv[3];
                    _b.label = 1;
                case 1:
                    _b.trys.push([1, 3, , 4]);
                    return [4 /*yield*/, fs.access(infile)];
                case 2:
                    _b.sent();
                    return [3 /*break*/, 4];
                case 3:
                    _a = _b.sent();
                    console.error("Error: File '".concat(infile, "' does not exist"));
                    process.exit(1);
                    return [3 /*break*/, 4];
                case 4: return [4 /*yield*/, fs.readFile(infile)];
                case 5:
                    fileContent = _b.sent();
                    directory = process.argv.length !== 3 ? process.argv[4] : ".";
                    return [4 /*yield*/, unpack(directory, fileContent)];
                case 6:
                    _b.sent();
                    return [2 /*return*/];
            }
        });
    });
}
// Main program
function main() {
    return __awaiter(this, void 0, void 0, function () {
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0:
                    if (process.argv.length <= 2 ||
                        !["pack", "unpack"].includes(process.argv[2])) {
                        usage("Please specify whether to pack or unpack");
                        process.exit(1);
                    }
                    if (!(process.argv[2] === "pack")) return [3 /*break*/, 2];
                    return [4 /*yield*/, doPack()];
                case 1:
                    _a.sent();
                    return [3 /*break*/, 4];
                case 2: return [4 /*yield*/, doUnpack()];
                case 3:
                    _a.sent();
                    _a.label = 4;
                case 4: return [2 /*return*/];
            }
        });
    });
}
// Only run if this is the main module
if (require.main === module) {
    main();
}
