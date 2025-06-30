/**
 * Shared memory IPC manager
 * Uses Atomics API for asnchronization
 */

class IPCManager {
  private buffer: SharedArrayBuffer;
  private view: Int32Array;

  constructor() {
    //initialized by native code during startup
    this.buffer = new SharedArrayBuffer(4 * 1024 * 1024);
    this.view = new Int32Array(this.buffer);
  }

  withSharedBuffer<T>(op: (buffer: SharedArrayBuffer) => T): T {
    while (Atomics.compareExchange(this.view, 0, 0, 1) !== 0) {
      Atomics.wait(this.view, 0, 1);
    }

    try {
      return op(this.buffer);
    } finally {
      Atomics.store(this.view, 0, 0);
      Atomics.notify(this.view, 0, 1);
    }
  }

  async readDir(path: string): Promise<string[]> {
    return this.withSharedBuffer((buffer) => {
      const encoder = new TextEncoder();
      const encoded = encoder.encode(path);
      const view = new Uint8Array(buffer);

      view.set(encoded, 16);

      Atomics.store(this.view, 1, 1);
      Atomics.notify(this.view, 1);

      while (Atomics.load(this.view, 2) === 0) {
        Atomics.wait(this.view, 2, 0);
      }

      const decoder = new TextDecoder();
      return JSON.parse(decoder.decode(view.subarray(1024, 2048)));
    });
  }
}
