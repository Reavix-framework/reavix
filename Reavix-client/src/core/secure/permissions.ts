import { ReavixClient } from "../index";

type Permission = {
  route: string;
  methods: string[];
};

/**
 * Client-side permission validator
 */
export class PermissionValidator {
  private permissions: Permission[] = [];
  private permissionCache = new Map<string, boolean>();

  /**
   * Loads the permissions configuration from the Reavix backend.
   *
   * Fetches the permissions from the `/permissions` endpoint and updates the
   * internal list of permissions. If the request fails, logs an error message
   * to the console.
   *
   * @param client - The Reavix client instance to use for making requests.
   */
  async loadPermissions(client: ReavixClient): Promise<void> {
    try {
      const response = await client.request<Permission[]>("/permissions");
      this.permissions = response.data;
      this.permissionCache.clear();
    } catch (error) {
      console.error("Failed to load permissions:", error);
    }
  }

  /**
   * Checks if the given method and path are permitted according to the
   * internal list of permissions.
   *
   * If the permission has been checked before, the result is returned from
   * the cache. Otherwise, the internal list of permissions is searched, and
   * the result is cached for subsequent lookups.
   *
   * @param method - HTTP method to check (e.g. GET, POST, PUT, DELETE)
   * @param path - URL path to check (e.g. /users, /users/:id)
   * @returns true if the method and path are permitted, false otherwise
   */
  isPermitted(method: string, path: string): boolean {
    const cacheKey = `${method}:${path}`;
    if (this.permissionCache.has(cacheKey)) {
      return this.permissionCache.get(cacheKey)!;
    }

    const permitted = this.permissions.some((permission) => {
      const routeMatches = this.matchPath(path, permission.route);
      const methodMatches =
        permission.methods.includes(method) || permission.methods.includes("*");
      return routeMatches && methodMatches;
    });

    this.permissionCache.set(cacheKey, permitted);
    return permitted;
  }

  /**
   * Matches the given request path against the permission path.
   *
   * Simple wildcard matching is supported, where a trailing `/*` is
   * interpreted as a prefix match. For example, `/users/*` will match
   * `/users`, `/users/123`, `/users/456`, etc.
   *
   * @param requestPath - The path to check (e.g. `/users/123`)
   * @param permissionPath - The permission path to match against (e.g. `/users/*`)
   * @returns true if the request path matches the permission path, false otherwise
   */
  private matchPath(requestPath: string, permissionPath: string): boolean {
    if (permissionPath === "*" || permissionPath === requestPath) {
      return true;
    }

    // Simple wildcard matching (e.g., /users/*)
    if (permissionPath.endsWith("/*")) {
      const basePath = permissionPath.slice(0, -2);
      return requestPath.startsWith(basePath);
    }

    return false;
  }
}
