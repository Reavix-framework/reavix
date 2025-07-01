import { useState, useEffect } from "react";
import ConnectionStatus from "./components/ConnectionStatus";

function App() {
  const [backendStatus, setBackendStatus] = useState<
    "connecting" | "connected" | "error"
  >("connecting");

  useEffect(() => {
    //Test backend conection

    fetch("/api/health")
      .then(() => setBackendStatus("connected"))
      .catch(() => setBackendStatus("error"));
  }, []);

  return (
    <>
      <div className="min-h-screen bg-gray-50">
        <header className="bg-white shadow">
          <div className="max-w-7xl mx-auto py-6 px-4 sm:px-6 lg:px-8">
            <h1 className="text-3xl font-bold text-gray-900"> Reavix App</h1>
          </div>
        </header>
        <main>
          <div className="max-w-7xl mx-auto py-6 sm:px-6 lg:px-8">
            <ConnectionStatus status={backendStatus} />
          </div>
        </main>
      </div>
    </>
  );
}

export default App;
