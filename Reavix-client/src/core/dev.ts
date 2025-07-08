import type { ReavixClient } from ".";

interface HotReloadConfig {
  /**Watch for changes in these endpoints */
  endpoints?: string[];
  /**Debounce time in ms */
  debounce?: number;
}

export class HotReloadManager {
  private static instance: HotReloadManager;
  private observers: Map<string, Function[]> = new Map();
  private timers: Map<string, ReturnType<typeof setTimeout>> = new Map();
  private constructor(private client: ReavixClient) {}

  /**
   * Initializes the HotReloadManager.
   *
   * This method is used to create a single instance of the HotReloadManager.
   * It is a singleton and will return the same instance if called multiple
   * times.
   *
   * @param client - The ReavixClient instance to use for making requests.
   * @returns The HotReloadManager instance.
   */
  static init(client: ReavixClient): HotReloadManager {
    if (!HotReloadManager.instance) {
      HotReloadManager.instance = new HotReloadManager(client);
    }
    return HotReloadManager.instance;
  }

  /**
   * Watches an endpoint for changes and calls the given callback function when
   * a change is detected.
   *
   * The callback function will receive the response data from the endpoint as
   * its only argument.
   *
   * An optional `config` object can be passed with the following properties:
   * - `endpoints`: An array of endpoints to watch. If not provided, the
   *   `endpoint` argument will be used as the only endpoint to watch.
   * - `debounce`: The amount of time in milliseconds to debounce the requests.
   *   If not provided, a default value of 5000 (5 seconds) will be used.
   *
   * If the callback function returns a promise, it will be awaited before
   * scheduling the next request.
   *
   * The returned function can be used to stop watching the endpoint.
   *
   * @param endpoint - The endpoint to watch.
   * @param callback - The callback function to call when a change is detected.
   * @param config - Optional configuration object.
   * @returns A function that can be used to stop watching the endpoint.
   */
  watchEndpoint<T = unknown>(
    endpoint: string,
    callback: (data: T) => void,
    config: HotReloadConfig = {}
  ): () => void {
    if (!this.observers.has(endpoint)) {
      this.observers.set(endpoint, []);
    }

    const endpointObservers = this.observers.get(endpoint)!;
    endpointObservers.push(callback);

    this.fetchEndpoint(endpoint);

    const refreshInterval = setInterval(
      () => this.fetchEndpoint(endpoint),
      config.debounce || 5000
    );

    return () => {
      clearInterval(refreshInterval);
      this.observers.set(
        endpoint,
        endpointObservers.filter((observer) => observer !== callback)
      );
    };
  }

  /**
   * Fetches the given endpoint and calls all registered callbacks with the
   * received data.
   *
   * If the endpoint is currently being fetched, it will be refetched after the
   * current fetch is finished.
   *
   * @param endpoint - The endpoint to fetch.
   * @returns A promise that resolves when all callbacks have been called.
   */
  private async fetchEndpoint(endpoint: string): Promise<void> {
    const timerId = this.timers.get(endpoint);
    if (timerId) {
      clearTimeout(timerId);
    }

    this.timers.set(
      endpoint,
      setTimeout(async () => {
        try {
          const response = await this.client.request(endpoint);
          const callbackFunctions = this.observers.get(endpoint) || [];
          await Promise.all(
            callbackFunctions.map((func) => func(response.data))
          );
        } catch (error) {
          console.error(`Hot reload failed for ${endpoint}`, error);
        }
      }, 300)
    );
  }
}
