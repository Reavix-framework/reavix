import { useState, useEffect } from "react";
import { useReavix } from "./ReavixProvider";

/**
 * A React component that renders a panel with debug events from the Reavix client.
 * The panel is only shown in development mode.
 *
 * The panel is toggleable by clicking on the header, and it will show the 20 most
 * recent debug events.
 *
 * Each event is displayed as a <code>div</code> with a <code>strong</code> type
 * and a <code>pre</code> with the event data.
 *
 * The panel is fixed to the bottom right of the page, and its width and height are
 * set to 400px.
 *
 * The component also sets up the Reavix client to enable debug mode and to emit
 * events to the component.
 *
 * @returns {JSX.Element} The component element.
 */
export function ReavixDevTools() {
  const client = useReavix();
  const [debugEvents, setDebugEvents] = useState<any[]>([]);
  const [isPanelOpen, setIsPanelOpen] = useState(false);

  useEffect(() => {
    if (process.env.NODE_ENV === "development") {
      client.enableDebug();
      const handleDebugEvents = () => setDebugEvents(client.getDebugEvents());
      client.on("*", handleDebugEvents);
      return () => client.off("*", handleDebugEvents);
    }
  }, [client]);

  if (process.env.NODE_ENV !== "development") return null;

  const togglePanel = () => setIsPanelOpen((prevState) => !prevState);

  return (
    <div
      style={{
        position: "fixed",
        bottom: 0,
        right: 0,
        width: "400px",
        height: isPanelOpen ? "400px" : "40px",
        backgroundColor: "#f5f5f5",
        border: "1px solid #ddd",
        cursor: "pointer",
        zIndex: 9999,
        transition: "height 0.3s",
      }}
    >
      <div
        style={{
          padding: "10px",
          backgroundColor: "#333",
          color: "white",
          cursor: "pointer",
        }}
        onClick={togglePanel}
      >
        Reavix DevTools ({debugEvents.length})
      </div>

      {isPanelOpen && (
        <div
          style={{
            padding: "10px",
            overflowY: "auto",
            height: "calc(100% - 40px)",
          }}
        >
          {debugEvents.map((event, index) => (
            <div
              key={index}
              style={{
                marginBottom: "10px",
                padding: "5px",
                borderBottom: "1px solid #eee",
              }}
            >
              <strong>{event.type}</strong>
              <pre style={{ fontSize: "12px", whiteSpace: "pre-wrap" }}>
                {JSON.stringify(event.data, null, 2)}
              </pre>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
