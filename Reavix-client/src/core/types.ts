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
};

export type Response<T = unknown> = {
  status: number;
  data: T;
  headers: Record<string, string>;
};

export type EventCallback<T = unknown> = (payload: T) => void;
