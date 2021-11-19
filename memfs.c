#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "file_tree.h"

static int my_getattr(const char *path, struct stat *st) {

    char my_path[255];
    memcpy(my_path, path, strlen(path)+1);
    //printf("copied: %s\n", my_path);

    struct node* ret = find(my_path);
    //file/dir does not exist
    if (ret == NULL) {
        return -ENOENT;
	}

	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_atime = time(NULL); 
	st->st_mtime = time(NULL); 

	//given path is a symlink
	if (ret->is_symlink == true) {
		st->st_mode = S_IFLNK | 0777;
		//TODO stmode ve stdize vermiyorum link ise

		struct node* pointed = find(ret->target);
		//file exists
		if (pointed != NULL) {
			st->st_size = strlen(ret->target);
			st->st_nlink = 1;
		}

		return 0;
	}
    //given path is a directory
    else if (ret->dir == true) {
        st->st_mode = S_IFDIR | 0777;
        st->st_nlink = 2 + number_of_children(ret);
    }
    //given path is a file
    else {
        st->st_mode = S_IFREG | 0777;
        st->st_nlink = 1;
        st->st_size = get_size(ret);
    }
    return 0;
}

static int my_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char my_path[255];
    memcpy(my_path, path, strlen(path)+1);

	struct node* temp = find(my_path);
    //dir does not exist, or its a file
    if (temp == NULL) {
        return -ENOENT;	//TODO bu hata returnu dogru mu burda, enoent file does not exist demekmis
		if (temp->dir == false) {
			return -1;
		}
	}
	
	filler(buffer, ".", NULL, 0); // Current Directory
	filler(buffer, "..", NULL, 0); // Parent Directory

	temp = temp->child;
	while (temp != NULL) {
		filler(buffer, temp->name, NULL, 0);
		temp = temp->next;
	}
	return 0;
}

static int my_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    char my_path[255];
    memcpy(my_path, path, strlen(path)+1);

	struct node* temp = find(my_path);
    //file does not exist, or its a folder
    if (temp == NULL) {
        return -ENOENT;	//TODO bu hata returnu dogru mu burda
	}
	if (temp->dir == true) {
		return -1;
	}	

	//file does not have any content
	if (temp->content == NULL) {
		return 0;
	}

	//TODO eger file okunmak istenen size kadar büyük degilse oldugu kadar okuyup returnluyorum	
	int file_size = get_size(temp);
	int bytes_to_copy;

	if (size > file_size - offset)
		bytes_to_copy = file_size - offset;
	else
		bytes_to_copy = size;

	memcpy(buffer, temp->content + offset, bytes_to_copy);
	return bytes_to_copy;
}

static int my_mkdir(const char *path, mode_t mode) {
	//TODO böyle bi directory olmadigini varsayiyorum
	char my_path[256];
    memcpy(my_path, path, strlen(path)+1);
	make_directory(my_path, true);
	return 0;
}

static int my_mknod(const char *path, mode_t mode, dev_t rdev) {
	//TODO böyle bi file olmadigini varsayiyorum
	char my_path[256];
    memcpy(my_path, path, strlen(path)+1);
	make_directory(my_path, false);
	return 0;	
}

static int my_create(const char *path, mode_t mode, struct fuse_file_info *info) {
	//TODO böyle bi file olmadigini varsayiyorum
	char my_path[256];
    memcpy(my_path, path, strlen(path)+1);
	make_directory(my_path, false);
	return 0;	
}

static int my_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info) {
    char my_path[255];
    memcpy(my_path, path, strlen(path)+1);

	struct node* temp = find(my_path);
    //file does not exist, or its a folder
    if (temp == NULL || temp->dir == true) {
        return -ENOENT;	//TODO bu hata returnu dogru mu burda	
	}

	int written = write_to_file(temp, buffer, size, offset);
	return written;
}

//TODO file yoksa acmasi gerekiyor olabilir, sonra bak
static int my_open(const char *path, struct fuse_file_info *info) {
    char my_path[255];
    memcpy(my_path, path, strlen(path)+1);

	struct node* temp = find(my_path);
    //file does not exist, or its a folder
    if (temp == NULL) {
        //return -ENOENT;	//TODO bu hata returnu dogru mu burda
		//if (temp->dir == true) {
		//	return -1;
		//}
		return my_mknod(path, 0, 0);	
	}
	return 0;
}

//TODO
static int my_readlink(const char* path, char* buf, size_t bufsize) {
    char my_path[255];
    memcpy(my_path, path, strlen(path)+1);

	struct node* temp = find(my_path);
	if (temp == NULL) {
		return -ENOENT;
	}
	if (temp->is_symlink == false) {
		return -1;
	}

	size_t path_length = strlen(temp->target);
	size_t cp = (path_length < bufsize)? path_length : bufsize;

	memcpy(buf, temp->target, cp);
	
	return 0;
}

//TODO
//symbolic link may point to an existing file or to a nonexistent one
static int my_symlink(const char *target, const char *linkpath) {
	//TODO böyle bi directory olmadigini varsayiyorum
	create_link(target, linkpath);
	return 0;
}


static struct fuse_operations operations = {
    .getattr	= my_getattr,
	.readdir    = my_readdir,
	.read       = my_read,
	.mkdir      = my_mkdir,
	.mknod      = my_mknod,
	.write      = my_write,
	.open       = my_open,
	.readlink   = my_readlink,
	.symlink    = my_symlink,
	.create     = my_create,
};

int main(int argc, char *argv[]) {
	init();
    //create_tree();
    //write_test();
    //create_link("/benim2/main.c", "/shortcut");

	//char buf[40];
	//my_readlink("/shortcut", buf, 40);
	//printf("readlink: %s\n", buf);
	int ret = fuse_main(argc, argv, &operations, NULL);
	return ret;
}