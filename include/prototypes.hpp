#ifndef PROTOTYPES_HPP
# define PROTOTYPES_HPP

int	ws_set_nonblocking(int fd);

int	ws_create_listen_socket(const std::string &host, int port);

int	ws_parse_request_line(const std::string &raw, t_request_line &out,
		size_t &consumed);

#endif
