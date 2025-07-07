import React, { createContext, useContext, useEffect, useMemo } from "react";
import { ReavixClient, getReavixClient } from "../core";
import type { ReavixConfig } from "../core";

const ReavixContext = createContext<ReavixClient | null>(null);

type reavixProviderProps = {
  config: ReavixConfig;
  children: React.ReactNode;
};

/**
 * Provides the Reavix client to the application via context.
 *
 * @example
 * import React from "react";
 * import ReactDOM from "react-dom";
 * import { ReavixProvider, ReavixClient } from "@reavix/client";
 *
 * const client = new ReavixClient({
 *   baseURL: "https://example.com/api",
 *   wsURL: "wss://example.com/ws",
 * });
 *
 * ReactDOM.render(
 *   <ReavixProvider config={client.config}>
 *     <App />
 *   </ReavixProvider>,
 *   document.getElementById("root")
 * );
 *
 * @param {reavixProviderProps} props The props for the provider.
 * @returns {JSX.Element}
 */
export function ReavixProvider({ config, children }: reavixProviderProps) {
  const client = useMemo(
    () => getReavixClient(config),
    [config.baseURL, config.wsURL]
  );

  //cleanup on unmount
  useEffect(() => {
    return () => {
      client.disconnect();
    };
  }, [client]);

  return (
    <ReavixContext.Provider value={client}>{children}</ReavixContext.Provider>
  );
}

/**
 * Hook to get the Reavix client instance.
 *
 * Must be used within a <ReavixProvider> component.
 *
 * @returns {ReavixClient} The Reavix client instance.
 *
 * @example
 * import { useReavix } from "@reavix/client";
 *
 * function MyComponent() {
 *   const reavix = useReavix();
 *   const { data, error } = reavix.request("/users/me");
 *
 *   if (error) return <div>Error: {error.message}</div>;
 *   if (!data) return <div>Loading...</div>;
 *
 *   return <div>Welcome, {data.name}</div>;
 * }
 */
export function useReavix() {
  const client = useContext(ReavixContext);

  if (!client) {
    throw new Error("useReavix must be used within a ReavixProvider");
  }

  return client;
}
