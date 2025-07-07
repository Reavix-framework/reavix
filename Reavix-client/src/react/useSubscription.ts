import { useEffect, useRef } from "react";
import { ReavixClient } from "../core";

type SubscriptionOptions = {
  once?: boolean;
  validate?: (payload: unknown) => boolean;
};

/**
 * Hook to subscribe to an event.
 *
 * @param client - The Reavix client instance
 * @param eventName - The name of the event to subscribe to
 * @param callback - The function to call when the event is emitted
 * @param options - Options for the subscription
 * @param options.once - If true, the subscription will be unsubscribed after the first emission
 * @param options.validate - A function to validate the event payload
 * @returns The unsubscribe function
 */
export function useSubscription<T = unknown>(
  client: ReavixClient,
  eventName: string,
  callback: (data: T) => void,
  options: SubscriptionOptions = {}
) {
  const callbackRef = useRef(callback);
  callbackRef.current = callback;

  useEffect(() => {
    const unsubscribe = client.on(eventName, (data: T) => {
      if (!options.validate || options.validate(data)) {
        callbackRef.current(data);
      }
    });

    return () => {
      unsubscribe();
    };
  }, [client, eventName, options.once, options.validate]);
}
