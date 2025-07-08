import { HTTPClient } from "./http";
import { WebSocketClient } from "./ws";
import { TopicManager } from "./topics";
import { CsrfManager } from "./secure/csrf";
import { CorsEnforcer } from "./secure/cors";
import { PermissionValidator } from "./secure/permissions";
import { SecureWebSocketClient } from "./secure/websocket";
import type {
  ReavixConfig,
  Response,
  EventCallback,
  EventDefinition,
  EventMap,
  TypedEventEmitter,
  Reavixrequest,
  ReavixResponse,
  SubscriptionOptions,
} from "./types";
import { debug } from "./debug";

export class ReavixClient<Events extends EventMap = EventMap>
  implements TypedEventEmitter<Events>
{
  private http: HTTPClient;
  private ws: WebSocketClient;
  private config: Required<ReavixConfig>;
  private topics: TopicManager;
  private csrfManager: CsrfManager;
  private corsEnforcer: CorsEnforcer;
  private permissionValidator: PermissionValidator;
  private debug = debug;

  constructor(config: ReavixConfig = {}) {
    if (config.debug) {
      this.debug.enable();
    }
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
      allowedOrigins: config.allowedOrigins || [],
    };
    this.http = new HTTPClient(this.config);

    this.csrfManager = new CsrfManager();
    this.corsEnforcer = new CorsEnforcer({
      allowedOrigins: config.allowedOrigins || [
        config.baseURL || window.location.origin,
      ],
      allowCredentials: true,
    });
    this.permissionValidator = new PermissionValidator();

    this.ws = new SecureWebSocketClient(this.config, this.csrfManager);
    this.topics = new TopicManager(this.ws);

    this.initializeSecurity();

    //Handle topic messages
    this.ws.on("topic", (message: { topic: string; data: unknown }) => {
      this.topics.handleMessage(message.topic, message.data);
    });
  }

  /**
   * Initializes security features for the Reavix client.
   *
   * This method performs asynchronous initialization of essential security
   * components, including CSRF protection and permission validation. It
   * concurrently initializes the CSRF manager and loads permissions from the
   * backend.
   *
   * @returns {Promise<void>} A Promise that resolves when all security
   * initializations are complete.
   */

  private async initializeSecurity(): Promise<void> {
    await Promise.all([
      this.csrfManager.initialize(this),
      this,
      this.permissionValidator.loadPermissions(this),
    ]);
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
   * Makes a request to the backend with security features enabled.
   *
   * This method performs a request to the given endpoint with the given options,
   * but also applies security features such as CSRF protection and permission
   * validation. If the request is not permitted according to the loaded
   * permissions, an error is thrown. Also, the response is validated for a new
   * CSRF token.
   *
   * @param {string} endpoint - The endpoint URL to send the request to.
   * @param {RequestOptions} [options] - Optional request configuration.
   * @returns {Promise<Response<T>>} A Promise that resolves with the response
   * data.
   */
  async request<Response, payload>(
    endpoint: string,
    options?: Reavixrequest<payload>
  ): Promise<ReavixResponse<Response>> {
    this.debug.log(`request:start`, options);
    // Check permissions
    const method = options?.method || "GET";
    if (!this.permissionValidator.isPermitted(method, endpoint)) {
      throw new Error(`Operation not permitted: ${method} ${endpoint}`);
    }

    // Apply security headers
    const headers = this.csrfManager.protectRequest(options?.headers || {});
    const secureHeaders = this.corsEnforcer.applyHeaders(
      headers,
      window.location.origin
    );

    const response = await this.http.request(endpoint, {
      ...options,
      headers: secureHeaders,
    });

    this.csrfManager.validateResponse(response);
    return response as ReavixResponse<Response>;
  }

  /**
   * Registers an event listener for a specified event name.
   *
   * @param {string} eventName - The name of the event to listen for.
   * @param {EventCallback<T>} callback - The callback function to invoke when the event is emitted.
   * @returns {() => void} A function that can be called to unsubscribe from the event.
   */

  on<Event extends keyof Events>(
    eventName: Event,
    callback: (payload: Events[Event]) => void,
    options?: SubscriptionOptions
  ): () => void {
    return this.ws.on(String(eventName), callback, options);
  }

  off(eventName: string, callback?: EventCallback): void {
    return this.ws.off(eventName, callback);
  }

  emit<Event extends keyof Events>(
    eventName: Event,
    payload: Events[Event]
  ): Promise<void> {
    return this.ws.emit(String(eventName), payload);
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

  /**
   * Enables debug logging.
   *
   * This will enable debug logging for all internal components of the Reavix
   * client. This can be useful for debugging purposes, but it is not recommended
   * to leave it enabled in production.
   */
  enableDebug(): void {
    this.debug.enable();
  }

  /**
   * Disables debug logging.
   *
   * This will disable debug logging for all internal components of the Reavix
   * client.
   */
  disableDebug(): void {
    this.debug.disable();
  }

  /**
   * Retrieves all debug events.
   *
   * This can be useful for debugging purposes, allowing you to access all
   * debug events that have been logged.
   *
   * @returns {DebugEvent[]} An array of all debug events.
   */
  getDebugEvents() {
    return this.debug.getEvents();
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

export type { ReavixConfig, Response, EventCallback, EventDefinition };
