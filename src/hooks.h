#ifndef HOOKS_H
#define HOOKS_H

#include <stdbool.h>
#include <yasl.h>

#include "yaib.h"

typedef struct func_node {
	bool (* func)(int sockfd, struct message * msg);
	struct func_node * next;
} func_node;

struct hook {
	yastr key;
	func_node * functions;
};

func_node * get_hook_callbacks(struct hook * list, size_t num_hooks, yastr hook);
func_node * add_hook_callback(func_node * list, bool (* func)(int sockfd, struct message * msg));
func_node * init_hook(void);
void free_hooks(struct hook * hook_list, size_t num_hooks);
void execute_callbacks(int sockfd, func_node * callbacks, struct message * msg);

#endif
