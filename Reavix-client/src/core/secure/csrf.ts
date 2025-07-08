import { ReavixClient } from "..";
import type { Response } from "..";
/**
 * CSRF protection manager
 */
export class CsrfManager {
  private token: string | null = null;
  private headerName = "X-CSRF-Token";

  /**
   * Initializes CSRF protection manager.
   *
   * Makes a request to `/csrf-token` and fetches the token to be used for
   * subsequent requests.
   *
   * @param client - The Reavix client instance to use for making requests.
   */
  async initialize(client: ReavixClient): Promise<void> {
    const response = await client.request<{ csrfToken: string }>("/csrf-token");
    this.token = response.data.csrfToken;
  }

  /**
   * Get the current CSRF token.
   *
   * Returns the current CSRF token if available, or null if no token has been
   * fetched yet.
   */
  getToken(): string | null {
    return this.token;
  }

  /**
   * Add the CSRF token to the given request headers if available.
   *
   * If the CSRF token has been fetched, this method adds it to the given
   * request headers. Otherwise, the original headers are returned.
   *
   * @param headers - The request headers to protect with a CSRF token.
   * @returns The protected headers.
   */
  protectRequest(headers: Record<string, string>): Record<string, string> {
    if (this.token) {
      return {
        ...headers,
        [this.headerName]: this.token,
      };
    }
    return headers;
  }

  /**
   * Updates the CSRF token when a response with a new token is received.
   *
   * When a response is received with a new CSRF token in the headers, this
   * method updates the current CSRF token with the new one.
   * @param response - The response object containing the new CSRF token.
   */
  validateResponse(response: Response): void {
    const newToken = response.headers[this.headerName];
    if (newToken) {
      this.token = newToken;
    }
  }
}
