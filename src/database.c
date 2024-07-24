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

int initialize_database() {
    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open("imageboard.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    char *sql = "CREATE TABLE IF NOT EXISTS Posts(Id INTEGER PRIMARY KEY, Content TEXT, ImagePath TEXT);";
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
    int rc = sqlite3_open("imageboard.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    char *sql = sqlite3_mprintf("INSERT INTO Posts(Content, ImagePath) VALUES(%Q, %Q);", content, image_path);
    if (sql == NULL) {
        fprintf(stderr, "Failed to allocate memory for SQL statement\n");
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_close(db);
    return 0;
}

