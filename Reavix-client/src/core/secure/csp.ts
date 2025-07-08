/**
 * Content Securityt Policy helper
 */

export class CSPHelper {
  private policies: Record<string, string[]>;

  constructor(policies: Record<string, string[]> = {}) {
    this.policies = {
      "default-src": ["'self'"],
      "script-src": ["'self'", "'unsafe-inline'"],
      "content-src": ["'self'"],
      ...policies,
    };
  }

  /**
   * Checks if the given URL is allowed according to the "connect-src" policy.
   *
   * The method takes a URL string as an argument and returns a boolean indicating
   * whether the request is allowed according to the policy. The policy is
   * evaluated as follows:
   *
   * - If the policy is not set, the request is allowed.
   * - If the policy is set to "'self'", the request is allowed if the URL origin
   *   matches the origin of the current page.
   * - If the policy is set to "none", the request is not allowed.
   * - If the policy is set to "*", the request is allowed.
   * - If the policy is set to a string (e.g. "https://example.com"), the request
   *   is allowed if the URL starts with the given string.
   *
   * @param url - The URL to check.
   * @returns true if the request is allowed, false otherwise.
   */
  validateRequest(url: string): boolean {
    const urlObj = new URL(url);
    const connectSources = this.policies["connect-src"] || [];

    return connectSources.some((source) => {
      switch (source) {
        case "'self'":
          return urlObj.origin === window.location.origin;
        case "none":
          return false;
        case "*":
          return true;
        default:
          return url.startsWith(source);
      }
    });
  }
}
