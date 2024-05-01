#include "../../includes/WebServer.hpp"
#include "../../includes/Request.hpp"
#include "../../includes/utils.hpp"

static void	hasBeenSent(ssize_t ret, ssize_t size, std::string where) {
	if (ret < 0)
		std::cerr << "[" << where << "] something went wrong with sending error" << std::endl;
	else if (ret == 0)
		std::cerr << "[" << where << "] nothing was sent... WHY?" << std::endl;
	else if (ret < size)
		std::cerr << "[" << where << "] not all bytes were sent on error" << std::endl;
}

static void	acceptClientRequest(int servFd, std::vector<struct pollfd>& pollFds) {
	int client = accept(servFd, NULL, NULL);

	if (client < 0)
		perror("accept");
	else if (pollFds.size() < CONNECTIONS) {
		struct pollfd pfd;

		pfd.fd = client;
		pfd.events = POLLIN | POLLOUT;
		pfd.revents = 0;
		pollFds.push_back(pfd);
	}
	else {
		Response resFull(HttpStatus::SERVICE_UNAVAILABLE);
		ssize_t	ret = 0;

		std::cerr << "Server full!" << std::endl;
		ret = send(client, resFull.response(), resFull.size(), MSG_CONFIRM);
		hasBeenSent(ret, resFull.size(), "new client");
		close(client);
	}
}

static void	readClientRequest(WebServer& wbserv, std::vector<struct pollfd>& pollFds, size_t pos) {
	char		buffer[BUFFER_SIZE] = {0};
	std::string totalRequest;
	ssize_t		bytesRead;

	bytesRead = recv(pollFds[pos].fd, buffer, BUFFER_SIZE, 0);
	if (bytesRead < 0) {
		Response resErr(HttpStatus::SERVER_ERR);

		std::cerr << "[recv] error" << std::endl;
		bytesRead = send(pollFds[pos].fd, resErr.response(), resErr.size(), MSG_CONFIRM);
		hasBeenSent(bytesRead, resErr.size(), "recv");
		close(pollFds[pos].fd);
		pollFds.erase(pollFds.begin() + pos);
		return ;
	}
	else if (bytesRead == 0) {
		std::cerr << "[recv] nothing was read" << std::endl;
		close(pollFds[pos].fd);
		pollFds.erase(pollFds.begin() + pos);
		return ;
	}
	buffer[bytesRead] = '\0';
	totalRequest = buffer;
	if (totalRequest.find("Expect: 100-continue") != std::string::npos) {
		std::string header(&wbserv.buffers[pollFds[pos].fd][0]);
		utils::sleep(2);
		bytesRead = recv(pollFds[pos].fd, buffer, BUFFER_SIZE, 0);
		if (bytesRead < 0) {
			Response resErr(HttpStatus::SERVER_ERR);

			std::cerr << "recv error" << std::endl;
			bytesRead = send(pollFds[pos].fd, resErr.response(), resErr.size(), MSG_CONFIRM);
			hasBeenSent(bytesRead, resErr.size(), "recv loop");
			close(pollFds[pos].fd);
			pollFds.erase(pollFds.begin() + pos);
			return ;
		}
		else if (bytesRead == 0) {
			std::cerr << "nothing was read" << std::endl;
			close(pollFds[pos].fd);
			pollFds.erase(pollFds.begin() + pos);
			return ;
		}
		buffer[bytesRead] = '\0';
		totalRequest += buffer;
	}
	wbserv.buffers[pollFds[pos].fd] = totalRequest;
}

static void	respondClientRequest(WebServer& wbserv, std::vector<struct pollfd>& pollFds, size_t pos) {
	Request	req(wbserv.buffers[pollFds[pos].fd], wbserv.servers, pollFds[pos].fd);
	Response	res = req.runRequest();
	ssize_t	ret = 0;

	ret = send(pollFds[pos].fd, res.response(), res.size(), MSG_CONFIRM);
	hasBeenSent(ret, res.size(), "response");
	close(pollFds[pos].fd);
	pollFds.erase(pollFds.begin() + pos);
}

void	WebServer::setupConfig(char* filename) {
	std::ifstream	file(filename);

	if (!file.is_open()) {
		this->servers.push_back(ServerConfig());
		this->servers.back().routes.push_back(new RouteConfig());
		return ;
	}
	addServers(file, this->servers);
}

bool	WebServer::configIsValid(char* filename) {
	std::map<std::string, Server::Keywords>	serverMap(buildServerMap());
	std::ifstream	file(filename);
	std::string	line, word;
	bool	openBrackets = false;

	if (!file.is_open())
		return false;
	while (!file.eof()) {
		std::getline(file, line);
		utils::trim(line, "\t \n");
		if (line.empty())
			continue ;
		word = line.substr(0, line.find(' '));
		if (word == "}" && !openBrackets)
			goto ret_error;
		if (word == "}") {
			openBrackets = !openBrackets;
			continue ;
		}
		if (serverMap.find(word) == serverMap.end() || serverMap.find(word)->second != Server::SERVER)
			goto ret_error;
		if (invalidServerInputs(file, line, &openBrackets, serverMap))
			goto ret_error;
	}
	file.close();
	return true;
ret_error:
	std::cerr << "bad line: `" << line << "`" << std::endl;
	file.close();
	return false;
}

void	WebServer::awaitRequest(std::vector<struct pollfd>& pollFds) {
	if (poll(&pollFds[0], pollFds.size(), POLL_TIMEOUT_SEC) == -1)
		perror("poll");
}

void	WebServer::handleRequests(WebServer& wbserv, std::vector<int>& serverFds, std::vector<struct pollfd>& pollFds) {
	for (size_t i = 0; i < pollFds.size(); i++) {
		if (pollFds[i].revents & POLLIN && i < serverFds.size()) {
			if (pollFds[i].fd == serverFds[i])
				acceptClientRequest(serverFds[i], pollFds);
		}
		else if (pollFds[i].revents & POLLIN)
			readClientRequest(wbserv, pollFds, i);
		else if (pollFds[i].revents & POLLOUT)
			respondClientRequest(wbserv, pollFds, i);
	}
}

std::ostream&	operator<<(std::ostream& o, WebServer& webserv) {
	size_t n = 1;
	for (std::vector<ServerConfig>::iterator it = webserv.servers.begin(); it != webserv.servers.end(); it++) {
		o << "Server " << n++ << ": {" << std::endl;
		o << *it;
		o << "}" << std::endl;
	}
	return o;
}
