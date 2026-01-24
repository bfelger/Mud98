import React from "react";
import ReactDOM from "react-dom/client";
import { ModuleRegistry, AllCommunityModule } from "ag-grid-community";
import App from "./App";
import { LocalFileRepository } from "./repository/localFileRepository";
import "./index.css";

// Register AG Grid modules (required for v31+)
ModuleRegistry.registerModules([AllCommunityModule]);

const repository = new LocalFileRepository();

ReactDOM.createRoot(document.getElementById("root") as HTMLElement).render(
  <React.StrictMode>
    <App repository={repository} />
  </React.StrictMode>
);
