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
int ignore404 = 0;
char *useragent = "ByteRecon v1.1.0";
char *tipo = "dir";

typedef struct {
    char host[MAX_HOST_SIZE];
    char path[MAX_PATH_SIZE];
    char useragent[MAX_USERAGENT_SIZE];
} ThreadArgs;

void banner(void) {
    printf("\n================================================================================================\n\n");
    printf("                ______         __           ______\n");
    printf("                |   __ \\.--.--.|  |_ .-----.|   __ \\.-----..----..-----..-----.\n");
    printf("                |   __ <|  |  ||   _||  -__||      <|  -__||  __||  _  ||     |\n");
    printf("                |______/|___  ||____||_____||___|__||_____||____||_____||__|__|\n");
    printf("                        |_____|\n");
    printf("                                   by %sSirProxy%s - v1.1.0\n", CHAT_CIANO, CHAT_PADRAO);
    printf("                           %shttps://github.com/SirProxy/byterecon%s\n\n", CHAT_CIANO, CHAT_PADRAO);
    printf("[%s!%s] Disclaimer: This tool was created for educational purposes only.\n", CHAT_CIANO, CHAT_PADRAO);
    printf("[%s!%s] Disclaimer: The author is not responsible for any misuse or illegal activities conducted using this tool.\n", CHAT_CIANO, CHAT_PADRAO);
    printf("\n================================================================================================\n");
    printf("\n");
}

void help(void) {
    printf("[%s?%s] Use: %s./byterecon --domain=example.com --wordlist=wordlist.txt%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%s?%s] Example: %s./byterecon --domain=example.com --wordlist=wordlist.txt --useragent='Mozilla/5.0' --threads=10%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
}

void commands(void) {
    printf("\n");
    printf("[%s?%s] %s--domain=%s       -> Target domain (Required)\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%s?%s] %s--wordlist=%s     -> list of words that will be used in the test (Required)\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%s?%s] %s--useragent=%s    -> Custom useragent\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%s?%s] %s--threads=%s      -> Number of threads that the tool will use. Default value is 1 thread\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    
    printf("[%s?%s] %s--type=%s         -> Enumeration type\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%si%s] %s       dir%s      -> Directory enumeration\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%si%s] %s       sub%s      -> Subdomain enumeration\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%si%s] %s       all%s      -> All enumeration types\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);

    printf("[%s?%s] %s--verbose%s       -> Detailed view\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("[%s?%s] %s--ignore404%s     -> Ignore 404 status code\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    printf("\n================================================================================================\n");
}

void enumDir(const char *host, const char *path, const char *useragent) {

    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char request[1024];
    char response[MAX_BUFFER_SIZE];
    int redirects = 0;
    char final_path[MAX_PATH_SIZE];
    char *location;
    char new_host[MAX_PATH_SIZE];
    char new_path[MAX_PATH_SIZE];

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
        snprintf(request, sizeof(request),
                    "HEAD %s HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "User-Agent: %s\r\n"
                    "Connection: close\r\n"
                    "\r\n", final_path, host, useragent);

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
            printf("[%s+%s] (HTTP: %d) Found -----> %shttp://%s%s%s\n", CHAT_CIANO, CHAT_PADRAO, status_code, CHAT_CIANO, host, final_path, CHAT_PADRAO);
        } else if (status_code == 301 || status_code == 302) {
            // Obter o novo local do redirecionamento
            location = strstr(response, "Location:");
            if (location != NULL) {
                location += strlen("Location:");
                while (*location == ' ') location++;
                char *end_location = strstr(location, "\r\n");
                if (end_location != NULL) {
                    *end_location = '\0'; // Terminar a string no final do cabeçalho Location
                }

                if (strncmp(location, "http://", 7) == 0 || strncmp(location, "https://", 8) == 0) {
                    // Extrair novo host e path da URL completa
                    char *start_path = strstr(location, "://") + 3;
                    char *slash_pos = strchr(start_path, '/');
                    if (slash_pos != NULL) {
                        *slash_pos = '\0';
                        snprintf(new_host, sizeof(new_host), "%s", start_path);
                        snprintf(new_path, sizeof(new_path), "%s", slash_pos + 1);
                    } else {
                        snprintf(new_host, sizeof(new_host), "%s", start_path);
                        snprintf(new_path, sizeof(new_path), "/");
                    }
                } else {
                    // Caso a URL não seja completa, apenas uma path relativa
                    snprintf(new_host, sizeof(new_host), "%s", host);
                    snprintf(new_path, sizeof(new_path), "%s", location);
                }

                // Atualizar host e path para a nova URL redirecionada
                snprintf(final_path, sizeof(final_path), "/%s", new_path);
                snprintf((char *)host, sizeof(new_host), "%s", new_host); // Cast para remover const

                redirects++;
                close(sockfd);
                continue;
            } else {
                printf("[%s!%s] Redirection with no URL found\n", CHAT_VERMELHO, CHAT_PADRAO);
            }
        } else {
            if (verbose == 1) {
                if (ignore404 == 0 || (ignore404 == 1 && status_code != 404)) {
                    printf("[%s-%s] (HTTP: %d) Not Found -> %shttp://%s%s%s \n", CHAT_VERMELHO, CHAT_PADRAO, status_code, CHAT_VERMELHO, host, final_path, CHAT_PADRAO);
                }
            }
        }

        close(sockfd);
        return;
    }

    if (redirects >= MAX_REDIRECTS) {
        printf("[%s!%s] Maximum number of redirects reached\n", CHAT_VERMELHO, CHAT_PADRAO);
    }
}

void enumSub(const char *host, const char *subdomain, const char *useragent) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char request[1024];
    char response[MAX_BUFFER_SIZE];

    // Construir o subdomínio completo
    char url[512];
    snprintf(url, sizeof(url), "%s.%s", subdomain, host);

    // Criar socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erro ao abrir o socket");
        return;
    }

    // Obter endereço do servidor
    server = gethostbyname(url);
    if (server == NULL) {
        if (verbose == 1) {
            if (ignore404 == 0) {
                printf("[%s-%s] (HTTP: %d) Not Found -> %shttp://%s%s \n", CHAT_VERMELHO, CHAT_PADRAO, 404, CHAT_VERMELHO, url, CHAT_PADRAO);
            }
        }
        close(sockfd);
        return;
    }

    // Configurar endereço do servidor
    bzero((char *)&server_addr, sizeof(server_addr));
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
    snprintf(request, sizeof(request),
             "HEAD / HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: %s\r\n"
             "Connection: close\r\n"
             "\r\n", url, useragent);

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
        printf("[%s+%s] (HTTP: %d) Found -----> %shttp://%s%s\n", CHAT_CIANO, CHAT_PADRAO, status_code, CHAT_CIANO, url, CHAT_PADRAO);
    } else {
        if (verbose == 1) {
            if (ignore404 == 0 || (ignore404 == 1 && status_code != 404)) {
                printf("[%s-%s] (HTTP: %d) Not Found -> %shttp://%s%s \n", CHAT_VERMELHO, CHAT_PADRAO, status_code, CHAT_VERMELHO, url, CHAT_PADRAO);
            }
        }
    }

    close(sockfd);
}

void *workerDir(void *args) {
    if (args == NULL) {
        fprintf(stderr, "Erro: argumentos da thread são nulos\n");
        pthread_exit(NULL);
    }

    ThreadArgs *thread_args = (ThreadArgs *)args;
    enumDir(thread_args->host, thread_args->path, thread_args->useragent);
    free(args); // Liberar memória alocada para os argumentos da thread
    return NULL;
}

void *workerSub(void *args) {
    if (args == NULL) {
        fprintf(stderr, "Erro: argumentos da thread são nulos\n");
        pthread_exit(NULL);
    }

    ThreadArgs *thread_args = (ThreadArgs *)args;
    enumSub(thread_args->host, thread_args->path, thread_args->useragent);
    free(args); // Liberar memória alocada para os argumentos da thread
    return NULL;
}

void start(const char *domain, const char *wordlist, const char *useragent) {

    printf("[%s*%s] Base Domain: %shttp://%s%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, domain, CHAT_PADRAO);
    printf("[%s*%s] Wordlist: %s%s%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, wordlist, CHAT_PADRAO);
    printf("[%s*%s] Max Follow Redirects: %s%d%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, MAX_REDIRECTS, CHAT_PADRAO);
    printf("[%s*%s] User-Agent: %s%s%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, useragent, CHAT_PADRAO);

    if (verbose == 0) {
        printf("[%s*%s] Verbose: %sFalse%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    } else {
        printf("[%s*%s] Verbose: %sTrue%s\n", CHAT_CIANO, CHAT_PADRAO, CHAT_CIANO, CHAT_PADRAO);
    }

    printf("\n================================================================================================\n");
    printf("[%s*%s] Starting analize...", CHAT_CIANO, CHAT_PADRAO);
    printf("\n================================================================================================\n");

    char linha[256];
    pthread_t threads[max_threads];
    int thread_count = 0;

    // Encontrar diretórios
    if (tipo == "dir" || tipo == "all") {

        FILE *arquivo = fopen(wordlist, "r");
        if (arquivo == NULL) {
            printf("[%s!%s] Wordlist %s%s%s not found!\n", CHAT_VERMELHO, CHAT_PADRAO, CHAT_VERMELHO, wordlist, CHAT_PADRAO);
            printf("[%s!%s] Your search has stopped\n", CHAT_VERMELHO, CHAT_PADRAO);
            exit(EXIT_FAILURE);
        }

        printf("\n[%s*%s] Starting directory analysis\n", CHAT_CIANO, CHAT_PADRAO);

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

            if (pthread_create(&threads[thread_count++], NULL, workerDir, args) != 0) {
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
    }

    if (tipo == "sub" || tipo == "all") {

        FILE *arquivo = fopen(wordlist, "r");
        if (arquivo == NULL) {
            printf("[%s!%s] Wordlist %s%s%s not found!\n", CHAT_VERMELHO, CHAT_PADRAO, CHAT_VERMELHO, wordlist, CHAT_PADRAO);
            printf("[%s!%s] Your search has stopped\n", CHAT_VERMELHO, CHAT_PADRAO);
            exit(EXIT_FAILURE);
        }

        printf("\n[%s*%s] Starting subdomain analisys\n", CHAT_CIANO, CHAT_PADRAO);

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
            snprintf(args->path, sizeof(args->path), "%s", path);
            strncpy(args->useragent, useragent ? useragent : "", sizeof(args->useragent) - 1);
            args->useragent[sizeof(args->useragent) - 1] = '\0';

            if (pthread_create(&threads[thread_count++], NULL, workerSub, args) != 0) {
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
    }

    printf("\n================================================================================================\n");
    printf("[%s*%s] Analysis completed", CHAT_CIANO, CHAT_PADRAO);
    printf("\n================================================================================================\n");

}

int main(int argc, char **argv) {
    banner();

    const char *domain = NULL;
    const char *wordlist = NULL;

    static struct option long_options[] = {
        {"domain", required_argument, 0, 'd'},
        {"wordlist", required_argument, 0, 'w'},
        {"useragent", optional_argument, 0, 'u'},
        {"threads", optional_argument, 0, 't'},
        {"type", optional_argument, 0, 'y'},
        {"help", no_argument, 0, 'h'},
        {"verbose", no_argument, 0, 'v'},
        {"ignore404", no_argument, 0, 'i'},
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
            case 'v':
                verbose = 1;
                break;
            case 'i':
                ignore404 = 1;
                break;
            case 'y':

                if (strcmp(optarg, "sub") == 0) {
                    tipo = "sub";
                } else if (strcmp(optarg, "all") == 0) {
                    tipo = "all";
                } else {
                    tipo = "dir";
                }

                break;
            case 't':
                max_threads = atoi(optarg);
                if (max_threads <= 0) {
                    max_threads = 1;
                }
                break;
            case 'h':
                help();
                commands();
                exit(EXIT_SUCCESS);
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
