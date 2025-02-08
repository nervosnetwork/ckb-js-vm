import { sprintf, printf } from "@ckb-js-std/bindings";

export enum LogLevel {
  debug = 1,
  info = 2,
  warn = 3,
  error = 4,
}

let CURRENT_LEVEL = LogLevel.info;

export function debug(format: string, ...args: any[]) {
  if (CURRENT_LEVEL <= LogLevel.debug) {
    let log = sprintf(format, ...args);
    printf("[DEBUG] %s", log);
  }
}

export function info(format: string, ...args: any[]) {
  if (CURRENT_LEVEL <= LogLevel.info) {
    let log = sprintf(format, ...args);
    printf("[INFO] %s", log);
  }
}

export function warn(format: string, ...args: any[]) {
  if (CURRENT_LEVEL <= LogLevel.warn) {
    let log = sprintf(format, ...args);
    printf("[WARN] %s", log);
  }
}

export function error(format: string, ...args: any[]) {
  if (CURRENT_LEVEL <= LogLevel.error) {
    let log = sprintf(format, ...args);
    printf("[ERROR] %s", log);
  }
}

export function setLevel(level: LogLevel) {
  CURRENT_LEVEL = level;
}
