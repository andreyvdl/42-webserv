#include "../includes/Response.hpp"

static std::string	generateDirectoryListing(std::string &path, std::string &requestUri) {
	std::string	dirListing;
	DIR	*dir = opendir(path.c_str());

	if (!utils::strEndsWith(requestUri, '/'))
		requestUri += "/";
	if (!utils::strEndsWith(path, '/'))
		path += "/";
	dirListing += "<html><head><title>Directory listing for " + requestUri + "</title></head><body>";
	dirListing += "<h1>Directory listing for " + requestUri + "</h1>";
	dirListing += "<hr><pre>";
	dirListing += "<a href=\"..\">../</a>\n";
	for (struct dirent *item = readdir(dir); item != NULL; item = readdir(dir)) {
		if (std::strcmp(item->d_name, ".") == 0 || std::strcmp(item->d_name, "..") == 0)
			continue ;

		std::string modTime, bytesSize, file = item->d_name;
		std::string realPath;
		std::string	uriPath;

		realPath = path + item->d_name;
		uriPath = requestUri + item->d_name;

		if (S_ISDIR(utils::pathInfo(realPath).st_mode)) {
			uriPath += "/";
		}

		utils::getDateAndBytes(realPath, modTime, bytesSize);
		dirListing += "<a href=\"" + uriPath + "\">" + file;
		if (item->d_type == DT_DIR) {
			dirListing += "/";
			dirListing += "</a>";
			dirListing.append(255 - file.size() - 1, ' ');
			dirListing += modTime;
			dirListing.append(19, ' ');
			dirListing += "-\n";
		} else {
			dirListing += "</a>";
			dirListing.append(255 - file.size(), ' ');
			dirListing += modTime;
			dirListing.append(20 - bytesSize.size(), ' ');
			dirListing += bytesSize + "\n";
		}
	}
	closedir(dir);
	dirListing += "</pre><hr></body></html>";
	return (dirListing);
}

static std::string	getErrInformation(HttpStatus::Code status)
{
	switch (status) {
		case HttpStatus::BAD_REQUEST:
			return (E400);
		case HttpStatus::FORBIDDEN:
			return (E403);
		case HttpStatus::NOT_FOUND:
			return (E404);
		case HttpStatus::NOT_ALLOWED:
			return (E405);
		case HttpStatus::TIMEOUT:
			return (E408);
		case HttpStatus::CONFLICT:
			return (E409);
		case HttpStatus::PAYLOAD_TOO_LARGE:
			return (E413);
		case HttpStatus::SERVER_ERR:
			return (E500);
		case HttpStatus::NOT_IMPLEMENTED:
			return (E501);
		case HttpStatus::SERVICE_UNAVAILABLE:
			return (E503);
		default:
			return ("");
	}
}

static std::string	generateDefaultErrorPage(HttpStatus::Code status, const std::string &statusMsg) {
	std::string	errorPage = "";

	errorPage += "<html>\n";
	errorPage += "<head><title>" + statusMsg + "</title></head>\n";
	errorPage += "<body>\n";
	errorPage += "<h1>" + statusMsg + "</h1>\n";
	errorPage += "<p>" + getErrInformation(status) + "</p>\n";
	errorPage += "</body>\n";
	errorPage += "</html>\n";

	return (errorPage);
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

	statusMessages[HttpStatus::OK] = "OK";
	statusMessages[HttpStatus::CREATED] = "Created";
	statusMessages[HttpStatus::NO_CONTENT] = "No Content";
	statusMessages[HttpStatus::MOVED_PERMANENTLY] = "Moved Permanently";
	statusMessages[HttpStatus::BAD_REQUEST] = "Bad Request";
	statusMessages[HttpStatus::FORBIDDEN] = "Forbidden";
	statusMessages[HttpStatus::NOT_FOUND] = "Not Found";
	statusMessages[HttpStatus::NOT_ALLOWED] = "Method Not Allowed";
	statusMessages[HttpStatus::TIMEOUT] = "Request Timeout";
	statusMessages[HttpStatus::CONFLICT] = "Conflict";
	statusMessages[HttpStatus::PAYLOAD_TOO_LARGE] = "Payload Too Large";
	statusMessages[HttpStatus::SERVER_ERR] = "Internal Server Error";
	statusMessages[HttpStatus::NOT_IMPLEMENTED] = "Not Implemented";
	statusMessages[HttpStatus::SERVICE_UNAVAILABLE] = "Service Unavailable";

	return (statusMessages);
}

static std::string	getErrorPage(int status, ServerConfig &server) {
	std::string *errPagePath;

	errPagePath = server.getFilePathFromStatusCode(status);
	if (!errPagePath)
		return ("");
	switch (utils::checkPath(*errPagePath)) {
		case ENOTDIR:
			return (*errPagePath);
		default:
			return ("");
	}
}

Response::Response(HttpStatus::Code status, Request &request) : _status(status), _filePath(request.filePath), _requestUri(request._reqUri), _locationHeader("") {
	this->_errPage = getErrorPage(status, request._server);
	this->_mimeTypes = defaultMimeTypes();
	this->_statusMessages = defaultStatusMessages();
	if (request._shouldRedirect)
		this->_locationHeader = request._locationHeader;
	this->defineStatusLine(status);
	switch (status / 100) {
		case 2:
			this->_success(request);
			break ;
		case 3:
			this->_redirection();
			break ;
		case 4:
			this->_error();
			break ;
		case 5:
			this->_serverError();
			break ;
	}
	this->generateFullResponse();
}

Response::Response(HttpStatus::Code status) : _status(status), _body(""), _filePath(""), _errPage("") {
	this->_mimeTypes = defaultMimeTypes();
	this->_statusMessages = defaultStatusMessages();
	this->defineStatusLine(status);
	switch (status / 100) {
		case 4:
			this->_error();
			break ;
		case 5:
			this->_serverError();
			break ;
	}
	this->generateFullResponse();
}

Response	&Response::operator=(const Response &other) {
	if (this != &other) {
		this->_body = other._body;
		this->_status = other._status;
		this->_filePath = other._filePath;
		this->_mimeTypes = other._mimeTypes;
		this->_statusLine = other._statusLine;
		this->_headerFields = other._headerFields;
		this->_fullResponse = other._fullResponse;
		this->_statusMessages = other._statusMessages;
		this->_requestUri = other._requestUri;
		this->_locationHeader = other._locationHeader;
		this->_errPage = other._errPage;
	}
	return (*this);
}

std::string	Response::getFullResponse() const {
	return (this->_fullResponse);
}

const char	*Response::response() const {
	return (this->_fullResponse.c_str());
}

size_t		Response::size() const {
	return (this->_fullResponse.length());
}

std::string	Response::getContentType(const std::string &filename) const {
	std::map<std::string, std::string>::const_iterator	it;
	std::string	extension;

	if (filename.empty())
		return ("text/plain");
	if (filename.find_last_of(".") == std::string::npos)
		return (*(filename.end() - 1) == '/' ? "text/html" : "text/plain");
	extension = filename.substr(filename.find_last_of("."));
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
		statusMessage = utils::toString(status) + " " + it->second;
	else
		statusMessage = utils::toString(status) + " Unknown Status";
	return (statusMessage);
}

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

void	Response::_success(Request &req) {
	this->addNewField("Date", utils::getCurrentTimeInGMT());
	this->addNewField("Server", SERVER_NAME);
	if (this->_filePath.empty())
		return ;
	switch (this->_status) {
		case 200:
			if (!S_ISDIR(utils::pathInfo(this->_filePath).st_mode)) {
				if (req.execCgi) {
					this->_body = req.cgiOutput;
					this->addNewField("Content-Length", utils::toString(this->_body.size()));
					this->addNewField("Content-Type", req.resContentType);
				}
				else {
					this->addNewField("Last-Modified", utils::getLastModifiedOfFile(this->_filePath));
					this->addNewField("Content-Length", utils::getFileSize(this->_filePath));
					this->addNewField("Content-Type", getContentType(this->_filePath));
					this->_body = utils::getFileContent(this->_filePath);
				}
			} else {
				this->addNewField("Content-Type", "text/html");
				this->_body = generateDirectoryListing(this->_filePath, this->_requestUri);
			}
			break ;
		case 201:
			this->addNewField("Content-Type", getContentType(this->_filePath));
			this->_body = "File created";
			break ;
		default:
			break ;
	}
}

void	Response::_redirection() {
	this->addNewField("Date", utils::getCurrentTimeInGMT());
	this->addNewField("Server", SERVER_NAME);
	addNewField("Location", this->_locationHeader);
}

void	Response::_error() {
	this->addNewField("Date", utils::getCurrentTimeInGMT());
	this->addNewField("Server", SERVER_NAME);
	if (!this->_errPage.empty()) {
		this->_body = utils::getFileContent(this->_errPage);
		this->addNewField("Content-Length", utils::getFileSize(this->_errPage));
		this->addNewField("Content-Type", getContentType(this->_errPage));
	} else {
		std::string statusMsg = getStatusMessage(this->_status);
		this->_body = generateDefaultErrorPage(this->_status, statusMsg);
		this->addNewField("Content-Length", utils::toString(this->_body.size()));
		this->addNewField("Content-Type", "text/html");
	}
}

void	Response::_serverError() {
	this->addNewField("Date", utils::getCurrentTimeInGMT());
	this->addNewField("Server", SERVER_NAME);
	if (!this->_errPage.empty()) {
		this->_body = utils::getFileContent(this->_errPage);
		this->addNewField("Content-Length", utils::getFileSize(this->_errPage));
		this->addNewField("Content-Type", getContentType(this->_errPage));
	} else {
		std::string statusMsg = getStatusMessage(this->_status);
		this->_body = generateDefaultErrorPage(this->_status, statusMsg);
		this->addNewField("Content-Length", utils::toString(this->_body.size()));
		this->addNewField("Content-Type", "text/html");
	}
}
