import { sprintf, printf } from "@ckb-js-std/bindings";

export enum LogLevel {
  Debug = 1,
  Info = 2,
  Warn = 3,
  Error = 4,
}

let CURRENT_LEVEL = LogLevel.Info;

/**
 * Logs a debug message if the current log level is set to debug or lower
 * @param format - The format string for the log message
 * @param args - Arguments to be formatted into the message
 * @example
 * debug("Processing item %d", 123);
 */
export function debug(format: string, ...args: any[]) {
  if (CURRENT_LEVEL <= LogLevel.Debug) {
    let log = sprintf(format, ...args);
    printf("[DEBUG] %s", log);
  }
}

/**
 * Logs an info message if the current log level is set to info or lower
 * @param format - The format string for the log message
 * @param args - Arguments to be formatted into the message
 * @example
 * info("Operation completed with %d items", count);
 */
export function info(format: string, ...args: any[]) {
  if (CURRENT_LEVEL <= LogLevel.Info) {
    let log = sprintf(format, ...args);
    printf("[INFO] %s", log);
  }
}

/**
 * Logs a warning message if the current log level is set to warn or lower
 * @param format - The format string for the log message
 * @param args - Arguments to be formatted into the message
 * @example
 * warn("Deprecated feature used: %s", featureName);
 */
export function warn(format: string, ...args: any[]) {
  if (CURRENT_LEVEL <= LogLevel.Warn) {
    let log = sprintf(format, ...args);
    printf("[WARN] %s", log);
  }
}

/**
 * Logs an error message if the current log level is set to error or lower
 * @param format - The format string for the log message
 * @param args - Arguments to be formatted into the message
 * @example
 * error("Failed to process: %s", errorMessage);
 */
export function error(format: string, ...args: any[]) {
  if (CURRENT_LEVEL <= LogLevel.Error) {
    let log = sprintf(format, ...args);
    printf("[ERROR] %s", log);
  }
}

/**
 * Sets the minimum log level for the logger
 * @param level - The minimum log level to display (debug, info, warn, or error)
 * @example
 * setLevel(LogLevel.debug);
 */
export function setLevel(level: LogLevel) {
  CURRENT_LEVEL = level;
}
