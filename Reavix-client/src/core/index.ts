import { HTTPClient } from "./http";
import { WebSocketClient } from "./ws";
import { TopicManager } from "./topics";
import type {
  ReavixConfig,
  RequestOptions,
  Response,
  EventCallback,
  EventDefinition,
} from "./types";

export class ReavixClient {
  private http: HTTPClient;
  private ws: WebSocketClient;
  private config: Required<ReavixConfig>;
  private topics: TopicManager;

  constructor(config: ReavixConfig = {}) {
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
    this.http = new HTTPClient(this.config);
    this.ws = new WebSocketClient(this.config);
    this.topics = new TopicManager(this.ws);

    //Handle topic messages
    this.ws.on("topic", (message: { topic: string; data: unknown }) => {
      this.topics.handleMessage(message.topic, message.data);
    });
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

  /**
   * Send a request to the HTTP server.
   *
   * @param {string} endpoint - The path to the endpoint (e.g. "/users/:id").
   * @param {RequestOptions} [options] - The request options.
   * @returns {Promise<Response<T>>} The response.
   */
  request<T = unknown>(
    endpoint: string,
    options?: RequestOptions
  ): Promise<Response<T>> {
    return this.http.request(endpoint, options);
  }

  /**
   * Registers an event listener for a specified event name.
   *
   * @param {string} eventName - The name of the event to listen for.
   * @param {EventCallback<T>} callback - The callback function to invoke when the event is emitted.
   * @returns {() => void} A function that can be called to unsubscribe from the event.
   */

  on<T = unknown>(eventName: string, callback: EventCallback<T>): () => void {
    return this.ws.on(eventName, callback);
  }

  /**
   * Emits a specified event to the WebSocket server with an associated payload.
   *
   * If the WebSocket connection is open, this method sends the event immediately.
   * If the connection is not open and reconnect is enabled, the event is queued
   * until the connection is re-established.
   *
   * @param {string} eventName - The name of the event to emit.
   * @param {unknown} payload - The data to send with the event.
   * @returns {Promise<void>} A Promise that resolves if the message is sent successfully,
   * or rejects with an error if the sending fails.
   */

  emit(eventName: string, payload: unknown): Promise<void> {
    return this.ws.emit(eventName, payload);
  }

  /**
   * Updates the internal configuration with the given options.
   *
   * Any `reconnect` options will be merged with the existing `reconnect` config.
   *
   * If a new `baseURL` is provided and no `wsURL` is specified, the `wsURL` will
   * be updated to be the same as the `baseURL` but with the `ws` or `wss` scheme.
   *
   * @param {ReavixConfig} newConfig - The new configuration options.
   */
  updateConfig(newConfig: ReavixConfig): void {
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

    this.http.updateConfig(this.config);
    this.ws.updateConfig(this.config);
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
    this.ws.disconnect();
  }

  /**
   * Registers multiple event definitions.
   *
   * @param {EventDefinition[]} definitions - An array of event definitions to register.
   */
  registerEvents(definitions: EventDefinition[]): void {
    this.ws.registerEvents(definitions);
  }

  /**
   * Subscribes to a topic.
   *
   * If the topic has not been subscribed to yet, this will send a subscription
   * request to the server.
   *
   * @param {string} topic - The topic to subscribe to
   * @param {(payload: T) => void} callback - The callback to call when a message is received
   * @returns {() => void} An unsubscribe function
   */
  subscribe<T = unknown>(
    topic: string,
    callback: (payload: T) => void
  ): () => void {
    return this.topics.subscribe(topic, callback);
  }

  /**
   * Unsubscribes from a topic.
   *
   * If a callback is provided, only the specific callback will be unsubscribed,
   * otherwise all callbacks for the topic will be unsubscribed.
   *
   * @param {string} topic - The topic to unsubscribe from
   * @param {Function} [callback] - The callback to unsubscribe, if not provided all will be unsubscribed
   */
  unsubscribe(topic: string, callback?: Function): void {
    this.topics.unsubscribe(topic, callback);
  }
}
//Global instance
let globalInstance: ReavixClient | null = null;

export function getReavixClient(config?: ReavixConfig): ReavixClient {
  if (!globalInstance) {
    if (!config?.baseURL && !config?.wsURL) {
      throw new Error(
        `Initial configuration requires eitheer baseURL or wsURL`
      );
    }
    globalInstance = new ReavixClient(config);
  } else if (config) {
    globalInstance.updateConfig(config);
  }
  return globalInstance;
}

export type {
  ReavixConfig,
  RequestOptions,
  Response,
  EventCallback,
  EventDefinition,
};
