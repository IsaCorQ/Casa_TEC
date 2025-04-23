#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/evp.h>


#define PORT 5000
#define BUFFER_SIZE 4096

// Estructuras de estado
typedef struct {
    bool cuarto1;
    bool cuarto2;
    bool sala;
    bool comedor;
    bool cocina;
} Luces;

typedef struct {
    bool delantera;
    bool trasera;
    bool cuarto1;
    bool cuarto2;
} Puertas;

// Variables globales con mutexes
Luces estado_luces = {false};
Puertas estado_puertas = {false};
pthread_mutex_t luces_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t puertas_mutex = PTHREAD_MUTEX_INITIALIZER;

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
        "{\"cuarto1\":%s, \"cuarto2\":%s, \"sala\":%s, \"comedor\":%s, \"cocina\":%s}",
        estado_luces.cuarto1 ? "true" : "false",
        estado_luces.cuarto2 ? "true" : "false",
        estado_luces.sala ? "true" : "false",
        estado_luces.comedor ? "true" : "false",
        estado_luces.cocina ? "true" : "false");
    pthread_mutex_unlock(&luces_mutex);
    return json;
}

char* generar_json_puertas() {
    pthread_mutex_lock(&puertas_mutex);
    char* json = malloc(256);
    snprintf(json, 256,
        "{\"delantera\":%s, \"trasera\":%s, \"cuarto1\":%s, \"cuarto2\":%s}",
        estado_puertas.delantera ? "true" : "false",
        estado_puertas.trasera ? "true" : "false",
        estado_puertas.cuarto1 ? "true" : "false",
        estado_puertas.cuarto2 ? "true" : "false");
    pthread_mutex_unlock(&puertas_mutex);
    return json;
}

// AQUI SE HACE LOS CAMBIOS DE LA PUERTA
void* cambiar_puertas(void* arg) {
    const char* puertas[] = {"delantera", "trasera", "cuarto1", "cuarto2"};
    
    while(1) {
        for(int i = 0; i < 4; i++) {
            sleep(5);
            pthread_mutex_lock(&puertas_mutex);
            if(strcmp(puertas[i], "delantera") == 0) estado_puertas.delantera = true;
            else if(strcmp(puertas[i], "trasera") == 0) estado_puertas.trasera = true;
            else if(strcmp(puertas[i], "cuarto1") == 0) estado_puertas.cuarto1 = true;
            else if(strcmp(puertas[i], "cuarto2") == 0) estado_puertas.cuarto2 = true;
            pthread_mutex_unlock(&puertas_mutex);
            printf("Puerta %s abierta\n", puertas[i]);
            
            sleep(5);
            pthread_mutex_lock(&puertas_mutex);
            if(strcmp(puertas[i], "delantera") == 0) estado_puertas.delantera = false;
            else if(strcmp(puertas[i], "trasera") == 0) estado_puertas.trasera = false;
            else if(strcmp(puertas[i], "cuarto1") == 0) estado_puertas.cuarto1 = false;
            else if(strcmp(puertas[i], "cuarto2") == 0) estado_puertas.cuarto2 = false;
            pthread_mutex_unlock(&puertas_mutex);
            printf("Puerta %s cerrada\n", puertas[i]);
        }
    }
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
        char* json = generar_json_puertas();
        write(socket, headers, strlen(headers));
        write(socket, json, strlen(json));
        free(json);
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
        if(strcmp(valor, "cuarto1") == 0) estado_luces.cuarto1 = nuevo_estado;
        else if(strcmp(valor, "cuarto2") == 0) estado_luces.cuarto2 = nuevo_estado;
        else if(strcmp(valor, "sala") == 0) estado_luces.sala = nuevo_estado;
        else if(strcmp(valor, "comedor") == 0) estado_luces.comedor = nuevo_estado;
        else if(strcmp(valor, "cocina") == 0) estado_luces.cocina = nuevo_estado;
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
    
    pthread_t hilo_puertas;
    pthread_create(&hilo_puertas, NULL, cambiar_puertas, NULL);
    
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