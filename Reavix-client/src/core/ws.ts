import type { ReavixConfig, EventCallback } from "./types";

type Subscription<T = unknown> = {
  eventName: string;
  callback: EventCallback<T>;
};


export class WebSocketClient {
  private config: Required<ReavixConfig>;
  private socket: WebSocket | null = null;
  private subscriptions: Subscription<unknown>[] = [];
  private reconnectAttempts = 0;
  private reconnectTimeout: number | null = null;
  private heartbeatInterval: number | null = null;
  private pendingMessages: Array<{
    data: unknown;
    resolve: () => void;
    reject: (err: Error) => void;
  }> = [];

  constructor(config: ReavixConfig) {
    this.config = {
      baseURL: config.baseURL || "",
      wsURL: config.wsURL || this.getWsUrl(config.baseURL),
      debug: config.debug || false,
      headers: config.headers || {},
      timeout: config.timeout || 10000,
      reconnect: {
        enabled: config.reconnect?.enabled ?? true,
        delay: config.reconnect?.delay ?? 1000,
        maxAttempts: config.reconnect?.maxAttempts ?? 5,
        backoffFactor: config.reconnect?.backoffFactor ?? 2,
      },
    };

    if (this.config.wsURL) {
      this.connect();
    }
  }

  /**
   * Converts an HTTP URL to a WebSocket URL.
   *
   * @param {string} [baseUrl] - The HTTP URL to convert.
   * @returns {string} The WebSocket URL.
   *
   * @example
   * getWsUrl('http://example.com') // 'ws://example.com'
   */
  private getWsUrl(baseUrl?: string): string {
    if (!baseUrl) return "";

    return baseUrl.replace(/^http/, "ws");
  }

  private connect(): void {
    if (this.socket) {
      this.disconnect();
    }

    try {
      this.socket = new WebSocket(this.config.wsURL);

      this.socket.onopen = () => {
        this.reconnectAttempts = 0;
        if (this.reconnectTimeout) {
          clearTimeout(this.reconnectTimeout);
          this.reconnectTimeout = null;
        }

        this.startHeartbeat();
        this.flushPendingMessages();
        if (this.config.debug) {
          console.debug(`[REAVIX] WebSocket connected`);
        }
      };

      this.socket.onmessage = (event) => {
        try {
          const { event: eventName, data } = JSON.parse(event.data);
          this.subscriptions
            .filter((sub) => sub.eventName === eventName)
            .forEach((sub) => sub.callback(data));
        } catch (err) {
          console.error(`[REAVIX] Failed to parse WebSocket message:`, err);
        }
      };

      this.socket.onclose = () => {
        this.stopHeartbeat();
        if (
          this.config.reconnect.enabled &&
          this.reconnectAttempts < (this.config.reconnect.maxAttempts || 5)
        ) {
          this.scheduleReconnect();
        }
      };

      this.socket.onerror = (error) => {
        console.error(`[REAVIX] WebSocket error:`, error);
      };
    } catch (error) {
      console.error(`[REAVIX] WebSocket initializetion error:`, error);
    }
  }

  /**
   * Schedules a reconnect to the WebSocket server using an exponential backoff
   * strategy to avoid overwhelming the server.
   *
   * @private
   */
  private scheduleReconnect(): void {
    if (
      this.reconnectTimeout ||
      !this.config.reconnect.delay ||
      !this.config.reconnect.backoffFactor
    ) {
      return;
    }

    const maxDelay = 30000;
    const calculatedDelay =
      this.config.reconnect.delay *
      Math.pow(this.config.reconnect.backoffFactor, this.reconnectAttempts);
    const delay = Math.min(calculatedDelay, maxDelay);

    this.reconnectAttempts++;
    this.reconnectTimeout = window.setTimeout(() => {
      this.connect();
    }, delay);
  }

  /**
   * Initiates a heartbeat mechanism to keep the WebSocket connection alive.
   *
   * Sends a "heartbeat" event with the current timestamp every 30 seconds
   * if the WebSocket connection is open. The interval is cleared when the
   * connection is closed or a new connection is established.
   *
   * @private
   */

  private startHeartbeat(): void {
    if (this.heartbeatInterval) return;

    const heartbeatEvent = JSON.stringify({
      event: "heartbeat",
      data: Date.now(),
    });

    this.heartbeatInterval = window.setInterval(() => {
      if (this.socket?.readyState === WebSocket.OPEN) {
        this.socket.send(heartbeatEvent);
      }
    }, 30000); // 30 seconds
  }

  /**
   * Clears the heartbeat interval.
   *
   * Called when the WebSocket connection is closed, as there is no need to
   * send a heartbeat event anymore.
   *
   * @private
   */
  private stopHeartbeat(): void {
    if (this.heartbeatInterval) {
      clearInterval(this.heartbeatInterval);
      this.heartbeatInterval = null;
    }
  }

  /**
   * Sends all queued messages over the WebSocket connection.
   *
   * If the WebSocket connection is open, this method iterates over the
   * pending messages queue and attempts to send each message. For each
   * message, it calls the `resolve` function if the message is sent
   * successfully, or the `reject` function with an error if the
   * sending fails.
   *
   * This method should be called when the WebSocket connection is
   * established to ensure all previously queued messages are delivered.
   *
   * @private
   */

  private flushPendingMessages(): void {
    while (
      this.pendingMessages.length > 0 &&
      this.socket?.readyState === WebSocket.OPEN
    ) {
      const { data, resolve, reject } = this.pendingMessages.shift()!;
      try {
        this.socket.send(JSON.stringify(data));
        resolve();
      } catch (error) {
        reject(error as Error);
      }
    }
  }

  //This needs attention, not good for now
  on<T = unknown>(eventName: string, callback: EventCallback<T>): () => void {
    const subscription: Subscription<unknown> = {
      eventName,
      callback: callback as EventCallback<unknown>, // cast is safe if you're in control
    };

    this.subscriptions.push(subscription);

    return () => {
      this.subscriptions = this.subscriptions.filter(
        (sub) => sub !== subscription
      );
    };
  }

  /**
   * Sends a message to the WebSocket server.
   *
   * If the WebSocket connection is open, this method sends the message
   * immediately. If the connection is not open and reconnect is enabled,
   * the message is queued until the connection is re-established.
   *
   * @param eventName - The name of the event to emit.
   * @param payload - The data to send with the event.
   * @returns A Promise that resolves if the message is sent successfully,
   * or rejects with an error if the sending fails.
   */
  emit(eventName: string, payload: unknown): Promise<void> {
    return new Promise((resolve, reject) => {
      const message = { event: eventName, data: payload };

      if (this.socket?.readyState === WebSocket.OPEN) {
        try {
          this.socket.send(JSON.stringify(message));
          resolve();
        } catch (error) {
          reject(error);
        }
      } else {
        if (this.config.reconnect.enabled) {
          this.pendingMessages.push({ data: message, resolve, reject });
        } else {
          reject(new Error("WebSocket is not connected"));
        }
      }
    });
  }

  /**
   * Disconnects the WebSocket client.
   *
   * Clears any pending reconnect timeouts, stops the heartbeat interval,
   * removes all event listeners from the WebSocket object, and closes the
   * connection if it is open. Sets the `socket` property to `null`.
   *
   * This method is useful when the application needs to disconnect from the
   * WebSocket server programmatically.
   */
  disconnect(): void {
    if (this.reconnectTimeout) {
      clearTimeout(this.reconnectTimeout);
      this.reconnectTimeout;
    }

    this.stopHeartbeat();

    if (this.socket) {
      this.socket.onopen = null;
      this.socket.onmessage = null;
      this.socket.onclose = null;
      this.socket.onerror = null;
    }

    if (this.socket?.readyState === WebSocket.OPEN) {
      this.socket.close();
    }

    this.socket = null;
  }

  /**
   * Updates the internal configuration with the given options.
   *
   * Any `reconnect` options will be merged with the existing `reconnect` config.
   *
   * If a new `baseURL` is provided and no `wsURL` is specified, the `wsURL` will
   * be updated to be the same as the `baseURL` but with the `ws` or `wss` scheme.
   *
   * If the `wsURL` changes, the WebSocket client will be reconnected.
   *
   * @param {Partial<ReavixConfig>} newConfig - The new configuration options.
   */
  updateConfig(newConfig: Partial<ReavixConfig>): void {
    const oldWsURL = this.config.wsURL;
    this.config = {
      ...this.config,
      ...newConfig,
      reconnect: {
        ...this.config.reconnect,
        ...newConfig.reconnect,
      },
    };

    if (newConfig.baseURL && !newConfig.wsURL) {
      this.config.wsURL = this.getWsUrl(newConfig.baseURL);
    }

    //Reconnect if URL changed
    if (this.config.wsURL !== oldWsURL && this.config.wsURL) {
      this.connect();
    }
  }
}
