import { useRef, useEffect } from "react";
import { ReavixClient } from "../core";

/**
 * Subscribe to a topic and execute a callback when a message is received.
 *
 * Automatically unsubscribes when the component is unmounted.
 *
 * @param client - The Reavix client instance
 * @param topic - The topic to subscribe to
 * @param callback - The callback to execute when a message is received
 * @returns {void}
 * @example
 * import { useTopic } from "@reavix/client";
 *
 * function MyComponent() {
 *   useTopic(reavix, "my-topic", (data) => {
 *     console.log(data);
 *   });
 * }
 */
export function useTopic<T = unknown>(
  client: ReavixClient,
  topic: string,
  callback: (data: T) => void
) {
  const callbackRef = useRef(callback);
  callbackRef.current = callback;

  useEffect(() => {
    const unsubscribe = client.subscribe(topic, (data: T) => {
      callbackRef.current(data);
    });

    return () => {
      unsubscribe();
    };
  }, [client, topic]);
}
