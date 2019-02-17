#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define FAIL        -1
#define BUFF_SIZE    (1024*1024)

#define bool        int
#define true        1
#define false       0

char szApiId[10];
char szApiKey[50];

void trim(char *str) {
    for (int i = 0; i < strlen(str); ++i) {
        if (str[i] == '\r' || str[i] == '\n') {
            str[i]='\0';
            break;
        }
    }
}

/*---------------------------------------------------------------------*/
/*--- InitCTX - initialize the SSL engine.                          ---*/
/*---------------------------------------------------------------------*/
SSL_CTX* InitCTX(void) {
    OpenSSL_add_all_algorithms();       /* Load cryptos, et.al. */
    SSL_load_error_strings();           /* Bring in and register error messages */
    const SSL_METHOD *method = TLS_client_method();       /* Create new client-method instance */
    SSL_CTX *ctx = SSL_CTX_new(method);          /* Create new context */
    if ( ctx == NULL ) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
bool load_credentials() {
    char buff[255];

    FILE *fp = fopen("oxford.txt", "r");
    if (fp==NULL)
        return false;

    while (fgets(buff, sizeof(buff), fp)) {
        char *ptr = strchr(buff, '=');
        if (ptr == NULL)
            continue;
        *ptr++='\0';
        trim(ptr);
        if (strcmp (buff, "id") == 0) {
            strcpy (szApiId, ptr);
        }
        else if (strcmp (buff, "key") == 0) {
            strcpy (szApiKey, ptr);
        }
    }    
    fclose(fp);
}

bool parse_json(char *json, char *tag, char *sub_tag, char *value) {
    char *ptr = strstr(json, tag);
    if (ptr) {
        ptr += strlen (tag);
        if (sub_tag) {
            ptr = strstr(ptr, sub_tag);
            ptr += strlen(sub_tag);
        }
        char *ptr1 = strchr (ptr, '\"');
        char *ptr2 = strchr (ptr1 + 1, '\"');
        int len = ptr2 - ptr1 - 1;
        strncpy (value, ptr1 + 1, len);
        value[len]='\0';
    }
    return strlen(value) > 0 ? true : false;
}

int call (char *word, char *category, char *definitions, char *example, char *audio) {
    struct addrinfo hints, *servinfo;
    
    category[0]=definitions[0]=example[0]=audio[0]='\0';
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo("od-api.oxforddictionaries.com", "https", &hints, &servinfo) != 0) {
        return false;
    }
    int total_len = 0;
    //Create socket
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc != -1) {
        //Connect to remote server
        if (connect(socket_desc, (struct sockaddr *)servinfo->ai_addr, servinfo->ai_addrlen) == 0) {
            SSL_CTX *ctx = InitCTX();
            SSL *ssl = SSL_new(ctx);
            SSL_set_fd(ssl, socket_desc);
            if (SSL_connect(ssl) != FAIL) {
                //Send request
                char message[512];
                char buffer[512+12];
                char *server_reply;
                
                server_reply = malloc(BUFF_SIZE);
                sprintf(message, "GET /api/v1/entries/en/%s HTTP/1.1\r\nHost: od-api.oxforddictionaries.com\r\nConnection: close\r\nAccept: application/json\r\napp_id: %s\r\napp_key: %s\r\n\r\n", word, szApiId, szApiKey);
                SSL_write(ssl, message, strlen(message));
                server_reply[0]='\0';
                while(1) {
                    int len = SSL_read(ssl, buffer, sizeof(buffer)-12);
                    if (len <= 0 )
                        break;
                    buffer[len]='\0';
                    strcat (server_reply, buffer);
                    total_len += len;
                }
                if (strstr(server_reply, "Authentication failed")) {
                    printf("*****Authentication failed\n");
                }
                parse_json(server_reply, "\"lexicalCategory\"", NULL, category);
                parse_json(server_reply, "\"definitions\"", NULL, definitions);
                parse_json(server_reply, "\"examples\"", "\"text\"", example);
                parse_json(server_reply, "\"audioFile\"", NULL, audio);
                SSL_free(ssl);
                free(server_reply);
            }
            SSL_CTX_free(ctx);
        }
        shutdown (socket_desc, SHUT_RD);
    }
    freeaddrinfo(servinfo);

    return total_len > 0 ? true : false;
}

//gcc -o init init.c -L/usr/lib -lssl -lcrypto
int main(int argc, char *argv[])
{
    char filename[255];
    char output[255];
    char category[50];
    char definitions[2048];
    char example[255];
    char audio[255];
    char buff[255];
    char *version="#Ver1.1\r\n";
    bool done = false;

    load_credentials();

    strcpy (filename, "dict/Asian.txt");
    strcpy (output, "dict/SpellingBeeAsian.txt");
    if (argc > 1) {
        if (strcmp(argv[1], "--test") == 0) {
            strcpy (buff, "shampoo");
            call(buff, category, definitions, example, audio);
            printf("%s:%s,%s(%s)\n", buff, category, definitions, example);

            done = true;
        }
        else {
            strcpy (filename, "dict/");
            strcat (filename, argv[1]);
            
            strcpy (output, "dict/SpellingBee");
            strcat (output, argv[1]);
        }
    }
    if (done)
        return 0;
    
    char *str;
    FILE *fp = fopen(filename, "r");
    if (fp==NULL) {
        printf("File does not exist: %s\n", filename);
        return 1;
    }
    
    FILE *fpOutput = fopen(output, "w");
    fwrite (version, strlen(version), 1, fpOutput);

    
    str = fgets(buff, sizeof(buff), fp);
    while (str!= NULL) {
        trim(buff);
        call(buff, category, definitions, example, audio);
        printf("%s:%s,%s[%s]\n", buff, category, definitions, example);
        
        //word
        fwrite (buff, strlen(buff), 1, fpOutput);
        fwrite ("\r\n", 2, 1, fpOutput);
        //category
        fwrite (category, strlen(category), 1, fpOutput);
        fwrite ("\r\n", 2, 1, fpOutput);
        //definition
        fwrite (definitions, strlen(definitions), 1, fpOutput);
        fwrite ("\r\n", 2, 1, fpOutput);
        //example
        fwrite (example, strlen(example), 1, fpOutput);
        fwrite ("\r\n", 2, 1, fpOutput);
        //audio
        fwrite (audio, strlen(audio), 1, fpOutput);
        fwrite ("\r\n", 2, 1, fpOutput);
        //end
        fwrite ("\r\n", 2, 1, fpOutput);
        //60 calls per minute
        usleep(1500 * 1000);
        str = fgets(buff, sizeof(buff), fp);
    }
    fclose(fp);
    fclose (fpOutput);
    
    return 0;
}
