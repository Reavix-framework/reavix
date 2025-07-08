interface DebugEvent {
  type: string;
  timestamp: number;
  data: unknown;
}

export class DebugLogger {
  private events: DebugEvent[] = [];
  private enabled = false;

  enable(): void {
    this.enabled = true;
  }

  disable(): void {
    this.enabled = false;
  }

  log(type: string, data: unknown): void {
    const event = { type, timestamp: Date.now(), data };
    this.events.push(event);

    if (this.enabled) {
      console.debug(`[REAVIX:${type}]`, data);
    }
  }

  getEvents(filter?: { type?: string }): DebugEvent[] {
    return this.events.filter(
      (event) => !filter?.type || event.type === filter.type
    );
  }

  clear(): void {
    this.events = [];
  }
}

// Global debug instance
export const debug = new DebugLogger();
