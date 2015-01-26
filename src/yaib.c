#define _DEFAULT_SOURCE
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <yasl.h>

#include "hooks.h"
#include "yaib.h"

int32_t send_all(int, yastr, int);
ssize_t fill_buffer(int, yastr);
yastr get_line(yastr);

int32_t
send_all(int sockfd, yastr msg, int flags) {
	size_t total = 0;
	size_t bytes_left = yasllen(msg);
	ssize_t n = 0;

	while (total < yasllen(msg)) {
		n = send(sockfd, msg + total, bytes_left, flags);
		if (n == -1) { break; }
		total += (size_t)n;
		bytes_left -= (size_t)n;
	}

	return n == -1 ? -1 : 0;
}

ssize_t
fill_buffer(int sockfd, yastr read_buf) {
	ssize_t rb = recv(sockfd, read_buf + yasllen(read_buf), 512, 0);
	if (rb <= 0) {
		return rb;
	} else {
		yaslIncrLen(read_buf, (size_t)rb);
		return rb;
	}
}

void
send_registration(int sockfd, user * u) {
	yastr msg = yaslempty();

	if (u->password) {
		msg = yaslcatprintf(msg, "PASS %s\r\n", u->password);
	}
	msg = yaslcatprintf(msg, "USER %s 8 * :%s\r\nNICK %s\r\n",
	                    u->username, u->realname, u->nick);

	send_all(sockfd, msg, 0);
	yaslfree(msg);
}

yastr
get_line(yastr read_buf) {
	yastr line = yasldup(read_buf);

	char * eol = strstr(read_buf, "\r\n");
	if (eol) {
		eol += 1;
		ptrdiff_t line_len = eol - read_buf;
		yaslrange(line, 0, line_len);
		if (line_len < (ptrdiff_t)yasllen(read_buf)) {
			yaslrange(read_buf, line_len+1, -1);
		} else {
			yaslclear(read_buf);
		}
		return line;
	} else {
		yaslfree(line);
		return NULL;
	}
}

struct message *
parse_line(yastr line) {
	struct message * msg = new_message();
	yasltrim(line, "\r\n");

	if (line[0] == ':') {
		char * delim = strstr(line, " ");
		ptrdiff_t prefix_len = delim - line - 1;
		msg->prefix = yaslnew(line + 1, (size_t)prefix_len);
		yaslrange(line, prefix_len + 2, -1);
	}

	{
		char * delim = strstr(line, " ");
		ptrdiff_t prefix_len = delim - line;
		msg->command = yaslnew(line, (size_t)prefix_len);
		yaslrange(line, prefix_len + 1, -1);
	}

	{
		msg->params = yaslauto(line);
	}

	return msg;
}

user *
parse_prefix_user(yastr prefix) {
	user * u = new_user();

	u->nick = yasldup(prefix);
	char * nick_delim = strstr(u->nick, "!");
	ptrdiff_t nick_len = nick_delim - u->nick - 1;
	yaslrange(u->nick, 0, nick_len);

	u->username = yaslauto(prefix+nick_len);
	char * user_delim = strstr(u->username, "@");
	ptrdiff_t user_len = user_delim - u->username - 1;
	yaslrange(u->username, 2, user_len);

	u->host = yaslauto(prefix + nick_len + user_len);
	yaslrange(u->host, 2, -1);

	return u;
}

yastr *
parse_params_privmsg(yastr params) {
	yastr * msg = malloc((sizeof (yastr)) * 2);

	msg[0] = yasldup(params);
	char * chan_delim = strstr(msg[0], ":");
	ptrdiff_t chan_len = chan_delim - msg[0] - 2;
	yaslrange(msg[0], 0, chan_len);

	msg[1] = yasldup(params);
	yaslrange(msg[1], chan_len+3, -1);

	return msg;
}

bool
hook_welcome_join(int sockfd, struct message * msg) {
	yastr channel = yaslauto("#kyriasis");
	yastr join = yaslcatprintf(yaslempty(), "JOIN %s\r\n", channel);
	send_all(sockfd, join, 0);
	yaslfree(channel);
	yaslfree(join);
	return true;
}

bool
hook_ping_respond(int sockfd, struct message * msg) {
	yastr pong = yaslcatprintf(yaslempty(), "PONG %s\r\n", msg->params);
	printf("sending reply: %s", pong);
	send_all(sockfd, pong, 0);
	yaslfree(pong);
	return true;
}

bool
hook_privmsg_print(int sockfd, struct message * msg) {
	user * u = parse_prefix_user(msg->prefix);
	yastr * message = parse_params_privmsg(msg->params);
	printf("%s: <%s> %s\n", message[0], u->nick, message[1]);
	free_user(u);
	yaslfree(message[0]);
	yaslfree(message[1]);
	return true;
}

bool
hook_privmsg_shutdown(int sockfd, struct message * msg) {
	yastr command = yaslauto("!shutdown");
	user * u = parse_prefix_user(msg->prefix);
	yastr * message = parse_params_privmsg(msg->params);
	if (yaslcmp(command, message[1]) == 0) {
		yastr irc_msg = yaslcatprintf(yaslempty(), "QUIT :Shutdown requested by %s\r\n",
		                              u->nick);
		send_all(sockfd, irc_msg, 0);
		yaslfree(irc_msg);
		sleep(1);
		shutdown(sockfd, SHUT_WR);
	}
	yaslfree(message[0]);
	yaslfree(message[1]);
	yaslfree(command);
	free_user(u);
	return true;
}

int32_t
main() {
	func_node * welcome = init_hook();
	func_node * ping = init_hook();
	func_node * privmsg = init_hook();
	struct hook hook_list[] = {
		{ "001",      welcome },
		{ "PING",     ping    },
		{ "PRIVMSG",  privmsg },
	};
	size_t num_hooks = ((sizeof hook_list)/(sizeof hook_list[0]));
	add_hook_callback(welcome, &hook_welcome_join);
	add_hook_callback(ping, &hook_ping_respond);
	add_hook_callback(privmsg, &hook_privmsg_print);
	add_hook_callback(privmsg, &hook_privmsg_shutdown);

	yastr server = yaslauto("chat.freenode.net");
	struct addrinfo hints;
	struct addrinfo * servinfo;
	memset(&hints, 0, (sizeof hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int status = 0;
	if ((status = getaddrinfo(server, "6667", &hints, &servinfo))) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 2;
	}

	int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	status = 0;
	if ((status = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))) {
		perror("connect");
		return 2;
	}

	user * self = new_user();
	self->nick = yaslauto("Einstein");
	self->username = yaslauto("Einstein");
	self->realname = yaslauto("I’m a relly dumb bot");
	self->password = yaslauto("ComeJoinTheDarkSide42");

	send_registration(sockfd, self);

	// Read buffer to temporarily read messages into.
	yastr read_buf = yaslempty();
	read_buf = yaslMakeRoomFor(read_buf, 1024);

	for (ssize_t ret = 0; (ret = fill_buffer(sockfd, read_buf)); ) {
		if (ret <= 0) {
			perror("fill_buffer");
		}

		for (yastr line = get_line(read_buf); line; line = get_line(read_buf)) {
			yasltrim(line, "\r\n");
			struct message * msg = parse_line(line);

			func_node * callbacks = get_hook_callbacks(hook_list, num_hooks, msg->command);
			if (callbacks) {
				execute_callbacks(sockfd, callbacks, msg);
			} else {
				if (msg->prefix) { printf("prefix: %s :prefix\n", msg->prefix); }
				printf("--------------------\n"
				       "line: %s :line\n", line);
				printf("command: %s :command\n", msg->command);
				printf("params: %s :params\n\n", msg->params);
				printf("--------------------\n");
			}
			yaslfree(line);
			free_message(msg);
		}
	}

	printf("Shutting down!\n");

	free_user(self);
	yaslfree(read_buf);
	yaslfree(server);
	close(sockfd);
	freeaddrinfo(servinfo);
	free_hooks(hook_list, num_hooks);
	return 0;
}
