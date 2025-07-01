import { useEffect, useState } from "react";

export default function useWebSocket(url: string) {
  const [data, setDate] = useState(null);
  const [status, setStatus] = useState<"connecting" | "open" | "closed">(
    "connecting"
  );

  useEffect(() => {
    const ws = new WebSocket(url);

    ws.onopen = () => setStatus("open");
    ws.onclose = () => setStatus("closed");
    ws.onmessage = (event) => setDate(event.data);

    return () => ws.close();
  }, [url]);

  return { data, status };
}
