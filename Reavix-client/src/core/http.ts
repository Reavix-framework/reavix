import type { ReavixConfig, RequestOptions, Response } from "./types";

/**
 * HTTP client for Reavix backend communication
 */

export class HTTPClient {
  private config: Required<ReavixConfig>;
  private abortControllers: Map<string, AbortController> = new Map();

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
  }

  /**
   * Convert an HTTP URL to a WebSocket URL
   * @param baseUrl HTTP URL to convert
   * @returns WebSocket URL
   */
  private getWsUrl(baseUrl?: string): string {
    if (!baseUrl) return "";

    return baseUrl.replace(/^http/, "ws");
  }

  /**
   * Sends an HTTP request to the specified endpoint with the given options.
   *
   * @template T - The expected response data type.
   * @param {string} endpoint - The endpoint to send the request to, relative to the baseURL.
   * @param {RequestOptions} [options={}] - Optional configurations for the request including method, headers, params, body, and timeout.
   * @returns {Promise<Response<T>>} - A promise that resolves to a Response object containing the status, data, and headers.
   * @throws {Error} - Throws an error if the request fails due to network issues, timeout, or an HTTP error status.
   */

  async request<T = unknown>(
    endpoint: string,
    options: RequestOptions = {}
  ): Promise<Response<T>> {
    const requestId = `${endpoint}-${Date.now()}`;
    const abortController = new AbortController();
    this.abortControllers.set(requestId, abortController);

    try {
      const url = new URL(endpoint, this.config.baseURL);
      if (options.params) {
        Object.entries(options.params).forEach(([key, value]) => {
          url.searchParams.append(key, String(value));
        });
      }

      const headers = new Headers({
        ...this.config.headers,
        ...options.headers,
        "Content-Type": "application/json",
      });

      const requestTimeout = options.timeout || this.config.timeout;
      const timeoutPromise = new Promise<never>((_, reject) =>
        setTimeout(
          () =>
            reject(new Error(`Request timed out after ${requestTimeout}ms`)),
          requestTimeout
        )
      );

      const fetchPromise = fetch(url.toString(), {
        method: options.method || "GET",
        headers,
        body: options.body ? JSON.stringify(options.body) : undefined,
        signal: abortController.signal,
      });

      const response = await Promise.race([fetchPromise, timeoutPromise]);

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const responseHeaders: Record<string, string> = {};
      response.headers.forEach((value, key) => {
        responseHeaders[key] = value;
      });

      return {
        status: response.status,
        data: await response.json(),
        headers: responseHeaders,
      };
    } finally {
      this.abortControllers.delete(requestId);
    }
  }

  /**
   * Aborts all requests matching the given endpoint and removes them from the
   * internal abort controller map.
   *   * @param {string} endpoint - The endpoint to abort, which can be a full URL or
   * a route path (e.g. "/users/:id").
   */
  abortRequest(endpoint: string): void {
    this.abortControllers.forEach((controller, key) => {
      if (key.startsWith(endpoint)) {
        controller.abort();
        this.abortControllers.delete(key);
      }
    });
  }

  /**
   * Updates the internal configuration with the given options.
   *
   * Any `reconnect` options will be merged with the existing `reconnect` config.
   *
   * If a new `baseURL` is provided and no `wsURL` is specified, the `wsURL` will
   * be updated to be the same as the `baseURL` but with the `ws` or `wss` scheme.
   *
   * @param {Partial<ReavixConfig>} newConfig - The new configuration options.
   */
  updateConfig(newConfig: Partial<ReavixConfig>): void {
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
  }
}
