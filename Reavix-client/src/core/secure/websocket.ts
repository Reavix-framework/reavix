import { WebSocketClient } from "../ws";
import type { ReavixConfig } from "../types";
import { CsrfManager } from "./csrf";

/**
 * Secure WebSocket client wrapper
 */
export class SecureWebSocketClient extends WebSocketClient {
  private csrfManager: CsrfManager;

  constructor(config: ReavixConfig, csrfManager: CsrfManager) {
    super(config);
    this.csrfManager = csrfManager;
  }

  /**
   * Emits an event with the given name and data.
   *
   * This method adds CSRF protection to sensitive events before
   * delegating the emission to the superclass. It throws an error
   * if the event name starts with the reserved "system." namespace.
   *
   * @param eventName - The name of the event to emit.
   * @param data - The data associated with the event.
   * @throws {Error} If the event name uses a reserved namespace.
   */

  async emit(eventName: string, data: unknown): Promise<void> {
    if (eventName.startsWith("system.")) {
      throw new Error("Reserved event namespace");
    }

    // Add CSRF token to sensitive events
    const enhancedData = this.applySecurity(eventName, data);
    return super.emit(eventName, enhancedData);
  }

  /**
   * Applies security measures to the given event data.
   *
   * This method checks if the event name contains any of the
   * reserved keywords for sensitive events and, if so, adds
   * the CSRF token to the event data object. Otherwise, it
   * returns the original data as is.
   *
   * @param eventName - The name of the event to check.
   * @param data - The event data to secure.
   * @returns The secured event data.
   */
  private applySecurity(eventName: string, data: unknown): unknown {
    const sensitiveEvents = ["update", "delete", "admin"];
    const isSensitive = sensitiveEvents.some((key) => eventName.includes(key));

    if (isSensitive) {
      return {
        ...(typeof data === "object" ? data : { value: data }),
        _csrf: this.csrfManager.getToken(),
      };
    }

    return data;
  }
}
