#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gpio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h> //AGREGAR ESTA LIBRERIA


#define PORT 5000
#define BUFFER_SIZE 4096
#define NUM_PUERTAS 5
#define NUM_LUCES 6

// Estructuras de estado
typedef struct {
    bool cuarto;
    bool oficina;
    bool sala;
    bool patio;
    bool cocina;
    bool bano;
} Luces;

typedef struct {
    bool delantera;
    bool trasera;
    bool cuarto;
    bool oficina;
    bool bano;
} Puertas;

// Variables globales con mutexes
Luces estado_luces = {false};
Puertas estado_puertas = {false};
pthread_mutex_t luces_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t puertas_mutex = PTHREAD_MUTEX_INITIALIZER;

int pins_puerta[NUM_PUERTAS] = {514,515,516,529,534};
int pins_luz[NUM_LUCES] = {519,513,524,528,532,533};

static void initialize_pins(){
	
	//Luces
	pinMode(519, OUTPUT); //cuarto
	pinMode(513, OUTPUT); //oficina
	pinMode(524, OUTPUT); //sala
	pinMode(528, OUTPUT); //patio
	pinMode(532, OUTPUT); //cocina
    pinMode(533, OUTPUT); //bano
	
	//Puertas
	pinMode(514, INPUT); //delantera
	pinMode(515, INPUT); //trasera
	pinMode(516, INPUT); //cuarto
	pinMode(529, INPUT); //oficina
    pinMode(534, INPUT); //bano
}

// Headers comunes
const char* headers = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
    "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
    "Connection: close\r\n\r\n";

const char* STORED_HASH = "5aa683681ce6bf58eb76d57d904420036f71af99f6d1e7da875b1f9e3413a392";

void send_error(int socket, int code, const char* message) {
    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %d Error\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n"
        "{\"error\": \"%s\"}",
        code, message);
    
    write(socket, response, strlen(response));
}

int base64_decode(const char* input, char** output) {
    BIO *bio, *b64;
    size_t len = strlen(input);
    *output = (char*)malloc(len);
    if (!*output) return -1;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new_mem_buf((void*)input, len);
    bio = BIO_push(b64, bio);

    int decoded_len = BIO_read(bio, *output, len);
    BIO_free_all(bio);
    
    if (decoded_len <= 0) {
        free(*output);
        *output = NULL;
        return -1;
    }
    (*output)[decoded_len] = '\0';
    return decoded_len;
}


// AGREGAR ESTA FUNCION:

char* codificar_imagen(const char* directorio, size_t* encoded_len) {
    FILE* fp = fopen(directorio, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    unsigned char* image_data = malloc(file_size);
    if (!image_data) {
        fclose(fp);
        return NULL;
    }

    fread(image_data, 1, file_size, fp);
    fclose(fp);

    BIO *bio, *b64;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, image_data, file_size);
    BIO_flush(bio);
    
    memset(image_data, 0, file_size);
    free(image_data);
    image_data = NULL;

    BUF_MEM *bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    char* encoded = malloc(bufferPtr->length + 1);
    memcpy(encoded, bufferPtr->data, bufferPtr->length);
    encoded[bufferPtr->length] = '\0';
    *encoded_len = bufferPtr->length;

    BIO_free_all(bio);
    return encoded;
}

bool validar_autenticacion(char* buffer) {
    char* auth_header = strstr(buffer, "Authorization: Basic ");
    if (!auth_header) return false;

    char* token_start = auth_header + strlen("Authorization: Basic ");
    char* token_end = strstr(token_start, "\r\n");
    if (!token_end) return false;

    size_t token_len = token_end - token_start;
    char* base64_token = malloc(token_len + 1);
    memcpy(base64_token, token_start, token_len);
    base64_token[token_len] = '\0';

    char* decoded_token = NULL;
    int decoded_len = base64_decode(base64_token, &decoded_token);
    free(base64_token);

    if (decoded_len <= 0 || !decoded_token) {
        if (decoded_token) free(decoded_token);
        return false;
    }

    char* colon = strchr(decoded_token, ':');
    if (!colon) {
        free(decoded_token);
        return false;
    }

    *colon = '\0';
    char* username = decoded_token;
    char* password = colon + 1;

    if (strcmp(username, "admin") != 0) {
        free(decoded_token);
        return false;
    }

    // Calcular HMAC-SHA256 con la secret key
    unsigned char hmac_result[32];
    HMAC(
        EVP_sha256(),
        "beticomijefecito", 16,  // Secret key y su longitud
        (unsigned char*)password, strlen(password),
        hmac_result, NULL
    );

    // Convertir a hexadecimal
    char hmac_hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(hmac_hex + (i*2), "%02x", hmac_result[i]);
    }
    hmac_hex[64] = '\0';

    bool valido = (strcmp(hmac_hex, STORED_HASH) == 0);
    free(decoded_token);
    return valido;
}

char* generar_json_luces() {
    pthread_mutex_lock(&luces_mutex);
    char* json = malloc(256);
    snprintf(json, 256, 
        "{\"cuarto\":%s, \"oficina\":%s, \"sala\":%s, \"patio\":%s, \"cocina\":%s, \"bano\":%s}",
        estado_luces.cuarto ? "true" : "false",
        estado_luces.oficina ? "true" : "false",
        estado_luces.sala ? "true" : "false",
        estado_luces.patio ? "true" : "false",
        estado_luces.cocina ? "true" : "false",
        estado_luces.bano ? "true" : "false");
    pthread_mutex_unlock(&luces_mutex);
    return json;
}

char* generar_json_puertas() {
    pthread_mutex_lock(&puertas_mutex);
    char* json = malloc(256);
    snprintf(json, 256,
        "{\"delantera\":%s, \"trasera\":%s, \"cuarto\":%s, \"oficina\":%s, \"bano\":%s}",
        estado_puertas.delantera ? "true" : "false",
        estado_puertas.trasera ? "true" : "false",
        estado_puertas.cuarto ? "true" : "false",
        estado_puertas.oficina ? "true" : "false",
        estado_puertas.bano ? "true" : "false");
    pthread_mutex_unlock(&puertas_mutex);
    return json;
}

// AQUI SE HACE LOS CAMBIOS DE LA PUERTA
void* cambiar_puertas() {
    bool cambio_puerta_d;
    bool cambio_puerta_t;
    bool cambio_puerta_c;
    bool cambio_puerta_o;
    bool cambio_puerta_b;
    cambio_puerta_d = digitalRead(514);
    estado_puertas.delantera = cambio_puerta_d;

    cambio_puerta_t = digitalRead(515);
    estado_puertas.trasera = cambio_puerta_t;

    cambio_puerta_c = digitalRead(516);
    estado_puertas.cuarto = cambio_puerta_c;

    cambio_puerta_o = digitalRead(529);
    estado_puertas.oficina = cambio_puerta_o;

    cambio_puerta_b = digitalRead(534);
    estado_puertas.bano = cambio_puerta_b;
    return NULL;
}

void* manejar_cliente(void* socket_ptr) {
    int socket = *(int*)socket_ptr;
    char buffer[BUFFER_SIZE];
    read(socket, buffer, BUFFER_SIZE);
    
    // Parsear solicitud
    char metodo[16], ruta[256];
    sscanf(buffer, "%s %s", metodo, ruta);
    
    printf("Solicitud: %s %s\n", metodo, ruta);

    // Manejar OPTIONS (CORS preflight)
    if (strcmp(metodo, "OPTIONS") == 0) {
        write(socket, headers, strlen(headers));
        close(socket);
        free(socket_ptr);
        return NULL;
    }

    // Validar autenticación para otros métodos
    if (!validar_autenticacion(buffer)) {
        send_error(socket, 401, "Acceso no autorizado");
        close(socket);
        free(socket_ptr);
        return NULL;
    }

    // Manejar rutas
    if(strcmp(ruta, "/estado_luces") == 0) {
        char* json = generar_json_luces();
        write(socket, headers, strlen(headers));
        write(socket, json, strlen(json));
        free(json);
    }
    else if(strcmp(ruta, "/estado_puertas") == 0) {
        cambiar_puertas();
        char* json = generar_json_puertas();
        write(socket, headers, strlen(headers));
        write(socket, json, strlen(json));
        free(json);
    }
    else if(strcmp(ruta, "/tomar_foto") == 0) {
        char* json = generar_json_puertas();
        write(socket, headers, strlen(headers));
        write(socket, json, strlen(json));
        free(json);
    }
    // AGREGAR ESTE ELSE IF
    else if(strcmp(ruta, "/tomar_foto") == 0) {
        const char* directorio_imagen = "/home/dylanggf/Documents/Empotrados/Casa_TEC/serv_provisional/image.jpg"; // Cambiar por tu directorio
        
        size_t encoded_len;
        char* encoded_data = codificar_imagen(directorio_imagen, &encoded_len);
        
        if (!encoded_data) {
            send_error(socket, 500, "Error al procesar la imagen");
            close(socket);
            free(socket_ptr);
            return NULL;
        }
    
        // Construir respuesta
        char header[512];
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-Transfer-Encoding: base64\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %zu\r\n\r\n",
            encoded_len);
    
        write(socket, header, strlen(header));
        write(socket, encoded_data, encoded_len);
    
        // Limpieza segura
        memset(encoded_data, 0, encoded_len);
        free(encoded_data);
        encoded_data = NULL;
    }


    else if(strcmp(ruta, "/encender_luz") == 0 || strcmp(ruta, "/apagar_luz") == 0) {
        char* cuerpo = strstr(buffer, "\r\n\r\n");
        if (!cuerpo) {
            send_error(socket, 400, "Solicitud sin cuerpo");
            close(socket);
            free(socket_ptr);
            return NULL;
        }
        cuerpo += 4;

        char valor[32] = {0};
        if (sscanf(cuerpo, "{\"luz\":\"%31[^\"]", valor) != 1) {
            send_error(socket, 400, "Formato JSON inválido");
            close(socket);
            free(socket_ptr);
            return NULL;
        }

        bool nuevo_estado = (strcmp(ruta, "/encender_luz") == 0);
        pthread_mutex_lock(&luces_mutex);
        
        // AQUI SE HACE LO DE LAS LUCES
        if(strcmp(valor, "cuarto") == 0) {
            estado_luces.cuarto = nuevo_estado;
            digitalWrite(519, nuevo_estado);
        }
        else if(strcmp(valor, "oficina") == 0) {
            estado_luces.oficina = nuevo_estado;
            digitalWrite(513, nuevo_estado);
        }
        else if(strcmp(valor, "sala") == 0) {
            estado_luces.sala = nuevo_estado;
            digitalWrite(524, nuevo_estado);
        }
        else if(strcmp(valor, "patio") == 0) {
            estado_luces.patio = nuevo_estado;
            digitalWrite(528, nuevo_estado);
        }
        else if(strcmp(valor, "cocina") == 0) {
            estado_luces.cocina = nuevo_estado;
            digitalWrite(532, nuevo_estado);
        }
        else if(strcmp(valor, "bano") == 0) {
            estado_luces.bano = nuevo_estado;
            digitalWrite(533, nuevo_estado);
        }
        else {
            pthread_mutex_unlock(&luces_mutex);
            send_error(socket, 400, "Luz no encontrada");
            close(socket);
            free(socket_ptr);
            return NULL;
        }
        
        pthread_mutex_unlock(&luces_mutex);
        printf("Luz %s cambiada a: %s\n", valor, nuevo_estado ? "ON" : "OFF");

        char* json = generar_json_luces();
        char respuesta[512];
        snprintf(respuesta, sizeof(respuesta), "%s{\"mensaje\":\"Luz %s %s\", \"estado\":%s}",
                headers, valor, nuevo_estado ? "encendida" : "apagada", json);
        
        write(socket, respuesta, strlen(respuesta));
        free(json);
    }
    else {
        send_error(socket, 404, "Ruta no encontrada");
    }

    close(socket);
    free(socket_ptr);
    return NULL;
}

int main() {
    initialize_pins();
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if(listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Servidor escuchando en puerto %d...\n", PORT);
    
    cambiar_puertas();
    
    while(1) {
        int* new_socket = malloc(sizeof(int));
        *new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        
        if (*new_socket < 0) {
            perror("accept");
            free(new_socket);
            continue;
        }
        
        pthread_t hilo;
        pthread_create(&hilo, NULL, manejar_cliente, new_socket);
        pthread_detach(hilo);
    }
    
    return 0;
}