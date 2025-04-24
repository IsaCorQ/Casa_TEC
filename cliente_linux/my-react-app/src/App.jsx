import React, { useState, useEffect } from "react";
import "./App.css";

function App() {
  const [isLoggedIn, setIsLoggedIn] = useState(false);
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [authError, setAuthError] = useState("");
  const [authHeader, setAuthHeader] = useState("");
  const [credentials, setCredentials] = useState(null); // Almacenar credenciales
  const [lastUpdate, setLastUpdate] = useState(Date.now());

  
  const [luces, setLuces] = useState({/*...*/});
  const [puertas, setPuertas] = useState({/*...*/});
  const [foto, setFoto] = useState(null);

  // Función para manejar el login
  const handleLogin = async (e) => {
    e.preventDefault();
    try {
      // Generar token temporal para la verificación
      const token = btoa(`${username}:${password}`);
      
      // Verificar credenciales con una solicitud
      const testResponse = await fetch("http://10.42.0.58:5000/estado_luces", {
        headers: { Authorization: `Basic ${token}` }
      });
      
      if (!testResponse.ok) throw new Error("Credenciales inválidas");
      
      // Si es válido, guardar credenciales en base64
      setCredentials(token);
      setIsLoggedIn(true);
      setAuthError("");
      
    } catch (error) {
      setAuthError(error.message);
      setIsLoggedIn(false);
    }
  };

  // Función de fetch protegida
  const authFetch = async (url, options = {}) => {
    const headers = {
      ...options.headers,
      Authorization: `Basic ${credentials}`
    };
    
    const response = await fetch(url, { ...options, headers });
    
    if (!response.ok) {
      throw new Error(`Error HTTP: ${response.status}`);
    }
    
    return response;
  };

  const toggleLuz = async (luz) => {
    try {
      const endpoint = luces[luz] ? "/apagar_luz" : "/encender_luz";
      const response = await authFetch(`http://10.42.0.58:5000${endpoint}`, {
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

const obtenerEstadoPuertas = async () => {
  try {
    const response = await authFetch("http://10.42.0.58:5000/estado_puertas");
    const data = await response.json();
    setPuertas(data);
  } catch (error) {
    console.error("Error obteniendo puertas:", error.message);
  }
};

  // Función para actualizar el estado de una puerta
  const actualizarPuerta = async (puerta, estado) => {
    try {
      await fetch("http://10.42.0.58:5000/actualizar_puerta", {
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
      const response = await fetch("http://10.42.0.58:5000/tomar_foto");
      const blob = await response.blob();
      setFoto(URL.createObjectURL(blob));
    } catch (error) {
      console.error("Error tomando la foto:", error);
    }
  };

  // Añadir función de logout
  const handleLogout = () => {
    setIsLoggedIn(false);
    setAuthHeader("");
    setUsername("");
    setPassword("");
  };

  // Actualizar el efecto para cargar datos iniciales
  useEffect(() => {
    if (isLoggedIn) {
      const fetchUpdates = async () => {
        try {
          const [lucesRes, puertasRes] = await Promise.all([
            authFetch(`http://10.42.0.58:5000/estado_luces`),
            authFetch(`http://10.42.0.58:5000/estado_puertas`)
          ]);
          
          if (lucesRes.status === 304 && puertasRes.status === 304) return;
          
          const lucesData = await lucesRes.json();
          const puertasData = await puertasRes.json();
          
          setLuces(prev => ({ ...prev, ...lucesData }));
          setPuertas(prev => ({ ...prev, ...puertasData }));
          setLastUpdate(Date.now());
        } catch (error) {
          console.error('Error actualizando estados:', error);
        }
      };
  
      const interval = setInterval(fetchUpdates, 300);
      return () => clearInterval(interval);
    }
  }, [isLoggedIn]);

  // Render condicional
  if (!isLoggedIn) {
    return (
      <div className="login-container">
        <h2>Autenticación Requerida</h2>
        <form onSubmit={handleLogin} className="login-form">
          <input
            type="text"
            placeholder="Usuario"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            required
          />
          <input
            type="password"
            placeholder="Contraseña"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            required
          />
          <button type="submit">Ingresar</button>
          {authError && <p className="error-message">{authError}</p>}
        </form>
      </div>
    );
  }

  return (
    <div className="App">
      <div className="header-bar">
        <button onClick={handleLogout} className="logout-button">
          Cerrar Sesión
        </button>
      </div>
      <h1>Control y Monitoreo de Casa Inteligente</h1>
      <div className="dashboard">
        <div className="luces">
          <h2>Luces</h2>
          <button
            onClick={() => toggleLuz("cuarto")}
            style={{ backgroundColor: luces.cuarto ? "#50fa7b" : "#ff79c6" }}
          >
            Cuarto 1: {luces.cuarto ? "Encendida" : "Apagada"}
          </button>
          <button
            onClick={() => toggleLuz("oficina")}
            style={{ backgroundColor: luces.oficina ? "#50fa7b" : "#ff79c6" }}
          >
            Cuarto 2: {luces.oficina ? "Encendida" : "Apagada"}
          </button>
          <button
            onClick={() => toggleLuz("sala")}
            style={{ backgroundColor: luces.sala ? "#50fa7b" : "#ff79c6" }}
          >
            Sala: {luces.sala ? "Encendida" : "Apagada"}
          </button>
          <button
            onClick={() => toggleLuz("patio")}
            style={{ backgroundColor: luces.patio ? "#50fa7b" : "#ff79c6" }}
          >
            Comedor: {luces.patio ? "Encendida" : "Apagada"}
          </button>
          <button
            onClick={() => toggleLuz("cocina")}
            style={{ backgroundColor: luces.cocina ? "#50fa7b" : "#ff79c6" }}
          >
            Cocina: {luces.cocina ? "Encendida" : "Apagada"}
          </button>
          <button
            onClick={() => toggleLuz("bano")}
            style={{ backgroundColor: luces.bano ? "#50fa7b" : "#ff79c6" }}
          >
            Baño: {luces.bano ? "Encendida" : "Apagada"}
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
            <p data-abierta={puertas.cuarto}>
              Puerta Cuarto 1: {puertas.cuarto ? "Abierta" : "Cerrada"}
            </p>
            <p data-abierta={puertas.oficina}>
              Puerta Cuarto 2: {puertas.oficina ? "Abierta" : "Cerrada"}
            </p>
            <p data-abierta={puertas.bano}>
              Puerta Delantera: {puertas.bano ? "Abierta" : "Cerrada"}
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