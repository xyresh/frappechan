/*
*
*   Frappechan database handling
*
*   copyright xyresh: https://github.com/xyresh
*
*
*/
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <time.h>
#include <ctype.h>

void print_all_posts() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open("imageboard.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *sql = "SELECT PostID, Content, ImagePath, Timestamp, ReplyTo FROM Posts";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        printf("PostID: %s\n", sqlite3_column_text(stmt, 0));
        printf("Content: %s\n", sqlite3_column_text(stmt, 1));
        printf("ImagePath: %s\n", sqlite3_column_text(stmt, 2));
        printf("Timestamp: %s\n", sqlite3_column_text(stmt, 3));
        const unsigned char *reply_to = sqlite3_column_text(stmt, 4);
        printf("ReplyTo: %s\n", reply_to ? (const char *)reply_to : "NULL");
        printf("\n");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


//compute MD5 hash
void compute_md5(const char *str, char output[13]) {
    EVP_MD_CTX *mdctx;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;

    mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        fprintf(stderr, "Failed to create MD5 context\n");
        return;
    }

    if (1 != EVP_DigestInit_ex(mdctx, EVP_md5(), NULL)) {
        fprintf(stderr, "Failed to initialize MD5 digest\n");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    if (1 != EVP_DigestUpdate(mdctx, str, strlen(str))) {
        fprintf(stderr, "Failed to update MD5 digest\n");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    if (1 != EVP_DigestFinal_ex(mdctx, digest, &digest_len)) {
        fprintf(stderr, "Failed to finalize MD5 digest\n");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    EVP_MD_CTX_free(mdctx);

    for (int i = 0; i < 6; i++) {
        sprintf(&output[i * 2], "%02x", (unsigned int)digest[i]);
    }
    output[12] = '\0'; // Null-terminate the string
}
//initialize database imageboard.db, the main database
//if it already exists we skip this part.
int initialize_database() {
    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open("imageboard.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    char *sql = "CREATE TABLE IF NOT EXISTS Posts("
                "PostID TEXT PRIMARY KEY, "
                "Content TEXT, "
                "ImagePath TEXT, "
                "Timestamp TEXT, "
                "ReplyTo TEXT);";  // Add ReplyTo column
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_close(db);
    return 0;
}


int store_post(const char *content, const char *image_path, const char *reply_to) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open("imageboard.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    const char *final_image_path = image_path && strlen(image_path) > 0 ? image_path : "static/img/                         frappelogo.png";

    time_t ttnow = time(NULL);
    char tttimestamp[20];
    strftime(tttimestamp, sizeof(tttimestamp), "%Y-%m-%d %H:%M:%S", localtime(&ttnow));

    // XOR the content with the current time since epoch and store 
    size_t content_len = strlen(content);
    char *ttt = (char *)malloc(content_len + 1);
    if (!ttt) {
        fprintf(stderr, "ttnow Memory allocation failed\n");
        sqlite3_close(db);
        return 1;
    }

    for (size_t i = 0; i < content_len; ++i) {
        ttt[i] = content[i] ^ ((unsigned char*  )&ttnow)[i % sizeof(time_t)];
    }
    ttt[content_len] = '\0'; // Null-terminate the XORed string

    char id[13];
    compute_md5(ttt, id);

    // Get current timestamp
    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    const char *sql = "INSERT INTO Posts (PostID, Content, ImagePath, Timestamp, ReplyTo) VALUES (?, ?, ?, ?, ?)";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_text(stmt, 1, id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, content, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, image_path, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, timestamp, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, reply_to && strlen(reply_to) > 0 ? reply_to : NULL, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert data: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return rc == SQLITE_DONE ? 0 : 1;
}
