import { useEffect, useMemo, useRef, useState } from "react";
import { ReavixClient } from "../core";

type QueryOptions<T> = {
  enabled?: boolean;
  refetchInterval?: number;
  onSuccess?: (data: T) => void;
  onError?: (error: Error) => void;
};

/**
 * React hook to perform a query using the Reavix client.
 *
 * @param {ReavixClient} client - The Reavix client instance to use for making requests.
 * @param {string} endpoint - The endpoint URL to send the request to.
 * @param {Object} options - Optional request configuration.
 * @param {Record<string, string | number | boolean>} [options.params] - URL parameters to include in the request.
 * @param {"GET" | "POST" | "PUT" | "DELETE" | "PATCH"} [options.method="GET"] - HTTP method to use for the request.
 * @param {unknown} [options.body] - Request body to send, usually for POST/PUT requests.
 * @param {boolean} [options.enabled=true] - Whether to enable the query.
 * @param {number} [options.refetchInterval] - Interval in milliseconds to refetch the data.
 * @param {Function} [options.onSuccess] - Callback function to call when the request is successful.
 * @param {Function} [options.onError] - Callback function to call when the request fails.
 * @returns {Object} An object containing the request state and functions to refetch or abort the request.
 * @returns {T | null} .data - The response data, or null if no response yet.
 * @returns {Error | null} .error - The error object if the request failed, or null otherwise.
 * @returns {boolean} .isLoading - Indicates if the request is in progress.
 * @returns {boolean} .isFetching - Indicates if the request is currently being refetched.
 * @returns {Function} .refetch - Function to manually refetch the data.
 * @returns {Function} .abort - Function to abort the ongoing request.
 */
export function useReavixQuery<T = unknown>(
  client: ReavixClient,
  endpoint: string,
  options: {
    params?: Record<string, string | number | boolean>;
    method?: "GET" | "POST" | "PUT" | "DELETE" | "PATCH";
    body?: unknown;
  } & QueryOptions<T>
) {
  const [state, setState] = useState<{
    data: T | null;
    error: Error | null;
    isLoading: Boolean;
    isFetching: boolean;
  }>({
    data: null,
    error: null,
    isLoading: false,
    isFetching: false,
  });

  const [version, setVersion] = useState(0);
  const abortControllerRef = useRef<AbortController>(new AbortController());

  const paramString = useMemo(
    () => JSON.stringify(options.params || {}),
    [options.params]
  );

  const refetch = () => {
    setVersion((v) => v + 1);
  };

  useEffect(() => {
    if (options.enabled === false) return;

    const fetchData = async () => {
      abortControllerRef.current?.abort();
      abortControllerRef.current = new AbortController();

      setState((prev) => ({
        ...prev,
        isLoading: prev.data === null,
        isFetching: true,
      }));
      try {
        const response = await client.request<T>(endpoint, {
          method: options.method,
          params: options.params,
          body: options.body,
          signal: abortControllerRef.current.signal,
        });
        setState((prev) => ({
          ...prev,
          data: response.data,
          error: null,
          isLoading: false,
          isFetching: false,
        }));
        options.onSuccess?.(response.data);
      } catch (error) {
        if (error instanceof Error && error.name === "AbortError") {
          setState((prev) => ({
            ...prev,
            isLoading: false,
            isFetching: false,
          }));
          options.onError?.(error);
        }
      }
    };

    fetchData();

    let intervalId: number | undefined;
    if (options.refetchInterval) {
      intervalId = window.setInterval(fetchData, options.refetchInterval);
    }

    return () => {
      abortControllerRef.current?.abort();
      if (!intervalId) clearInterval(intervalId);
    };
  }, [
    endpoint,
    paramString,
    options.method,
    options.params,
    options.body,
    options.enabled,
    options.refetchInterval,
    version,
  ]);

  return {
    ...state,
    refetch,
    abort: () => abortControllerRef.current.abort(),
  };
}
