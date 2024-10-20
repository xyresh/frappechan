/*
*
*   Frappechan server
*
*   copyright xyresh: https://github.com/xyresh
*
*
*/
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sqlite3.h>
#include "database.h"
#include <cjson/cJSON.h>

#define PORT 80

struct connection_info_struct {
    char *content;
    size_t content_size;
    FILE *file;
    char *filename; // Remove const qualifier
    struct MHD_PostProcessor *pp;
    char *reply_to;
};

static enum MHD_Result iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
                                    const char *filename, const char *content_type,
                                    const char *transfer_encoding, const char *data, uint64_t off, size_t size) {
    struct connection_info_struct *con_info = coninfo_cls;

    if (0 == strcmp(key, "image")) {
        if (size > 0) {
            if (!con_info->file) {
                con_info->filename = strdup(filename);
                if (!con_info->filename) {
                    fprintf(stderr, "Failed to allocate memory for filename\n");
                    return MHD_NO;
                }
                char filepath[256];
                snprintf(filepath, sizeof(filepath), "static/uploads/%s", filename);
                con_info->file = fopen(filepath, "wb");
                if (!con_info->file) {
                    fprintf(stderr, "Failed to open file: %s\n", filepath);
                    return MHD_NO;
                }
            }
            if (fwrite(data, size, 1, con_info->file) != 1) {
                fprintf(stderr, "Failed to write data to file\n");
                return MHD_NO;
            }
        }
    } else if (0 == strcmp(key, "content")) {
        char *new_content = realloc(con_info->content, con_info->content_size + size + 1);
        if (!new_content) {
            fprintf(stderr, "Failed to allocate memory for content\n");
            return MHD_NO;
        }
        con_info->content = new_content;
        memcpy(con_info->content + con_info->content_size, data, size);
        con_info->content_size += size;
        con_info->content[con_info->content_size] = '\0';
    } else if (0 == strcmp(key, "reply_to")) {
        if (size > 0) {
            con_info->reply_to = realloc(con_info->reply_to, size + 1);
            if (!con_info->reply_to) {
                fprintf(stderr, "Failed to allocate memory for reply_to\n");
                return MHD_NO;
            }
            memcpy(con_info->reply_to, data, size);
            con_info->reply_to[size] = '\0';
        }
    }
    return MHD_YES;
}



void request_completed(void *cls, struct MHD_Connection *connection,
                       void **con_cls, enum MHD_RequestTerminationCode toe) {
    struct connection_info_struct *con_info = *con_cls;

    if (NULL == con_info)
        return;

    if (con_info->content)
        free(con_info->content);
    if (con_info->filename) // Ensure filename is not NULL before freeing
        free(con_info->filename);
    if (con_info->pp)
        MHD_destroy_post_processor(con_info->pp);

    free(con_info);
}

  

static enum MHD_Result send_page(struct MHD_Connection *connection, const char *page, int status_code) {
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), (void *)page, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static enum MHD_Result redirect_to_index(struct MHD_Connection *connection) {
    struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Location", "/index.html");
    int ret = MHD_queue_response(connection, MHD_HTTP_FOUND, response); // HTTP 302 Found
    MHD_destroy_response(response);
    return ret;
}   

static enum MHD_Result serve_post_thread(struct MHD_Connection *connection, const char *post_id) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open("imageboard.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return MHD_NO;
    }

    const char *sql = "SELECT PostID, Content, ImagePath, Timestamp FROM Posts WHERE ReplyTo = ?";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to fetch replies: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return MHD_NO;
    }

    sqlite3_bind_text(stmt, 1, post_id, -1, SQLITE_STATIC);

    cJSON *json = cJSON_CreateArray();

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        cJSON *reply = cJSON_CreateObject();
        cJSON_AddStringToObject(reply, "id", (const char *)sqlite3_column_text(stmt, 0));
        cJSON_AddStringToObject(reply, "content", (const char *)sqlite3_column_text(stmt, 1));
        cJSON_AddStringToObject(reply, "imagePath", (const char *)sqlite3_column_text(stmt, 2));
        cJSON_AddStringToObject(reply, "timestamp", (const char *)sqlite3_column_text(stmt, 3));
        cJSON_AddItemToArray(json, reply);
    }

    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(json_str), (void *)json_str, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}


static enum MHD_Result serve_static_file(struct MHD_Connection *connection, const char *url) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "static%s", url);

    struct stat sb;
    if (stat(filepath, &sb) == 0 && S_ISREG(sb.st_mode)) {
        FILE *file = fopen(filepath, "rb");
        if (file == NULL) {
            fprintf(stderr, "Failed to open static file: %s\n", filepath);
            return MHD_NO;
        }

        char *buffer = malloc(sb.st_size);
        fread(buffer, 1, sb.st_size, file);
        fclose(file);

        struct MHD_Response *response = MHD_create_response_from_buffer(sb.st_size, buffer, MHD_RESPMEM_MUST_FREE);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    } else {
        return MHD_NO;
    }
}
    
enum MHD_Result answer_to_connection(void *cls, struct MHD_Connection *connection,
                                     const char *url, const char *method,
                                     const char *version, const char *upload_data,
                                     size_t *upload_data_size, void **con_cls) {
    if (NULL == *con_cls) {
        struct connection_info_struct *con_info;
        con_info = malloc(sizeof(struct connection_info_struct));
        if (NULL == con_info)
            return MHD_NO;

        con_info->content = NULL;
        con_info->content_size = 0;
        con_info->file = NULL;
        con_info->filename = NULL;
        con_info->pp = NULL;
        con_info->reply_to = NULL;
        *con_cls = con_info;

        if (0 == strcmp(method, "POST")) {
            con_info->pp = MHD_create_post_processor(connection, 1024, iterate_post, (void *)con_info);
            if (NULL == con_info->pp) {
                fprintf(stderr, "Failed to create post processor\n");
                free(con_info);
                return MHD_NO;
            }
            return MHD_YES;
        }
    }

    if (0 == strcmp(method, "POST")){
        struct connection_info_struct *con_info = *con_cls;

        if (*upload_data_size != 0) {
            MHD_post_process(con_info->pp, upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            if (con_info->file) {
                fclose(con_info->file);
                con_info->file = NULL;
            }

            // Ensure content, filename, and reply_to are not NULL before calling store_post
            if (con_info->content && con_info->filename) {
                if (store_post(con_info->content, con_info->filename, con_info->reply_to) != 0) {
                    return send_page(connection, "<html><body>Failed to store post!</body></html>", MHD_HTTP_INTERNAL_SERVER_ERROR);
                }

                // Print all posts to the console after a new post is stored
                print_all_posts();
            } else {
                fprintf(stderr, "Error: Missing content, filename, or reply_to in connection info.\n");
            }  

            // Clean up
            //free(con_info->filename);
            //free(con_info->content);
            //free(con_info->reply_to);
            //free(con_info);

            return redirect_to_index(connection);
        }
    }


    if (0 == strcmp(method, "GET") && strcmp(url, "/posts") == 0) {
        // Handle JSON response for posts
        sqlite3 *db;
        sqlite3_stmt *stmt;
        int rc = sqlite3_open("imageboard.db", &db);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
            return MHD_NO;
        }

        const char *sql = "SELECT PostID, Content, ImagePath, Timestamp, ReplyTo FROM Posts";
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            return MHD_NO;
        }

        cJSON *json = cJSON_CreateArray();

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            cJSON *post = cJSON_CreateObject();
            cJSON_AddStringToObject(post, "id", (const char *)sqlite3_column_text(stmt, 0));
            cJSON_AddStringToObject(post, "content", (const char *)sqlite3_column_text(stmt, 1));
            cJSON_AddStringToObject(post, "imagePath", (const char *)sqlite3_column_text(stmt, 2));
            cJSON_AddStringToObject(post, "timestamp", (const char *)sqlite3_column_text(stmt, 3));
            cJSON_AddStringToObject(post, "replyTo", (const char *)sqlite3_column_text(stmt, 4));
            cJSON_AddItemToArray(json, post);
        }

        char *json_str = cJSON_Print(json);
        cJSON_Delete(json);
        sqlite3_finalize(stmt);
        sqlite3_close(db);

        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(json_str), (void *)json_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    if (0 == strcmp(method, "GET") && (strcmp(url, "/") == 0 || strcmp(url, "/index.html") == 0)) {
        return serve_static_file(connection, "/index.html");
    }

    if (0 == strcmp(method, "GET")) {
        return serve_static_file(connection, url);
    }

    return send_page(connection, "<html><body>Not found.</body></html>", MHD_HTTP_NOT_FOUND);
}




            
int main() {
    mkdir("uploads", 0777);  // Create the uploads directory if it doesn't exist

    if (initialize_database() != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }

    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                              &answer_to_connection, NULL, MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL, MHD_OPTION_END);
    if (NULL == daemon) {
        fprintf(stderr, "Failed to start MHD daemon\n");
        return 1;
    }

    printf("Server is running on http://localhost:%d\n", PORT);
    getchar(); // Wait for user input to stop the server
    MHD_stop_daemon(daemon);
    return 0;
}
