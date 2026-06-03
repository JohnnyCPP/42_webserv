#ifndef WS_PROTOTYPES_HPP
# define WS_PROTOTYPES_HPP

int	ws_set_nonblocking(int fd);
int	ws_create_listen_socket(const std::string &host, int port);

#endif
