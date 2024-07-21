#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <getopt.h>

#define MAX_REDIRECTS 10
#define MAX_BUFFER_SIZE 4096
#define MAX_HOST_SIZE 256
#define MAX_PATH_SIZE 256
#define MAX_USERAGENT_SIZE 256

#define CHAT_CIANO "\033[36m"
#define CHAT_VERMELHO "\033[31m"
#define CHAT_PADRAO "\033[0m"

int max_threads = 1;
int verbose = 0;

typedef struct {
    char host[MAX_HOST_SIZE];
    char path[MAX_PATH_SIZE];
    char useragent[MAX_USERAGENT_SIZE];
} ThreadArgs;

void banner(void) {
    printf("\n==========================================================================\n\n");
    printf("    ______         __           ______\n");
    printf("    |   __ \\.--.--.|  |_ .-----.|   __ \\.-----..----..-----..-----.\n");
    printf("    |   __ <|  |  ||   _||  -__||      <|  -__||  __||  _  ||     |\n");
    printf("    |______/|___  ||____||_____||___|__||_____||____||_____||__|__|\n");
    printf("            |_____|\n");
    printf("                	by %sSirProxy%s - v1.0.0\n", CHAT_CIANO, CHAT_PADRAO);
    printf("               %shttps://github.com/SirProxy/byterecon%s\n\n", CHAT_CIANO, CHAT_PADRAO);
    printf("[%s!%s] Disclaimer: This tool was created for educational purposes only.\n", CHAT_CIANO, CHAT_PADRAO);
    printf("[%s!%s] Disclaimer: The author is not responsible for any misuse or illegal activities conducted using this tool.\n", CHAT_CIANO, CHAT_PADRAO);
    printf("\n==========================================================================\n");
    printf("\n");
}

void help(void) {
    printf("[%s?%s] Use: ./byterecon --domain=example.com --wordlist=wordlist.txt\n", CHAT_CIANO, CHAT_PADRAO);
    printf("[%s?%s] Example: ./byterecon --domain=example.com --wordlist=wordlist.txt --useragent='Mozilla/5.0' --threads=10\n", CHAT_CIANO, CHAT_PADRAO);
}

void commands(void) {
    printf("\n");
    printf("[%s?%s] %s--domain=%s    - Target domain\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%s?%s] %s--wordlist=%s  - list of words that will be used in the test\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%s?%s] %s--useragent=%s - Custom useragent\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%s?%s] %s--threads=%s   - Number of threads that the tool will use. Default value is 10 threads\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%s?%s] %s--verbose=%s   - Detailed view\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("\n==========================================================================");
}

void verificarArquivoNaURL(const char *host, const char *path, const char *useragent) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char request[1024];
    char response[MAX_BUFFER_SIZE];
    int redirects = 0;
    char final_path[MAX_PATH_SIZE];
    char *location;

    snprintf(final_path, sizeof(final_path), "%s", path);

    while (redirects < MAX_REDIRECTS) {
        // Criar socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Erro ao abrir o socket");
            return;
        }

        // Obter endereço do servidor
        server = gethostbyname(host);
        if (server == NULL) {
            fprintf(stderr, "Erro: não foi possível encontrar o host\n");
            close(sockfd);
            return;
        }

        // Configurar endereço do servidor
        bzero((char *) &server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
        server_addr.sin_port = htons(80);

        // Conectar ao servidor
        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Erro ao conectar");
            close(sockfd);
            return;
        }

        // Construir requisição HTTP
        if (strlen(useragent) > 0) {
            snprintf(request, sizeof(request),
                     "HEAD %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "User-Agent: %s\r\n"
                     "Connection: close\r\n"
                     "\r\n", final_path, host, useragent);
        } else {
            snprintf(request, sizeof(request),
                     "HEAD %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "User-Agent: ByteRecon v1.0.0\r\n"
                     "Connection: close\r\n"
                     "\r\n", final_path, host);
        }

        // Enviar requisição
        if (write(sockfd, request, strlen(request)) < 0) {
            perror("Erro ao enviar a requisição");
            close(sockfd);
            return;
        }

        // Ler resposta
        bzero(response, sizeof(response));
        ssize_t bytes_read = read(sockfd, response, sizeof(response) - 1);
        if (bytes_read < 0) {
            perror("Erro ao ler a resposta");
            close(sockfd);
            return;
        }
        response[bytes_read] = '\0'; // Garantir que a resposta é uma string C válida

        // Verificar código de resposta HTTP
        int status_code = 0;
        sscanf(response, "HTTP/1.1 %d", &status_code);

        if (status_code == 200) {
            printf("[%s+%s] Found -> %s%s%s%s (Código HTTP: %d)\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, host, final_path, CHAT_PADRAO, status_code);
        } else if (status_code == 301 || status_code == 302) {
            // Obter o novo local do redirecionamento
            location = strstr(response, "Location:");
            if (location != NULL) {
                location += strlen("Location:");
                while (*location == ' ') location++;
                snprintf(final_path, sizeof(final_path), "%s", location);
                redirects++;
                close(sockfd);
                continue;
            } else {
                printf("[%s!%s] Redirection with no URL found\n", CHAT_VERMELHO, CHAT_PADRAO);
            }
        } else {
            if (verbose == 1) {
                printf("[%s-%s] Not Found -> %s%s%s%s (Código HTTP: %d)\n", CHAT_VERMELHO, CHAT_PADRAO, CHAT_VERMELHO, host, final_path, CHAT_PADRAO, status_code);
            }
        }
        close(sockfd);
        return;
    }

    if (redirects >= MAX_REDIRECTS) {
        printf("[%s!%s] Maximum number of redirects reached\n", CHAT_VERMELHO, CHAT_PADRAO);
    }
}

void *worker(void *args) {
    if (args == NULL) {
        fprintf(stderr, "Erro: argumentos da thread são nulos\n");
        pthread_exit(NULL);
    }

    ThreadArgs *thread_args = (ThreadArgs *)args;
    verificarArquivoNaURL(thread_args->host, thread_args->path, thread_args->useragent);
    free(args); // Liberar memória alocada para os argumentos da thread
    return NULL;
}


void start(const char *domain, const char *wordlist, const char *useragent) {
    FILE *arquivo = fopen(wordlist, "r");
    if (arquivo == NULL) {
        printf("[%s!%s] Wordlist %s%s%s not found!\n", CHAT_VERMELHO, CHAT_PADRAO, CHAT_VERMELHO, wordlist, CHAT_PADRAO);
        printf("[%s!%s] Your search has stopped\n", CHAT_VERMELHO, CHAT_PADRAO);
        exit(EXIT_FAILURE);
    }

    printf("[%s*%s] Base Domain: %s%s%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, domain, CHAT_PADRAO);
    printf("[%s*%s] Wordlist: %s%s%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, wordlist, CHAT_PADRAO);
    printf("[%s*%s] Max Follow Redirects: %s%d%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, MAX_REDIRECTS, CHAT_PADRAO);

    if (verbose == 0) {
        printf("[%s*%s] Verbose: %sFalse%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    } else {
        printf("[%s*%s] Verbose: %sTrue%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    }

    if (useragent != NULL) {
        printf("[%s*%s] Custom User-Agent: %s%s%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, useragent, CHAT_PADRAO);
    }

    printf("\n==========================================================================\n");
    printf("[%s*%s] Starting analize...\n\n", CHAT_CIANO, CHAT_PADRAO);

    char linha[256];
    pthread_t threads[max_threads];
    int thread_count = 0;

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        linha[strcspn(linha, "\n")] = 0; // Remover nova linha

        char url[512];
        snprintf(url, sizeof(url), "%s/%s", domain, linha);

        char *path = strchr(url, '/');
        if (path) {
            *path = '\0';
            path++;
        } else {
            path = "";
        }

        ThreadArgs* args = malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            perror("Erro ao alocar memória");
            exit(EXIT_FAILURE);
        }

        strncpy(args->host, url, sizeof(args->host) - 1);
        args->host[sizeof(args->host) - 1] = '\0';
        snprintf(args->path, sizeof(args->path), "/%s", path);
        strncpy(args->useragent, useragent ? useragent : "", sizeof(args->useragent) - 1);
        args->useragent[sizeof(args->useragent) - 1] = '\0';

        if (pthread_create(&threads[thread_count++], NULL, worker, args) != 0) {
            perror("Erro ao criar thread");
            free(args);
            continue;
        }

        if (thread_count >= max_threads) {
            for (int i = 0; i < max_threads; i++) {
                pthread_join(threads[i], NULL);
            }
            thread_count = 0;
        }
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    fclose(arquivo);

    printf("\n[%s*%s] Analysis completed", CHAT_CIANO, CHAT_PADRAO);
    printf("\n==========================================================================\n");

}

int main(int argc, char **argv) {
    banner();

    const char *domain = NULL;
    const char *wordlist = NULL;
    const char *useragent = NULL;

    static struct option long_options[] = {
        {"domain", required_argument, 0, 'd'},
        {"wordlist", required_argument, 0, 'w'},
        {"useragent", optional_argument, 0, 'u'},
        {"threads", optional_argument, 0, 't'},
        {"help", no_argument, 0, 'h'},
        {"verbose", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "d:w:u:t:hv", long_options, &option_index)) != -1) {
        switch (c) {
            case 'd':
                domain = optarg;
                break;
            case 'w':
                wordlist = optarg;
                break;
            case 'u':
                useragent = optarg;
                break;
            case 't':
                max_threads = atoi(optarg);
                if (max_threads <= 0) {
                    max_threads = 1; // Valor padrão se inválido
                }
                break;
            case 'h':
                help();
                commands();
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                help();
                printf("\n[%s?%s] You can use %s--help%s to see a parameter guide.\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
                printf("\n==========================================================================\n");
                exit(EXIT_FAILURE);
        }
    }

    if (!domain || !wordlist) {
        help();
        printf("\n[%s?%s] You can use %s--help%s to see a parameter guide.\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
        printf("\n==========================================================================\n");
        exit(EXIT_FAILURE);
    }

    start(domain, wordlist, useragent);

    return 0;
}
