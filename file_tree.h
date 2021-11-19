
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

//DEBUGGING
void open_log();
void close_log();
void print_log(char* string);

struct node* init();

struct node* make_directory(char* given_path, bool is_dir);

int create_link(const char* target, const char* linkpath);

int write_to_file(struct node* file, const char *new_content, size_t size, off_t offset);

//test
struct node* create_tree();

//test
void write_test();

struct node* find(char* path);

int number_of_children(struct node* parent);

int get_size(struct node* file);