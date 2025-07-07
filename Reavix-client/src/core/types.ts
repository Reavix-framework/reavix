/**
 * Core types for Reavix Client SDK
 */

export type ReavixConfig = {
  /** Base URL for HTTP requests */
  baseURL?: string;

  /**WebSocket URL (defaults to baseURL with ws/wss protocol) */
  wsURL?: string;

  /**Enable debug logging */
  debug?: boolean;

  /**Default headers for all requests */
  headers?: Record<string, string>;

  /**Time for HTTP requests in milliseconds */
  timeout?: number;

  /**Auto-reconnect settings for WebSocket */
  reconnect?: {
    /**Enable auto-reconnect */
    enabled?: boolean;

    /**Delay between reconnect attempts in ms */
    delay?: number;

    /**Maximum number of reconnect attempts */
    maxAttempts?: number;

    /**Exponential backoff factor */
    backoffFactor?: number;
  };
};

export type RequestOptions = {
  method?: "GET" | "POST" | "PUT" | "DELETE" | "PATCH";
  headers?: Record<string, string>;
  params?: Record<string, string | number | boolean>;
  body?: unknown;
  timeout?: number;
  signal?: AbortSignal;
};

export type Response<T = unknown> = {
  status: number;
  data: T;
  headers: Record<string, string>;
};

/**
 * Event subscription options
 */
export type SubscriptionOptions = {
  /**Only trigger callback once */
  once?: boolean;

  /**Custom validator for payload */
  validate?: (payload: unknown) => boolean;
};

/**
 * Event handler with metadata
 */
export type EventCallback<T = unknown> = (data: T) => void;

export type EventHandler<T = unknown> = {
  callback: EventCallback<T>;
  options: SubscriptionOptions;
  onceCalled: boolean;
};

/**
 * Event Emitter interface
 */
export interface EventEmitter {
  on<T = unknown>(
    eventName: string,
    callback: EventCallback<T>,
    options?: SubscriptionOptions
  ): () => void;
  emit(eventName: string, data: unknown): Promise<void>;
  off(eventName: string, callback?: EventCallback): void;
}

/**Validated event definition */
export type EventDefinition<T = unknown> = {
  /**Event name pattern (support wildcards) */
  name: string;

  /**Type guard for payload validation */
  guard?: (payload: unknown) => payload is T;

  /**JSON schema for validation */
  schema?: unknown;
};
