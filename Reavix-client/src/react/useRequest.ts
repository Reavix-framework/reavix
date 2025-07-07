import { useEffect, useRef, useState } from "react";
import { ReavixClient } from "../core";

type RequestState<T> = {
  data: T | null;
  error: Error | null;
  loading: boolean;
};

/**
 * Custom React hook to perform HTTP requests using the Reavix client.
 *
 * @template T - The expected response data type.
 * @param {ReavixClient} client - The Reavix client instance to use for making requests.
 * @param {string} endpoint - The endpoint URL to send the request to.
 * @param {Object} options - Optional request configuration.
 * @param {"GET" | "POST" | "PUT" | "DELETE" | "PATCH"} [options.method="GET"] - HTTP method to use for the request.
 * @param {Record<string, string | number | boolean>} [options.params] - URL parameters to include in the request.
 * @param {unknown} [options.body] - Request body to send, usually for POST/PUT requests.
 * @param {boolean} [options.immediate=true] - Whether to execute the request immediately upon hook initialization.
 * @returns {Object} An object containing the request state and functions to execute or abort the request.
 * @returns {T | null} .data - The response data, or null if no response yet.
 * @returns {Error | null} .error - The error object if the request failed, or null otherwise.
 * @returns {boolean} .loading - Indicates if the request is in progress.
 * @returns {Function} .execute - Function to manually execute the request with optional overrides.
 * @returns {Function} .abort - Function to abort the ongoing request.
 */

export function useRequest<T = unknown>(
  client: ReavixClient,
  endpoint: string,
  {
    method = "GET",
    params,
    body,
    immediate = true,
  }: {
    method?: "GET" | "POST" | "PUT" | "DELETE" | "PATCH";
    params?: Record<string, string | number | boolean>;
    body?: unknown;
    immediate?: boolean;
  } = {}
) {
  const [state, setState] = useState<RequestState<T>>({
    data: null,
    loading: false,
    error: null,
  });

  const abortController = useRef<AbortController>(new AbortController());

  const execute = async ({
    method: overrideMethod,
    params: overrideParams,
    body: overrideBody,
  }: {
    method?: typeof method;
    params?: typeof params;
    body?: typeof body;
  } = {}) => {
    const mergedOptions = {
      method: overrideMethod ?? method,
      params: { ...params, ...overrideParams },
      body: overrideBody ?? body,
      signal: abortController.current.signal,
    };

    abortController.current.abort();
    abortController.current = new AbortController();

    setState((prev) => ({
      ...prev,
      loading: true,
      error: null,
    }));

    try {
      const response = await client.request<T>(endpoint, mergedOptions);

      setState({ data: response.data, loading: false, error: null });
    } catch (error) {
      if (error instanceof Error && error.name !== "AbortError") {
        setState((prev) => ({
          ...prev,
          loading: false,
          error,
        }));
      }
    }
  };

  useEffect(() => {
    if (immediate) {
      execute();
    }

    return () => {
      abortController.current.abort();
    };
  }, [endpoint, JSON.stringify(params)]);

  return {
    ...state,
    execute,
    abort: () => abortController.current.abort(),
  };
}
