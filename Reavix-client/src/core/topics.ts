import { WebSocketClient } from "./ws";
/**
 * Topic-based subscription manager
 */

export class TopicManager {
  private ws: WebSocketClient;
  private subscribedTopics: Set<string> = new Set();
  private topicCallback: Map<string, Set<Function>> = new Map();

  constructor(ws: WebSocketClient) {
    this.ws = ws;
  }

  /**
   * Subscribe to a topic. If the topic has not been subscribed to yet,
   * this will send a subscription request to the server.
   *
   * @param topic - The topic to subscribe to
   * @param handler - The callback to call when a message is received
   * @returns An unsubscribe function
   */
  subscribe<T = unknown>(
    topic: string,
    handler: (payload: T) => void
  ): () => void {
    // Ensure the topic has an entry in the callback map
    if (!this.topicCallback.has(topic)) {
      this.topicCallback.set(topic, new Set());
    }

    // Add the handler to the set of callbacks for the topic
    this.topicCallback.get(topic)?.add(handler);

    // Subscribe to the topic if not already subscribed
    if (!this.subscribedTopics.has(topic)) {
      this.ws
        .emit("subscribe", { topic })
        .then(() => this.subscribedTopics.add(topic))
        .catch((error) => console.error("Subscription error:", error));
    }

    // Return an unsubscribe function
    return () => this.unsubscribe(topic, handler);
  }

  /**
   * Unsubscribe from a topic. If a callback is provided, only the specific
   * callback will be unsubscribed, otherwise all callbacks for the topic will
   * be unsubscribed.
   *
   * @param topic - The topic to unsubscribe from
   * @param callback - The callback to unsubscribe, if not provided all will be unsubscribed
   */
  unsubscribe(topic: string, callback?: Function): void {
    const callbacks = this.topicCallback.get(topic);

    if (callback && callbacks) {
      callbacks.delete(callback);

      if (callbacks.size === 0) {
        this.topicCallback.delete(topic);
        this.subscribedTopics.delete(topic);
        this.ws.emit("unsubscribe", { topic });
      }
    } else {
      this.topicCallback.delete(topic);
      this.subscribedTopics.delete(topic);
      this.ws.emit("unsubscribe", { topic });
    }
  }

  /**
   * Internal method to handle messages received from the WebSocket connection.
   *
   * Will iterate over all registered callbacks for the given topic and call
   * them with the received payload.
   *
   * @param receivedTopic - The topic the message was received for
   * @param payload - The payload of the received message
   */
  handleMessage(receivedTopic: string, payload: unknown): void {
    const callbacks = this.topicCallback.get(receivedTopic);
    callbacks?.forEach((callback) => callback(payload));
  }
}
