import type { RetryPolicy } from "./types";

export class ReavixError extends Error {
  constructor(
    public message: string,
    public code: string,
    public originalError?: unknown,
    public metadata?: Record<string, unknown>
  ) {
    super(message);
    this.name = "ReavixError";
  }

  /**
   * Creates a `ReavixError` instance from an unknown error type.
   *
   * @param {unknown} error - The error to convert into a `ReavixError`.
   * @returns {ReavixError} A `ReavixError` instance representing the given error.
   * If the input is already a `ReavixError`, it is returned directly.
   * If the input is an instance of `Error`, its message is used with a code of "UNKNOWN".
   * Otherwise, the input is converted to a string and used as the message with a code of "UNKNOWN".
   */

  static from(error: unknown): ReavixError {
    if (error instanceof ReavixError) return error;
    if (error instanceof Error)
      return new ReavixError(error.message, "UNKNOWN", error);
    return new ReavixError(String(error), "UNKNOWN", error);
  }
}

export class RetryHandler {
  /**
   * Executes the given operation with a retry policy.
   *
   * @param {() => Promise<T>} operation - The operation to execute with a retry policy.
   * @param {RetryPolicy} [policy] - The retry policy to use. If not specified, uses a default policy with 3 attempts, a delay of 1000ms,
   * and a maximum delay of 30000ms. The retryIf function is set to always retry.
   * @returns {Promise<T>} A promise that resolves with the result of the operation, or rejects with the last error if all retries failed.
   */
  static async executeWithRetry<T>(
    operation: () => Promise<T>,
    policy: RetryPolicy = {}
  ): Promise<T> {
    const resolvedPolicy: Required<RetryPolicy> = {
      attempts: policy.attempts ?? 3,
      delay: policy.delay ?? 1000,
      maxDelay: policy.maxDelay ?? 30000,
      retryIf: policy.retryIf ?? (() => true),
    };

    let attempt = 0;
    let lastError: unknown;

    while (attempt <= resolvedPolicy.attempts) {
      try {
        return await operation();
      } catch (error) {
        lastError = error;
        attempt++;

        if (
          attempt > resolvedPolicy.attempts ||
          !resolvedPolicy.retryIf(error)
        ) {
          break;
        }

        const delay = Math.min(
          resolvedPolicy.delay * Math.pow(2, attempt - 1),
          resolvedPolicy.maxDelay
        );
        await new Promise((resolve) => setTimeout(resolve, delay));
      }
    }

    throw ReavixError.from(lastError);
  }
}
