type CorsConfig = {
  allowedOrigins: string[];
  allowCredentials: boolean;
};

/**
 * CORS policy enforcer
 */
export class CorsEnforcer {
  private config: CorsConfig;

  constructor(config: CorsConfig) {
    this.config = config;
  }

  /**
   * Validates the given origin against the allowed origins in the CORS configuration.
   *
   * This method checks if the specified origin is included in the list of allowed origins
   * or if all origins are allowed (indicated by "*"). If either condition is met, the
   * origin is considered valid.
   *
   * @param origin - The origin to validate.
   * @returns `true` if the origin is allowed or if all origins are permitted; otherwise, `false`.
   */

  validateOrigin(origin: string): boolean {
    if (this.config.allowedOrigins.includes("*")) {
      return true;
    }
    return this.config.allowedOrigins.includes(origin);
  }

  /**
   * Applies the CORS headers to the given headers object.
   *
   * If the origin is not allowed according to the CORS configuration, the headers are
   * returned unchanged. Otherwise, the `Access-Control-Allow-Origin` header is set to
   * the specified origin, and the `Access-Control-Allow-Credentials` header is set to
   * `"true"` if the `allowCredentials` option is enabled in the CORS configuration.
   * Additionally, the `Vary` header is set to `"Origin"`.
   *
   * @param headers - The headers object to which the CORS headers should be applied.
   * @param origin - The origin for which the CORS headers should be applied.
   * @returns The headers object with the CORS headers applied if the origin is allowed.
   */
  applyHeaders(
    headers: Record<string, string>,
    origin: string
  ): Record<string, string> {
    if (!this.validateOrigin(origin)) {
      return headers;
    }

    return {
      ...headers,
      "Access-Control-Allow-Origin": origin,
      "Access-Control-Allow-Credentials": this.config.allowCredentials
        ? "true"
        : "false",
      Vary: "Origin",
    };
  }
}
