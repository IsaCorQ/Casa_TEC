import React, { useState, useEffect } from "react";
import "./App.css";

function App() {
  const [luces, setLuces] = useState({
    cuarto1: false,
    cuarto2: false,
    sala: false,
    comedor: false,
    cocina: false,
  });

  const [puertas, setPuertas] = useState({
    delantera: false,
    trasera: false,
    cuarto1: false,
    cuarto2: false,
  });

  const [foto, setFoto] = useState(null);

  // Función para obtener el estado de las luces desde el servidor
  const obtenerEstadoLuces = async () => {
    try {
      const response = await fetch("http://127.0.0.1:5000/estado_luces");
      const data = await response.json();
      setLuces(data);
    } catch (error) {
      console.error("Error obteniendo estado de las luces:", error);
    }
  };

  // Función para encender/apagar una luz
  // Modifica el fetch para capturar mejor los errores
const toggleLuz = async (luz) => {
  try {
    const endpoint = luces[luz] ? "/apagar_luz" : "/encender_luz";
    const response = await fetch(`http://127.0.0.1:5000${endpoint}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ luz }),
    });
    
    console.log("Response status:", response.status);  // <- Agrega esto
    const data = await response.json();
    console.log("Response data:", data);  // <- Y esto
    
    setLuces(data.estado);
  } catch (error) {
    console.error("Error completo:", error);
  }
};

// Obtener estado de puertas
  const obtenerEstadoPuertas = async () => {
    try {
      const response = await fetch("http://127.0.0.1:5000/estado_puertas");
      if (!response.ok) throw new Error("Error HTTP: " + response.status);
      const data = await response.json();
      setPuertas(data);
    } catch (error) {
      console.error("Error obteniendo puertas:", error);
    }
  };

  // Función para actualizar el estado de una puerta
  const actualizarPuerta = async (puerta, estado) => {
    try {
      await fetch("http://127.0.0.1:5000/actualizar_puerta", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ puerta, estado }),
      });
      obtenerEstadoPuertas(); // Actualizar el estado de las puertas
    } catch (error) {
      console.error("Error actualizando el estado de la puerta:", error);
    }
  };

  // Función para tomar una foto
  const tomarFoto = async () => {
    try {
      const response = await fetch("http://127.0.0.1:5000/tomar_foto");
      const blob = await response.blob();
      setFoto(URL.createObjectURL(blob));
    } catch (error) {
      console.error("Error tomando la foto:", error);
    }
  };

  // Actualizar el estado de las puertas cada 5 segundos
  useEffect(() => {
    const interval = setInterval(obtenerEstadoPuertas, 5000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="App">
      <h1>Control y Monitoreo de Casa Inteligente</h1>
      <div className="dashboard">
        <div className="luces">
          <h2>Luces</h2>
          <button
            onClick={() => toggleLuz("cuarto1")}
            style={{ backgroundColor: luces.cuarto1 ? "#50fa7b" : "#ff79c6" }}
          >
            Cuarto 1: {luces.cuarto1 ? "Encendida" : "Apagada"}
          </button>
          <button
            onClick={() => toggleLuz("cuarto2")}
            style={{ backgroundColor: luces.cuarto2 ? "#50fa7b" : "#ff79c6" }}
          >
            Cuarto 2: {luces.cuarto2 ? "Encendida" : "Apagada"}
          </button>
          <button
            onClick={() => toggleLuz("sala")}
            style={{ backgroundColor: luces.sala ? "#50fa7b" : "#ff79c6" }}
          >
            Sala: {luces.sala ? "Encendida" : "Apagada"}
          </button>
          <button
            onClick={() => toggleLuz("comedor")}
            style={{ backgroundColor: luces.comedor ? "#50fa7b" : "#ff79c6" }}
          >
            Comedor: {luces.comedor ? "Encendida" : "Apagada"}
          </button>
          <button
            onClick={() => toggleLuz("cocina")}
            style={{ backgroundColor: luces.cocina ? "#50fa7b" : "#ff79c6" }}
          >
            Cocina: {luces.cocina ? "Encendida" : "Apagada"}
          </button>
        </div>
        <div className="puertas-camara">
          <div className="puertas">
            <h2>Puertas</h2>
            <p data-abierta={puertas.delantera}>
              Puerta Delantera: {puertas.delantera ? "Abierta" : "Cerrada"}
            </p>
            <p data-abierta={puertas.trasera}>
              Puerta Trasera: {puertas.trasera ? "Abierta" : "Cerrada"}
            </p>
            <p data-abierta={puertas.cuarto1}>
              Puerta Cuarto 1: {puertas.cuarto1 ? "Abierta" : "Cerrada"}
            </p>
            <p data-abierta={puertas.cuarto2}>
              Puerta Cuarto 2: {puertas.cuarto2 ? "Abierta" : "Cerrada"}
            </p>
          </div>
          <div className="camera">
            <h2>Cámara</h2>
            <button onClick={tomarFoto}>Tomar foto del jardín</button>
            {foto && <img src={foto} alt="Foto del jardín" className="camera-feed" />}
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;