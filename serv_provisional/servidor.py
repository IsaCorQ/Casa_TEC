from flask import Flask, jsonify, request, send_file
from flask_cors import CORS
import os
from datetime import datetime
from PIL import Image, ImageDraw
import threading
import time
import io  # Para manejar la imagen en memoria

app = Flask(__name__)
CORS(app)  # Habilitar CORS para todas las rutas

# Estado inicial de las luces y puertas
estado_luces = {
    "cuarto1": False,
    "cuarto2": False,
    "sala": False,
    "comedor": False,
    "cocina": False,
}

estado_puertas = {
    "delantera": False,
    "trasera": False,
    "cuarto1": False,
    "cuarto2": False,
}

# Función para cambiar el estado de las puertas una a una
def cambiar_estado_puertas():
    while True:
        for puerta in estado_puertas:
            # Abrir la puerta
            estado_puertas[puerta] = True
            print(f"Puerta {puerta} abierta")  # Log en el servidor
            time.sleep(5)  # Esperar 5 segundos

            # Cerrar la puerta
            estado_puertas[puerta] = False
            print(f"Puerta {puerta} cerrada")  # Log en el servidor
            time.sleep(5)  # Esperar 5 segundos

# Iniciar el hilo para cambiar el estado de las puertas
hilo_puertas = threading.Thread(target=cambiar_estado_puertas)
hilo_puertas.daemon = True  # El hilo se detendrá cuando el servidor se detenga
hilo_puertas.start()

@app.route("/estado_luces", methods=["GET"])
def obtener_estado_luces():
    return jsonify(estado_luces)

@app.route("/encender_luz", methods=["POST"])
def encender_luz():
    luz = request.json.get("luz")
    if luz in estado_luces:
        estado_luces[luz] = True
        print(f"Luz {luz} encendida")  # Log en el servidor
        return jsonify({"mensaje": f"Luz {luz} encendida", "estado": estado_luces})
    return jsonify({"error": "Luz no encontrada"}), 400

@app.route("/apagar_luz", methods=["POST"])
def apagar_luz():
    luz = request.json.get("luz")
    if luz in estado_luces:
        estado_luces[luz] = False
        print(f"Luz {luz} apagada")  # Log en el servidor
        return jsonify({"mensaje": f"Luz {luz} apagada", "estado": estado_luces})
    return jsonify({"error": "Luz no encontrada"}), 400

@app.route("/estado_puertas", methods=["GET"])
def obtener_estado_puertas():
    return jsonify(estado_puertas)

@app.route("/tomar_foto", methods=["GET"])
def tomar_foto():
    try:
        # Crear una imagen simple con Pillow en memoria
        img = Image.new("RGB", (200, 200), color="blue")
        draw = ImageDraw.Draw(img)
        draw.text((50, 80), "Foto simulada", fill="white")

        # Guardar la imagen en un flujo de bytes en memoria
        img_byte_arr = io.BytesIO()
        img.save(img_byte_arr, format="JPEG")
        img_byte_arr.seek(0)  # Reiniciar el puntero del flujo de bytes

        # Enviar la imagen como respuesta sin guardarla en el disco
        return send_file(img_byte_arr, mimetype="image/jpeg")
    except Exception as e:
        print(f"Error al tomar la foto: {e}")
        return jsonify({"error": "No se pudo tomar la foto"}), 500

if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000)