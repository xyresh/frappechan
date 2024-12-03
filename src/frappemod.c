#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <sqlite3.h>
#include <time.h>
#include <locale.h>
#include <signal.h>

#define MAX_POSTS 100
#define MAX_CONTENT_LENGTH 256
#define ID_WIDTH 12
#define CONTENT_WIDTH (DISPLAY_WIDTH - ID_WIDTH - 20)  // Adjust based on ID_WIDTH and padding
#define SCROLLABLE_HEIGHT (LINES - 3) // Adjust dynamically based on terminal height
#define DISPLAY_WIDTH COLS

typedef struct {
    char id[13];
    char content[MAX_CONTENT_LENGTH];
    char timestamp[20];  // Added timestamp
} Post;

static Post posts[MAX_POSTS];
static int num_posts = 0;
static int selected_index = 0;
static int start_index = 0;  // Starting index for scrolling

void handle_resize(int sig) {
    // Reinitialize the screen size and adjust layout
    endwin();            // End the current ncurses session
    refresh();           // Refresh the screen
    clear();             // Clear the screen
    resize_term(LINES, DISPLAY_WIDTH);  // Adjust window size to new terminal size
    // You can also add code to adjust any UI elements here, if needed
    start_color();       // Restart colors if you're using them
    // Call display functions to update window contents after resizing
    display_title();
    display_posts(stdscr); // Redraw the posts
    display_command_line();
    refresh();           // Ensure the screen is redrawn with the new size
}

int fetch_posts(sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT PostID, Content, Timestamp FROM Posts";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to fetch posts: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    num_posts = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (num_posts >= MAX_POSTS) break;

        strncpy(posts[num_posts].id, (const char *)sqlite3_column_text(stmt, 0), sizeof(posts[num_posts].id) - 1);
        strncpy(posts[num_posts].content, (const char *)sqlite3_column_text(stmt, 1), sizeof(posts[num_posts].content) - 1);
        strncpy(posts[num_posts].timestamp, (const char *)sqlite3_column_text(stmt, 2), sizeof(posts[num_posts].timestamp) - 1);
        posts[num_posts].id[12] = '\0';
        posts[num_posts].content[MAX_CONTENT_LENGTH - 1] = '\0';
        posts[num_posts].timestamp[19] = '\0';

        num_posts++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

// Initialize the archive database
int initialize_archive() {
    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open("archive.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open archive database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // Create table if it does not exist
    char *sql = "CREATE TABLE IF NOT EXISTS ArchivedPosts("
                "PostID TEXT PRIMARY KEY, "
                "Content TEXT, "
                "ImagePath TEXT, "
                "Timestamp TEXT, "
                "ReplyTo TEXT);";
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    // Test write operation
    const char *test_insert_sql = "INSERT INTO ArchivedPosts (PostID, Content, Timestamp) VALUES ('test_id', 'test_content', 'test_timestamp');";
    rc = sqlite3_exec(db, test_insert_sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to write to archive database: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    // Clean up test data
    const char *cleanup_sql = "DELETE FROM ArchivedPosts WHERE PostID = 'test_id';";
    sqlite3_exec(db, cleanup_sql, 0, 0, 0);

    sqlite3_close(db);
    return 0;
}               

void display_posts(WINDOW *win) {
    werase(win);

    for (int i = 0; i < SCROLLABLE_HEIGHT && i + start_index < num_posts; i++) {
        if (i + start_index == selected_index) {
            wattron(win, A_REVERSE);
        }

        // Truncate content to 70 characters and append "..." if it is longer
        char truncated_content[MAX_CONTENT_LENGTH];
        strncpy(truncated_content, posts[i + start_index].content, 70);
        truncated_content[70] = '\0';  // Ensure the string is null-terminated
        if (strlen(posts[i + start_index].content) > 70) {
            strcat(truncated_content, "...");
        }

        mvwprintw(win, i, 0, "ID: %-12s", posts[i + start_index].id);
        mvwprintw(win, i, ID_WIDTH + 1, "  Content: %-*s", CONTENT_WIDTH, truncated_content);

        // Move to the end of the line for timestamp
        mvwprintw(win, i, DISPLAY_WIDTH - 20, "%s", posts[i + start_index].timestamp);

        if (i + start_index == selected_index) {
            wattroff(win, A_REVERSE);
        }
    }

    // Print the separator line separately
    if (num_posts > 0) {
        mvwprintw(win, SCROLLABLE_HEIGHT, 0, "----------------------------");
    }

    // Refresh the window to display changes
    wrefresh(win);
}


void display_title() {
    move(0, 0);
    attron(A_REVERSE);  
    clrtoeol();
    printw("    FrappeMod V0.1");
    attroff(A_REVERSE);
    refresh();
}

void display_command_line() {
    move(LINES - 2, 0);
    attron(A_REVERSE); 
    clrtoeol();
    printw("[a] Archive [d] Delete [q] Quit [r] Refresh ");
    attroff(A_REVERSE);
    refresh();
}

void handle_input(WINDOW *win, sqlite3 *db) {
    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case KEY_UP:
                if (selected_index > 0) {
                    selected_index--;
                    if (selected_index < start_index) {
                        start_index--;
                    }
                }
                break;
            case KEY_DOWN:
                if (selected_index < num_posts - 1) {
                    selected_index++;
                    if (selected_index >= start_index + SCROLLABLE_HEIGHT) {
                        start_index++;
                    }
                }
                break;
            case 'd': {
                // Delete post
                char sql[256];
                snprintf(sql, sizeof(sql), "DELETE FROM Posts WHERE PostID = '%s'", posts[selected_index].id);
                sqlite3_exec(db, sql, 0, 0, 0);
                fetch_posts(db);
                selected_index = 0; // Reset index after deletion
                start_index = 0;   // Reset scroll
                break;
            }
            case 'a': {
                // Archive post
                sqlite3 *archive_db;
                int rc = sqlite3_open("archive.db", &archive_db);
                if (rc != SQLITE_OK) {
                    fprintf(stderr, "Cannot open archive database: %s\n", sqlite3_errmsg(archive_db));
                    break;
                }

                char *err_msg = 0;
                const char *insert_sql = "INSERT INTO ArchivedPosts (PostID, Content, Timestamp) VALUES (?, ?, ?)";
                sqlite3_stmt *stmt;
                rc = sqlite3_prepare_v2(archive_db, insert_sql, -1, &stmt, 0);
                if (rc != SQLITE_OK) {
                    fprintf(stderr, "Failed to prepare insert statement: %s\n", sqlite3_errmsg(archive_db));
                    sqlite3_close(archive_db);
                    break;
                }

                sqlite3_bind_text(stmt, 1, posts[selected_index].id, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, posts[selected_index].content, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, posts[selected_index].timestamp, -1, SQLITE_STATIC);

                rc = sqlite3_step(stmt);
                if (rc != SQLITE_DONE) {
                    fprintf(stderr, "Failed to insert archived post: %s\n", sqlite3_errmsg(archive_db));
                }

                sqlite3_finalize(stmt);
                sqlite3_close(archive_db);

                char delete_sql[256];
                snprintf(delete_sql, sizeof(delete_sql), "DELETE FROM Posts WHERE PostID = '%s'", posts[selected_index].id);
                sqlite3_exec(db, delete_sql, 0, 0, 0);
                fetch_posts(db);
                selected_index = 0; // Reset index after archiving
                start_index = 0;   // Reset scroll
                break;
            }
            case 'r': {
                // Refresh posts
                fetch_posts(db);
                selected_index = 0;
                start_index = 0;
                break;
            }
        }
        display_posts(win);
        display_command_line();
    }
}

int main() {
    setlocale(LC_CTYPE, ""); // Ensure UTF-8 locale
    initscr();               // Initialize ncurses
    keypad(stdscr, TRUE);    // Enable special keys handling
    noecho();                // Disable echoing of input
    cbreak();                // Disable line buffering
    
    signal(SIGWINCH, handle_resize);  // Handle resizing when terminal is resized

    // Initialize SQLite database
    sqlite3 *db;
    int rc = sqlite3_open("imageboard.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    // Initialize the archive database
    if (initialize_archive() != 0) {
        sqlite3_close(db);
        return 1;
    }

    // Create a window for posts
    WINDOW *win = newwin(LINES - 3, DISPLAY_WIDTH, 1, 0);
    if (win == NULL) {
        endwin();
        sqlite3_close(db);
        fprintf(stderr, "Failed to create window\n");
        return 1;
    }

    if (fetch_posts(db) != 0) {
        delwin(win);
        endwin();
        sqlite3_close(db);
        return 1;
    }

    // Display UI components
    display_title();
    display_posts(win);
    display_command_line();

    // Handle user input
    handle_input(win, db);

    delwin(win);
    endwin();
    sqlite3_close(db);
    return 0;
}
