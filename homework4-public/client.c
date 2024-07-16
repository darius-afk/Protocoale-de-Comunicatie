#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <arpa/inet.h>
#include "buffer.h"
#include "helpers.h"

#define COMLEN 10001

char *session_cookie = NULL;
char *token = NULL;

void clean_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void get_input(char *prompt, char *buffer, int buffer_size) {
    printf("%s", prompt);
    if (fgets(buffer, buffer_size, stdin) != NULL) {
        // Remove newline character if present
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
    }
}

void register_user(char *username, char *password)
{
    int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
    char message[COMLEN];

    char payload[COMLEN];
    snprintf(payload, COMLEN, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);

    snprintf(message, COMLEN,
             "POST /api/v1/tema/auth/register HTTP/1.1\r\n"
             "Host: 34.246.184.49:8080\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %ld\r\n\r\n"
             "%s",
             strlen(payload), payload);

    send_to_server(sockfd, message);

    char *response = receive_from_server(sockfd);

    close_connection(sockfd);

    if (response != NULL) {
        // printf("Response from server:\n%s\n", response);
        if (strstr(response, "HTTP/1.1 201 Created") != NULL) {
            printf("User registered: SUCCESS!\n");
        } else {
            char *error_message_start = strstr(response, "{\"error\":\"");
            if (error_message_start != NULL) {
                error_message_start += strlen("{\"error\":\"");
                char *error_message_end = strstr(error_message_start, "\"}");
                if (error_message_end != NULL) {
                    int error_message_length = error_message_end - error_message_start;
                    char error_message[COMLEN];
                    strncpy(error_message, error_message_start, error_message_length);
                    error_message[error_message_length] = '\0';

                    if (strstr(error_message, "is taken") != NULL) {
                        printf("The username %s is already taken: ERROR!\n", username);
                    } else {
                        printf("User could not be registered: ERROR!\n");
                    }
                }
            } else {
                printf("User could not be registered: ERROR!\n");
            }
        }
        free(response);
    } else {
        printf("No response from server: ERROR!\n");
    }
}

void login_user(char *username, char *password)
{
    int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
    char message[COMLEN];
    char payload[COMLEN];

    snprintf(payload, COMLEN, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);

    snprintf(message, COMLEN,
             "POST /api/v1/tema/auth/login HTTP/1.1\r\n"
             "Host: 34.246.184.49:8080\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %ld\r\n\r\n"
             "%s",
             strlen(payload), payload);

    send_to_server(sockfd, message);

    char *response = receive_from_server(sockfd);

    close_connection(sockfd);

    if (response != NULL) {
        if (strstr(response, "HTTP/1.1 200 OK") != NULL) {
            printf("User logged in: SUCCESS!\n");

            char *cookie_start = strstr(response, "Set-Cookie: ");
            if (cookie_start != NULL) {
                cookie_start += strlen("Set-Cookie: ");
                char *cookie_end = strstr(cookie_start, "\r\n");
                if (cookie_end != NULL) {
                    int cookie_length = cookie_end - cookie_start;
                    session_cookie = malloc(cookie_length + 1);
                    strncpy(session_cookie, cookie_start, cookie_length);
                    session_cookie[cookie_length] = '\0';
                    // printf("Session cookie: %s\n", session_cookie);
                }
            }
        } else {
            printf("User could not be logged in: ERROR!\n");

            char *body = strstr(response, "\r\n\r\n");
            if (body != NULL) {
                body += 4;

                // printf("Server response body:\n%s\n", body);
            }
        }
        free(response);
    } else {
        printf("No response from server: ERROR!\n");
    }
}

void enter_library()
{
    if (session_cookie == NULL) {
        printf("You are not authenticated. Please log in first: ERROR!\n");
        return;
    }

    int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
    char message[COMLEN];

    snprintf(message, COMLEN,
             "GET /api/v1/tema/library/access HTTP/1.1\r\n"
             "Host: 34.246.184.49:8080\r\n"
             "Cookie: %s\r\n"
             "\r\n",
             session_cookie);

    send_to_server(sockfd, message);

    char *response = receive_from_server(sockfd);

    close_connection(sockfd);

    if (response != NULL) {
        // printf("Response from server:\n%s\n", response);

        if (strstr(response, "HTTP/1.1 200 OK") != NULL) {
            printf("Entered library: SUCCESS!\n");

            char *token_start = strstr(response, "{\"token\":\"");
            if (token_start != NULL) {
                token_start += strlen("{\"token\":\"");
                char *token_end = strstr(token_start, "\"}");
                if (token_end != NULL) {
                    int token_length = token_end - token_start;
                    token = malloc(token_length + 1);
                    strncpy(token, token_start, token_length);
                    token[token_length] = '\0';
                    // printf("Token: %s\n", token);
                }
            }
        } else {
            printf("Could not enter library: ERROR!\n");
            char *body = strstr(response, "\r\n\r\n");
            if (body != NULL) {
                body += 4;
                // printf("Server response body:\n%s\n", body);
            }
        }
        free(response);
    } else {
        printf("No response from server: ERROR!\n");
    }
}

void get_books()
{
    if (session_cookie == NULL || token == NULL) {
        printf("No session cookie or token found: ERROR!\n");
        return;
    }

    int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
    char message[COMLEN];

    snprintf(message, COMLEN,
             "GET /api/v1/tema/library/books HTTP/1.1\r\n"
             "Host: 34.246.184.49:8080\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Authorization: Bearer %s\r\n"
             "Cookie: %s\r\n\r\n",
             token, session_cookie);

    send_to_server(sockfd, message);

    char *response = receive_from_server(sockfd);

    close_connection(sockfd);

    if (response != NULL) {
        // printf("Response from server:\n%s\n", response);

        if (strstr(response, "HTTP/1.1 200 OK") != NULL) {
            printf("Books retrieved: SUCCESS!\n");

            // Print the response body to see the list of books
            char *body = strstr(response, "\r\n\r\n");
            if (body != NULL) {
                body += 4;
                printf("Books:\n%s\n", body);
            }
        } else {
            printf("Failed to retrieve books: ERROR!\n");

            // Find the start of the response body
            char *body = strstr(response, "\r\n\r\n");
            if (body != NULL) {
                body += 4;
                // printf("Server response body:\n%s\n", body);
            }
        }
        free(response);
    } else {
        printf("No response from server: ERROR!\n");
    }
}

void get_book(int book_id)
{
    if (session_cookie == NULL || token == NULL) {
        printf("No session cookie or token found: ERROR!\n");
        return;
    }

    int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
    char message[COMLEN];

    snprintf(message, COMLEN,
             "GET /api/v1/tema/library/books/%d HTTP/1.1\r\n"
             "Host: 34.246.184.49:8080\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Authorization: Bearer %s\r\n"
             "Cookie: %s\r\n\r\n",
             book_id, token, session_cookie);

    send_to_server(sockfd, message);

    char *response = receive_from_server(sockfd);

    close_connection(sockfd);

    if (response != NULL) {
        // printf("Response from server:\n%s\n", response);

        if (strstr(response, "HTTP/1.1 200 OK") != NULL) {
            printf("Book details retrieved: SUCCESS!\n");

            char *body = strstr(response, "\r\n\r\n");
            if (body != NULL) {
                body += 4;
                printf("Book details:\n%s\n", body);
            }
        } else {
            char *body = strstr(response, "\r\n\r\n");
            if (body != NULL) {
                body += 4;
                if (strstr(body, "{\"error\":\"No book was found!\"}") != NULL) {
                    printf("No book was found with ID %d: ERROR!\n", book_id);
                } else {
                    printf("Failed to retrieve book details: ERROR!\n");
                    // printf("Server response body:\n%s\n", body);
                }
            } else {
                printf("Failed to retrieve book details: ERROR!\n");
            }
        }
        free(response);
    } else {
        printf("No response from server: ERROR!\n");
    }
}

void add_book(char *title, char *author, char *genre, int page_count, char *publisher)
{
    if (session_cookie == NULL || token == NULL) {
        printf("No session cookie or token found: ERROR!\n");
        return;
    }
 
    int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
    char message[COMLEN];
    char payload[COMLEN];
 
    snprintf(payload, COMLEN,
             "{\"title\":\"%s\", \"author\":\"%s\", \"genre\":\"%s\", \"page_count\":%d, \"publisher\":\"%s\"}",
             title, author, genre, page_count, publisher);
 
    snprintf(message, COMLEN,
             "POST /api/v1/tema/library/books HTTP/1.1\r\n"
             "Host: 34.246.184.49:8080\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %ld\r\n"
             "Authorization: Bearer %s\r\n"
             "Cookie: %s\r\n\r\n"
             "%s",
             strlen(payload), token, session_cookie, payload);
 
    send_to_server(sockfd, message);
 
    char *response = receive_from_server(sockfd);
 
    close_connection(sockfd);
 
    if (response != NULL) {
        // printf("Response from server:\n%s\n", response);
 
        if (strstr(response, "HTTP/1.1 200 OK") != NULL || strstr(response, "HTTP/1.1 201 Created") != NULL) {
            printf("Book added successfully: SUCCESS!\n");
        } else {
            printf("Failed to add book: ERROR!\n");
 
            // char *body = strstr(response, "\r\n\r\n");
            // if (body != NULL) {
            //     body += 4;
            //     printf("Server response body:\n%s\n", body);
            // }
        }
        free(response);
    } else {
        printf("No response from server: ERROR!\n");
    }
}

void delete_book(int book_id)
{
    if (session_cookie == NULL || token == NULL) {
        printf("No session cookie or token found: ERROR!\n");
        return;
    }

    int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
    char message[COMLEN];

    snprintf(message, COMLEN,
             "DELETE /api/v1/tema/library/books/%d HTTP/1.1\r\n"
             "Host: 34.246.184.49:8080\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Authorization: Bearer %s\r\n"
             "Cookie: %s\r\n\r\n",
             book_id, token, session_cookie);

    send_to_server(sockfd, message);

    char *response = receive_from_server(sockfd);

    close_connection(sockfd);

    if (response != NULL) {
        // printf("Response from server:\n%s\n", response);

        if (strstr(response, "HTTP/1.1 200 OK") != NULL) {
            printf("Book with ID %d deleted: SUCCESS!\n", book_id);
        } else if (strstr(response, "{\"error\":\"No book was deleted!\"}") != NULL) {
            printf("Book with ID %d does NOT exist: ERROR!\n", book_id);
        } else if (strstr(response, "{\"error\":\"") != NULL) {
            char *error_start = strstr(response, "{\"error\":\"") + strlen("{\"error\":\"");
            char *error_end = strstr(error_start, "\"}");
            char error_message[COMLEN];
            strncpy(error_message, error_start, error_end - error_start);
            error_message[error_end - error_start] = '\0';
            printf("Error: %s: ERROR!\n", error_message);
        } else {
            printf("Failed to delete book with ID %d: ERROR!\n", book_id);
        }
        free(response);
    } else {
        printf("No response from server: ERROR!\n");
    }
}

void logout()
{
    if (session_cookie == NULL /*|| token == NULL*/) {
        printf("You are not logged in: ERROR!\n");
        return;
    }

    int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
    char message[COMLEN];

    snprintf(message, COMLEN,
             "GET /api/v1/tema/auth/logout HTTP/1.1\r\n"
             "Host: 34.246.184.49:8080\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Authorization: Bearer %s\r\n"
             "Cookie: %s\r\n\r\n",
             token, session_cookie);

    send_to_server(sockfd, message);

    char *response = receive_from_server(sockfd);

    close_connection(sockfd);

    if (response != NULL) {
        // printf("Response from server:\n%s\n", response);

        if (strstr(response, "HTTP/1.1 200 OK") != NULL) {
            printf("User logged out: SUCCESS!\n");

            // Clear the session cookie and token
            free(session_cookie);
            session_cookie = NULL;
            free(token);
            token = NULL;
        } else {
            printf("User could not be logged out: ERROR!\n");

            // Extract and print the error message from the response body
            char *body = strstr(response, "\r\n\r\n");
            if (body != NULL) {
                body += 4;
                printf("Server response body:\n%s\n", body);
            }
        }
        free(response);
    } else {
        printf("No response from server: ERROR!\n");
    }
}


int main(int argc, char *argv[])
{
    int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
    char command[COMLEN];
    char buffer[COMLEN];

    while (1) {
        scanf("%s", command);

        if (strcmp(command, "register") == 0) {
            char username[COMLEN];
            char password[COMLEN];

            printf("\nusername=");
            scanf("%s", username);
            printf("\npassword=");
            scanf("%s", password);
            register_user(username, password);

        } else if (strcmp(command, "login") == 0) {
            char username[COMLEN];
            char password[COMLEN];
            clean_buffer();

            printf("\nusername=");
            scanf("%s", username);
            printf("\npassword=");
            scanf("%s", password);
            login_user(username, password);

        } else if (strcmp(command, "enter_library") == 0) {
            enter_library();
        } else if (strcmp(command, "get_books") == 0) {
            get_books();
        } else if (strcmp(command, "get_book") == 0) {
            int book_id;
            printf("\nbook_id=");
            scanf("%d", &book_id);
            get_book(book_id);
        } else if (strcmp(command, "add_book") == 0) {
            char title[COMLEN];
            char author[COMLEN];
            char genre[COMLEN];
            char page_count_str[COMLEN];
            char publisher[COMLEN];
            int page_count;
            int ok = 1;
            clean_buffer();

            get_input("\ntitle = ", title, COMLEN);
            get_input("author = ", author, COMLEN);
            get_input("genre = ", genre, COMLEN);
            get_input("page_count = ", page_count_str, COMLEN);
            get_input("publisher = ", publisher, COMLEN);

            // Check if any field is empty
            if (strlen(title) == 0 || strlen(author) == 0 || strlen(genre) == 0 || strlen(page_count_str) == 0 || strlen(publisher) == 0) {
                printf("All fields must be filled: ERROR!\n");
            }

            // Validate and convert page_count
            for (int i = 0; page_count_str[i] != '\0'; i++) {
                if (page_count_str[i] < '0' || page_count_str[i] > '9') {
                    printf("Page count must be a number: ERROR!\n");
                    ok = 0;
                    break;
                }
            }

            if (ok) {
                page_count = atoi(page_count_str);  // Convert the valid string to integer
                add_book(title, author, genre, page_count, publisher);
            }
        } else if (strcmp(command, "delete_book") == 0) {
            clean_buffer();
            int book_id;
            printf("\nbook_id=");
            scanf("%d", &book_id);
            delete_book(book_id);
        } else if (strcmp(command, "logout") == 0) {
            clean_buffer();
            logout();
        } else if (strcmp(command, "exit") == 0) {
            clean_buffer();
            break;
        } else {
            clean_buffer();
            printf("This command does not exist: ERROR!\n");
        }
    }

    return 0;
}