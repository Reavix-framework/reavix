/**
 * Reavix Core FFI Types
 * enforces type-safe IPC Communication
 *
 *
 */

declare module "reavix" {
  interface FileSystem {
    /**
     * Read directory contents
     * @param path Absolute filesystem path
     * @returns Array of filenames
     * @throws {ReavixError} on permission or system errors
     */
    readDir(path: string): Promise<string[]>;
  }

  interface IPC {
    /**
     * Low level shared memory access
     * @param callback Worker thread-safe function
     */
    withSharedBuffer<T>(callback: (buffer: SharedArrayBuffer) => T): T;
  }

  export const fs: FileSystem;
  export const ipc: IPC;
  export const version: string;

  export class ReavixError extends Error {
    code: number;
    syscall: string;
  }
}
