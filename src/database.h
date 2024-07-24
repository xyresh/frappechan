//database handler headers
#ifndef DATABASE_H
#define DATABASE_H

int initialize_database();
int store_post(const char *content, const char *image_path, const char *reply_to);
void print_all_posts();
#endif