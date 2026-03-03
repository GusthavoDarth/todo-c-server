#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <ctype.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define WWW_FOLDER "www/"
#define MAX_TASKS 100

// Variáveis globais
typedef struct {
    int id;
    char task[256];
} Task;

Task tasks[MAX_TASKS];
int task_count = 0;

// Protótipos das funções (opcional, mas ajuda na organização)
void init_tasks();
void save_tasks();
void load_tasks();
const char* get_mime_type(const char *path);
char* read_file(const char *path, long *file_size);
void handle_client(int client_fd);

// Funções auxiliares
int next_id() {
    int max = 0;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].id > max) max = tasks[i].id;
    }
    return max + 1;
}

void init_tasks() {
    strcpy(tasks[0].task, "Estudar C");
    tasks[0].id = 1;
    strcpy(tasks[1].task, "Fazer projeto");
    tasks[1].id = 2;
    task_count = 2;
}

void save_tasks() {
    FILE *f = fopen("tasks.json", "w");
    if (!f) return;
    fprintf(f, "[");
    for (int i = 0; i < task_count; i++) {
        fprintf(f, "{\"id\":%d,\"task\":\"%s\"}", tasks[i].id, tasks[i].task);
        if (i < task_count - 1) fprintf(f, ",");
    }
    fprintf(f, "]");
    fclose(f);
}

void load_tasks() {
    FILE *f = fopen("tasks.json", "r");
    if (!f) {
        init_tasks();
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *content = (char*)malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    task_count = 0;
    char *p = content;
    while ((p = strstr(p, "{\"id\":")) != NULL && task_count < MAX_TASKS) {
        int id;
        char task[256];
        if (sscanf(p, "{\"id\":%d,\"task\":\"%[^\"]\"}", &id, task) == 2) {
            tasks[task_count].id = id;
            strcpy(tasks[task_count].task, task);
            task_count++;
        }
        p++;
    }
    free(content);
}

// Função para obter o tipo MIME baseado na extensão
const char* get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    return "application/octet-stream";
}

// Função para ler o arquivo e retornar o conteúdo em um buffer alocado
char* read_file(const char *path, long *file_size) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    rewind(file);
    char *buffer = (char*)malloc(*file_size);
    fread(buffer, 1, *file_size, file);
    fclose(file);
    return buffer;
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("read");
        return;
    }
    buffer[bytes_read] = '\0';

    // Parsear a primeira linha da requisição: "GET /path HTTP/1.1"
    char method[10], path[256], protocol[10];
    sscanf(buffer, "%s %s %s", method, path, protocol);

    // Se for uma requisição para a API
    if (strncmp(path, "/api/tasks", 10) == 0) {
        if (strcmp(method, "GET") == 0) {
            // Construir JSON manualmente (simples)
            char response_body[2048] = "[";
            for (int i = 0; i < task_count; i++) {
                char item[512];
                snprintf(item, sizeof(item), "{\"id\":%d,\"task\":\"%s\"}", tasks[i].id, tasks[i].task);
                strcat(response_body, item);
                if (i < task_count - 1) strcat(response_body, ",");
            }
            strcat(response_body, "]");

            char header[256];
            int header_len = snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %ld\r\n"
                "\r\n",
                strlen(response_body));

            send(client_fd, header, header_len, 0);
            send(client_fd, response_body, strlen(response_body), 0);
        }
        else if (strcmp(method, "POST") == 0) {
            // Ler o corpo da requisição (precisamos do Content-Length)
            char *body_start = strstr(buffer, "\r\n\r\n");
            if (body_start) {
                body_start += 4; // pula os \r\n\r\n

                // Parse simples do JSON: esperamos {"task":"alguma coisa"}
                char *task_ptr = strstr(body_start, "\"task\"");
                if (task_ptr) {
                    task_ptr = strchr(task_ptr, ':');
                    if (task_ptr) {
                        task_ptr++;
                        while (*task_ptr == ' ' || *task_ptr == '\"') task_ptr++;
                        char *end = strchr(task_ptr, '\"');
                        if (end) {
                            *end = '\0';
                            if (task_count < MAX_TASKS) {
                                strcpy(tasks[task_count].task, task_ptr);
                                tasks[task_count].id = next_id();
                                task_count++;
                                save_tasks();
                                char response_body[2048] = "[";
                                for (int i = 0; i < task_count; i++) {
                                    char item[512];
                                    snprintf(item, sizeof(item), "{\"id\":%d,\"task\":\"%s\"}", tasks[i].id, tasks[i].task);
                                    strcat(response_body, item);
                                    if (i < task_count - 1) strcat(response_body, ",");
                                }
                                strcat(response_body, "]");
                            }
                        }
                    }
                }
            }
            // Após adicionar, retornar a lista atualizada (mesmo código do GET)
            char response_body[2048] = "[";
            for (int i = 0; i < task_count; i++) {
                char item[512];
                snprintf(item, sizeof(item), "{\"id\":%d,\"task\":\"%s\"}", tasks[i].id, tasks[i].task);
                strcat(response_body, item);
                if (i < task_count - 1) strcat(response_body, ",");
            }
            strcat(response_body, "]");

            char header[256];
            int header_len = snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %ld\r\n"
                "\r\n",
                strlen(response_body));

            send(client_fd, header, header_len, 0);
            send(client_fd, response_body, strlen(response_body), 0);
        }
        else {
            const char *resp = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            send(client_fd, resp, strlen(resp), 0);
        }
        close(client_fd);
        return;
    }

    // Se o caminho for "/", usar "/index.html"
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/index.html");
    }

    // Construir caminho completo do arquivo
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s%s", WWW_FOLDER, path + 1); // +1 para pular a barra

    // Verificar se o arquivo existe
    struct stat st;
    if (stat(full_path, &st) == 0) {
        // Arquivo existe
        long file_size;
        char *file_content = read_file(full_path, &file_size);
        if (file_content) {
            const char *mime = get_mime_type(full_path);
            char header[256];
            int header_len = snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %ld\r\n"
                "\r\n",
                mime, file_size);

            send(client_fd, header, header_len, 0);
            send(client_fd, file_content, file_size, 0);
            free(file_content);
        } else {
            // Erro ao ler o arquivo
            const char *resp = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            send(client_fd, resp, strlen(resp), 0);
        }
    } else {
        // Arquivo não encontrado
        const char *not_found =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "<h1>404 Not Found</h1>";
        send(client_fd, not_found, strlen(not_found), 0);
    }
    close(client_fd);
}

int main() {
    load_tasks();
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor rodando em http://localhost:%d\n", PORT);

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        handle_client(client_fd);
    }

    close(server_fd);
    return 0;
}
