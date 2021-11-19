#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static struct node* root = NULL;

//Node struct
struct node {
    struct node* child;
    struct node* next;
    char* name;
    char* content;
    char* eof;
    bool dir;

    //symlink
    bool is_symlink;
    char* target;
};

int get_size(struct node* file);

struct node* new_node(char* name) {
    struct node *ret = malloc(sizeof(struct node));
    ret->child = NULL;
    ret->next = NULL;
    ret->name = name;
    ret->dir = false; //default olarak file olusturuyorum
    ret->content = NULL;
    ret->eof = NULL;
    ret->is_symlink = false;
    ret-> target = NULL;
    return ret;
}

//bu fonksiyon rekursiv degil, sadece bi alttakilere bakacak
//name: full path name degil
struct node* get_child(struct node* parent, char* name) {
    if (parent->child == NULL)
        return NULL;


    struct node* temp;
    temp = parent->child;



    while (temp != NULL) {
        printf("temp: %s\n", temp->name);
        printf("name: %s\n", name);
        if (strcmp(temp->name, name) == 0) {
            return temp;
        }
        temp = temp->next;
    }

    printf("get child. could not find child.\n");
    return NULL;
}

int number_of_children(struct node* parent) {
    if (parent->dir == false)
        return -1;
    if (parent->child == NULL)
        return 0;
    
    struct node* temp;
    temp = parent->child;
    int ret = 0;

    while (temp != NULL) {
        temp = temp->next;
        ret++;
    }

    return ret;
}

//TODO simdilik append edecek sekilde degil de bastan yazicak sekilde ayarliyorum
int write_to_file(struct node* file, const char *new_content, size_t size, off_t offset) {
    if ((size + offset) > 512) {
        printf("Maximum file size exceeded.");
        return -1;
    }

    //file does not have a content yet
    if (file->content == NULL) {
        file->content = malloc((size + offset) * sizeof(char));
        file->eof = file->content;
    } 
    //file already has content
    else {
        //we need a bigger buffer, copy old data to new buffer
        //yeniden daha büyük bi sey malloclayip eski contenti yenisine yazip devamke
        if ((size + offset) > get_size(file)) {
            char* bigger_buffer = malloc((size + offset) * sizeof(char));
            memcpy(bigger_buffer, file->content, get_size(file));
            file->eof = bigger_buffer + (file->eof - file->content);
            free(file->content);
            file->content = bigger_buffer;
        }
    }
    //TODO binary mode'da calisir mi acaba? null bytedan sonra yazmaya devam edebilmeli memcpy.
    memcpy(file->content + offset, new_content, size);
    file->eof = file->content + offset + size;
    return size;
}

//direkt find("/benim/benim2") seklinde kullanma SEG veriyor. array ver icine
struct node* find(char* given_path) {
    int length = strlen(given_path);
    char path[length+1];
    memcpy(path, given_path, length+1);

    if (root == NULL) {
        printf("Root has not been created yet.\n");
        return NULL;
    }

    //given path is root, it exists
    if (strcmp(path, "/") == 0)
        return root;


    const char del[2] = "/";
    char* token = strtok(path, del);
    struct node* temp = root;

    while (token != NULL) {
        temp = get_child(temp, token);

        if (temp == NULL)
            return NULL;

        token = strtok(NULL, del);
    }
    return temp;
}

//TODO strlen ile yapamazsin bu dalgayi. binary modeda calismiyormus
int get_size(struct node* file) {
    if (file->dir == true)
        return -1;
    if (file->content == NULL)
        return 0;
    
    //return strlen(file->content) + 1;
    return file->eof - file->content;
}

//TODO pathin valid oldugunu ve oyle bi dir olmadigini varsayiyorum
struct node* make_directory(char* given_path, bool is_dir) {
    //printf("given path: %s\n", path);
    int length = strlen(given_path);
    char path[length+1];
    memcpy(path, given_path, length+1);
    
    //can not create root directory again
    if (length == 1) {
        return NULL;
    }
    
    int i;
    char* c = NULL;

    for (i=length-1; i >= 0; --i) {
        c = path+i;

        if (47 == *c) { // '/' == 47
            break;
        }
    }

    printf("make directory. char c: %c\n", *c);
    printf("make directory. int i: %d\n", i);

    char parent[i+1];

    if (i == 0) {
        parent[0] = (char) '/';
        parent[1] = '\0';
    }
    else {
        memcpy(parent, path, i); 
        //printf("overwrite this char with 0 terminal: %c\n", *(parent+i));
        *(parent+i) = '\0';
    }

    printf("inside make directory. parent: %s\n", parent);

    //printf("parent path: %s\n", parent);
    //printf("path itself: %s\n=========\n", path+i+1);
    
    struct node* insert = find(parent);

    if (insert == NULL) {
        printf("invalid path for make_directory\n");
        return NULL;
    }

    char* new_name = malloc(strlen(path+i+1)+1 * sizeof(char));
    memcpy(new_name, path+i+1, strlen(path+i+1)+1);

    struct node* new = new_node(new_name);
    
    new->dir = is_dir;

    if (insert->child == NULL) {
        insert->child = new;
    } else {
        insert = insert->child;
        while (insert != NULL) {
            if (insert->next == NULL) {
                insert->next = new;
                break;
            }
            insert = insert->next;
        }
    }
    return new;
}

int create_link(const char* target, const char* linkpath) {

	char my_link_path[256];

    //memcpy(my_link_path, target, strlen(target)+1);
    //make_directory(my_link_path, false);

    memcpy(my_link_path, linkpath, strlen(linkpath)+1);
	struct node* link = make_directory(my_link_path, false);
	link->is_symlink = true;

	int s = strlen(target) + 1;
	char* t = malloc(s * sizeof(char));
	memcpy(t, target, s);
	link->target = t;

	return 0;
}

static FILE* fp = NULL;

void print_log(char* string) {
    fp = fopen("/mnt/c/Users/burak/OneDrive/Desktop/c_rust/task2-fileio-burak00ayaz/log2.txt", "a");
    fprintf(fp, string);
    fclose(fp);
}

void open_log() {
    fp = fopen("/mnt/c/Users/burak/OneDrive/Desktop/c_rust/task2-fileio-burak00ayaz/log2.txt", "w");
    if (fp == NULL) {
        exit(-1);
    }
    fclose(fp);
}

void close_log() {
    fclose(fp);
}

//test amacli
void make_directory_test() {
    //make_directory("/benim2/main.c", true);
    //make_directory("/benim", true);
    //make_directory("/benim2/benim3", true);
    //make_directory("/benim2/benim3/Makefile", true);
    //make_directory("/", true);

    char path[256];
    struct node* ret;

    memcpy(path, "/benim2/benim4", 15);
    printf("inserting: %s\n", path);
    make_directory(path, true);
    ret = find(path); 
    printf("found: %s\n", ret->name);

    memcpy(path, "/benim2/benim4/adam", 20);
    printf("inserting: %s\n", path);
    make_directory(path, true);
    ret = find(path);
    printf("found: %s\n", ret->name);

    memcpy(path, "/yeni", 6);
    printf("inserting: %s\n", path);
    make_directory(path, true);
    ret = find(path);
    printf("found: %s\n", ret->name);

}

void make_directory_test2() {
    char path[256];
    struct node* ret;

    memcpy(path, "/foo", 5);
    printf("inserting: %s\n", path);
    make_directory(path, true);
    ret = find(path); 
    printf("found: %s\n", ret->name);

    memcpy(path, "/foo/b", 7);
    printf("inserting: %s\n", path);
    make_directory(path, true);
    ret = find(path);
    printf("found: %s\n", ret->name);
}


struct node* init() {
    char* root_name = malloc(2 * sizeof(char));
    memcpy(root_name, "/", 2);
    root = new_node(root_name);
    root->dir = true;
    return root;
}

//test amacli
struct node* create_tree() {
    if (root == NULL) {
        printf("Root has not been created yet.\n");
        return NULL;
    }

    struct node* benim = new_node("benim");
    benim->dir = true;
    struct node* benim2 = new_node("benim2");
    benim2->dir = true;

    root->child = benim;
    benim->next = benim2;

    struct node* kod = new_node("main.c");
    benim2->child = kod;

    struct node* benim3 = new_node("benim3");
    benim3->dir = true;
    kod->next = benim3;

    struct node* make = new_node("Makefile");
    benim3->child = make;

    return root;
}

//test amacli
void find_test() {
    if (root == NULL) {
        printf("Root has not been created yet.\n");
        return;
    }
    char path[256];
    struct node* ret;

    memcpy(path, "/", 2);
    printf("testing find() with path %s\n", path);
    ret = find(path);
    printf("> expected: /, return: %s\n\n", ret->name);

    memcpy(path, "//", 3);
    printf("testing find() with path %s\n", path);
    ret = find(path);
    printf("> expected: null, return: %s ??\n\n", ret->name);

    memcpy(path, "/benim", 7);
    printf("testing find() with path %s\n", path);
    ret = find(path);
    printf("> expected: benim, return: %s\n\n", ret->name);

    memcpy(path, "/benim2/main.c", 15);
    printf("testing find() with path %s\n", path);
    ret = find(path);
    printf("> expected: main.c, return: %s\n\n", ret->name);

    memcpy(path, "/benim2/benim3", 15);
    printf("testing find() with path %s\n", path);
    ret = find(path);
    printf("> expected: benim3, return: %s\n\n", ret->name);

    memcpy(path, "/benim2/benim3/Makefile", 24);
    printf("testing find() with path %s\n", path);
    ret = find(path);
    printf("> expected: Makefile, return: %s\n\n", ret->name);

    memcpy(path, "/benim2/benim3/yok", 19);
    printf("testing find() with path %s\n", path);
    ret = find(path);
    printf("> expected: null, return: %p\n\n", ret);    
}

//test amacli
void write_test() {
    //main.c ye bi seyler yaziyoruz
    struct node* testnode = root->child->next->child;
    write_to_file(testnode, "welcome\n", 9, 0);
    write_to_file(testnode, "arcraft heheheh\n", 17, 1);
}

/*
int main() {    
    init();
    create_tree();
    write_test();
    create_link("/benim2/main.c", "/shortcut");

    fclose(fp);

    
    return 0;   
}
*/