#ifndef YAIB_H
#define YAIB_H

#include <yasl.h>

typedef struct message {
	yastr prefix;
	yastr command;
	yastr params;
} message;

typedef struct user {
	yastr nick;
	yastr username;
	yastr host;
	yastr realname;
	yastr password;
} user;


static struct message * new_message(void);
static struct user * new_user(void);
int32_t send_all(int sockfd, yastr msg, int flags);
ssize_t fill_buffer(int sockfd, yastr read_buf);
void send_registration(int sockfd, user * u);
yastr get_line(yastr read_buf);
struct message * parse_line(yastr line);
bool handle_ping(int sockfd, struct message * msg);
bool hook_welcome_join(int sockfd, struct message * msg);
bool hook_ping_respond(int sockfd, struct message * msg);
bool hook_privmsg_print(int sockfd, struct message * msg);

user * parse_prefix_user(yastr prefix);
yastr * parse_params_privmsg(yastr params);

static inline struct message *
new_message(void) {
	struct message * new = malloc((sizeof (struct message)));
	new->prefix = NULL;
	new->command = NULL;
	new->params = NULL;
	return new;
}

static inline void
free_message(struct message * msg) {
	yaslfree(msg->prefix);
	yaslfree(msg->command);
	yaslfree(msg->params);
	free(msg);
	return;
}

static inline struct user *
new_user(void) {
	struct user * new = malloc((sizeof (struct user)));
	new->nick = NULL;
	new->username = NULL;
	new->host = NULL;
	new->realname = NULL;
	new->password = NULL;
	return new;
}

static inline void
free_user(struct user * user) {
	yaslfree(user->nick);
	yaslfree(user->username);
	yaslfree(user->host);
	yaslfree(user->realname);
	yaslfree(user->password);
	free(user);
	return;
}

#endif
