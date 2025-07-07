import type { EventDefinition } from "./types";

/**
 * Event validation manager
 */
export class EventValidator {
  private eventDefinitions: Map<string, EventDefinition> = new Map();

  /**
   * Register multiple event definitions.
   *
   * @param definitions - An array of event definitions to register.
   */
  register(defintions: EventDefinition[]): void {
    defintions.forEach((def) => {
      this.eventDefinitions.set(def.name, def);
    });
  }

  validate(
    eventName: string,
    payload: unknown
  ): {
    valid: boolean;
    error?: string;
  } {
    const definition = this.findDefinition(eventName);
    if (!definition) return { valid: true };

    //check type guard if provided
    if (definition.guard && !definition.guard(payload)) {
      return {
        valid: false,
        error: `Payload failed type guard for event ${eventName}`,
      };
    }

    //Here will implement the JSON schema validation schema if provided.
    //Hope to use ajv library to validate the schema

    return { valid: true };
  }

  /**
   * Finds an event definition by event name, supporting both exact and wildcard matches.
   *
   * @param eventName - The name of the event to find the definition for.
   * @returns The event definition if found, otherwise undefined.
   *          An exact match takes precedence over a wildcard match.
   */

  private findDefinition(eventName: string): EventDefinition | undefined {
    // Exact match
    const exactMatch = this.eventDefinitions.get(eventName);
    if (exactMatch) return exactMatch;

    // Wildcard match
    for (const [pattern, definition] of this.eventDefinitions) {
      if (pattern.includes("*")) {
        const regexPattern = pattern.replace(/\*/g, ".*");
        const regex = new RegExp(`^${regexPattern}$`);
        if (regex.test(eventName)) return definition;
      }
    }

    return undefined;
  }
}
