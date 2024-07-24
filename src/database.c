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

    for (int i = 0; i < 6; i++) {  // Only use the first 12 hex digits (6 bytes)
        sprintf(&output[i * 2], "%02x", (unsigned int)digest[i]);
    }
    output[12] = '\0'; // Null-terminate the string
}

int initialize_database() {
    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open("imageboard.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // Update schema to include the Timestamp column
    char *sql = "CREATE TABLE IF NOT EXISTS Posts("
                "PostID TEXT PRIMARY KEY, "
                "Content TEXT, "
                "ImagePath TEXT, "
                "Timestamp TEXT);";
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

int store_post(const char *content, const char *image_path) {
    sqlite3 *db;
    char *err_msg = 0;
    int rc;

    // Open the database
    rc = sqlite3_open("imageboard.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // Compute current timestamp
    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Generate a unique ID for the post
    char id[13];  // 12 characters for MD5 hash + 1 null terminator
    compute_md5(content, id);

    // Prepare SQL statement to insert the post
    char *sql = sqlite3_mprintf(
        "INSERT INTO Posts(PostID, Content, ImagePath, Timestamp) VALUES(%Q, %Q, %Q, %Q);",
        id, content, image_path, timestamp
    );

    // Execute SQL statement
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    // Close the database
    sqlite3_close(db);
    return 0;
}
    