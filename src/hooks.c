#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <yasl.h>

#include "hooks.h"
#include "yaib.h"

func_node *
get_hook_callbacks(struct hook * list, size_t num_hooks, yastr hook) {
	for (size_t i = 0; i < num_hooks; i++) {
		if (strncmp(list[i].key, hook, yasllen(hook)) == 0) {
			return list[i].functions;
		} else {
			continue;
		}
	}
	return NULL;
}

func_node *
add_hook_callback(func_node * list, bool (* func)(int sockfd, struct message * msg)) {
	if (!list) { return NULL; }
	while (list->next) {
		list = list->next;
	}

	func_node * new_hook = malloc((sizeof (func_node)));
	new_hook->func = func;
	new_hook->next = NULL;

	list->next = new_hook;
	return new_hook;
}

func_node *
init_hook() {
	func_node * new_hook = malloc((sizeof (func_node)));
	new_hook->func = NULL;
	new_hook->next = NULL;
	return new_hook;
}

void
execute_callbacks(int sockfd, func_node * callbacks, struct message * msg) {
	for (; callbacks; callbacks = callbacks->next) {
		if (callbacks->func) {
			callbacks->func(sockfd, msg);
		}
	}
}

void
free_hooks(struct hook * hook_list, size_t num_hooks) {
	for (size_t i = 0; i < num_hooks; i++) {
		func_node * node = hook_list[i].functions;
		func_node * next = NULL;
		while (node->next) {
			next = node->next;
			free(node);
			node = next;
		}
		free(node);
	}
}

/*
int
main() {
	func_node welcome = {.func = NULL, .next = NULL};
	func_node join =    {.func = NULL, .next = NULL};

	add_hook_callback(&welcome, &print_hello);
	add_hook_callback(&welcome, &print_goodbye);

	struct hook hook_list[] = {
		{ "welcome", &welcome},
		{ "join",    &join},
		{ "onjoin",  NULL},
		{ "privmsg", NULL},
		{ "mode",    NULL},
	};

	func_node * callbacks = get_hook_callbacks(hook_list, yaslauto("welcome"));
	execute_callbacks(callbacks);
}
*/
