#include "../includes/Response.hpp"

// UTILS FUNCTIONS

static std::string	getFileContent(const std::string &filename) {
	std::ifstream	file;
	std::string		content;

	file.open(filename.c_str());
	std::getline(file, content, '\0');
	file.close();
	return(content);
}

static time_t	convertTimeToGMT(time_t t) {
	struct tm	*gmtTime = gmtime(&t);
	return (mktime(gmtTime));
}

static std::string	formatTimeString(time_t	time) {
	char	buffer[80];
	std::strftime(buffer, sizeof(buffer), "%c", localtime(&time));
	std::string strTime(buffer);
	strTime += " GMT";
	return (strTime);
}

static std::string	getCurrentTimeInGMT() {
	time_t	now = convertTimeToGMT(time(0));
	return (formatTimeString(now));
}

static std::string	getLastModifiedOfFile(const std::string &filename) {
	struct stat stat_buff;
	stat(filename.c_str(), &stat_buff);
	time_t	gmtTime = convertTimeToGMT(stat_buff.st_mtime);
	return (formatTimeString(gmtTime));
}

static std::string	getFileSize(const std::string &filename) {
	struct stat	stat_buff;
	stat(filename.c_str(), &stat_buff);
	return (toString(stat_buff.st_size));
}

static std::map<std::string, std::string>	defaultMimeTypes() {
	std::map<std::string, std::string>	mimeTypes;
	mimeTypes[".html"] = "text/html";
	mimeTypes[".htm"] = "text/html";
	mimeTypes[".css"] = "text/css";
	mimeTypes[".txt"] = "text/plain";
	mimeTypes[".gif"] = "image/gif";
	mimeTypes[".png"] = "image/png";
	mimeTypes[".jpg"] = "image/jpeg";
	mimeTypes[".jpeg"] = "image/jpeg";
	mimeTypes[".php"] = "application/x-httpd-php";
	return (mimeTypes);
}

static std::map<int, std::string>	defaultStatusMessages() {
	std::map<int, std::string>	statusMessages;
	statusMessages[200] = "OK";
	statusMessages[201] = "Created";
	statusMessages[204] = "No Content";
	statusMessages[301] = "Moved Permanently";
	statusMessages[302] = "Found";
	statusMessages[304] = "Not Modified";
	statusMessages[400] = "Bad Request";
	statusMessages[401] = "Unauthorized";
	statusMessages[403] = "Forbidden";
	statusMessages[404] = "Not Found";
	statusMessages[405] = "Method Not Allowed";
	statusMessages[408] = "Request Timeout";
	statusMessages[500] = "Internal Server Error";
	statusMessages[502] = "Bad Gateway";
	statusMessages[503] = "Service Unavailable";
	return (statusMessages);
}

// CONSTRUCTORS

Response::Response() {
	this->_body = "";
	this->_status = 0;
	this->_bodyFile = "";
	this->_statusLine = "";
	this->_mimeTypes = defaultMimeTypes();
	this->_statusMessages = defaultStatusMessages();
}

Response::Response(short int status, std::string bodyFile) {
	this->_status = status;
	this->_bodyFile = bodyFile;
	this->_mimeTypes = defaultMimeTypes();
	this->_statusMessages = defaultStatusMessages();
	this->defineStatusLine(status);
	switch (status / 100) { 
		// case 1: // 1xx
		//	this->_informatiol
		// 	break ;
		case 2: // 2xx
			this->_success();
			break ;
		case 3: // 3xx
			this->_redirection();
			break ;
		case 4: // 4xx
			this->_error();
			break ;
		case 5: // 5xx
			this->_serverError();
			break ;
	}
	this->generateFullResponse();
}

Response::Response(short int status) {
	this->_status = status;
}

Response::~Response() {}

// GETTERS AND SETTERS

std::string	Response::response() const {
	return (this->_fullResponse);
}

std::string	Response::getContentType(const std::string &filename) const {
	std::map<std::string, std::string>::const_iterator	it;
	std::string	extension = filename.substr(filename.find_last_of("."));
	it = this->_mimeTypes.find(extension);
	if (it != this->_mimeTypes.end())
		return (it->second);
	else
		return ("Content Type Unknown");
}

std::string	Response::getStatusMessage(int status) const {
	std::map<int, std::string>::const_iterator	it;
	std::string	statusMessage;
	it = this->_statusMessages.find(status);
	if (it != this->_statusMessages.end())
		statusMessage = toString(status) + " " + it->second;
	else
		statusMessage = toString(status) + " Unknown Status";
	return (statusMessage);
}

// MEMBERS FUNCTIONS

void	Response::generateFullResponse() {
	std::vector<t_fields>::iterator	it;

	this->_fullResponse += this->_statusLine + "\n";
	for (it = this->_headerFields.begin(); it != this->_headerFields.end(); it++)
		this->_fullResponse += it->first + ": " + it->second + "\n";
	this->_fullResponse += '\n' + this->_body;
}

void	Response::defineStatusLine(int status) {
	this->_statusLine = HTTP_VERSION + (" " + getStatusMessage(status));
}

void	Response::addNewField(std::string key, std::string value) {
	this->_headerFields.push_back(std::make_pair(key, value));
}

void	Response::_success() {
	this->addNewField("Date", getCurrentTimeInGMT());
	this->addNewField("Server", SERVER_NAME);
	this->addNewField("Etag", "* hash function *");
	switch (this->_status) {
		case 200:
			this->addNewField("Last-Modified", getLastModifiedOfFile(this->_bodyFile));
			this->addNewField("Content-Lenght", getFileSize(this->_bodyFile));
			this->addNewField("Content-Type", getContentType(this->_bodyFile));
			this->_body = getFileContent(this->_bodyFile);
			break ;
		case 201:
			this->addNewField("Location:", "/path/to/some?");
			this->_body = getFileContent(this->_bodyFile);
			break ;
		case 204:
			break ;
	}
	// this->_header.push_back(std::make_pair("Connection", "keep-alive"));
}

void	Response::_redirection() {
	this->addNewField("Date", getCurrentTimeInGMT());
	this->addNewField("Server", SERVER_NAME);
	this->addNewField("Etag", "* hash function *");
	switch (this->_status) {
		case 301:
			this->addNewField("Location:", "/path/to/some?");
			break ;
		case 302:
			this->addNewField("Location:", "/path/to/some?");
			break ;
		case 304:
			break ;
	}
}

void	Response::_error() {
	this->addNewField("Date", getCurrentTimeInGMT());
	this->addNewField("Server", SERVER_NAME);
	this->addNewField("Etag", "* hash function *");
	this->addNewField("Content-Lenght", getFileSize(this->_bodyFile));
	this->addNewField("Content-Type", getContentType(this->_bodyFile));
	this->_body = getFileContent(this->_bodyFile);
	// switch (this->_status) {
	// 	case 400:
	// 		break ;
	// 	case 401:
	// 		break ;
	// 	case 403:
	// 		break ;
	// 	case 404:
	// 		break ;
	// 	case 405:
	// 		break ;
	// 	case 408:
	// 		break ;
	// }
}

void	Response::_serverError() {
	return ;
}
