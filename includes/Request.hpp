#pragma once
#include <iostream>

enum Methods {
	GET,
	POST,
	DELETE,
	INVALMETHOD
};

enum RequestLine {
	METHOD,
	REQUESTURI,
	PROTOCOLVER
};

namespace HttpStatus {
	enum Code {
		OK = 200,
		FORBIDDEN = 403,
		NOTFOUND = 404,
		NOTALLOWED = 405,
		ERROR = 1000
	};
};

class Request {
	private:
		Methods _method;
		HttpStatus::Code _httpStatusCode;
		std::string _rootPath;
		std::string _fileContent;
	public:
		Request(std::string request, std::string rootPath);
		void setStatusCode(HttpStatus::Code httpStatusCode);
		HttpStatus::Code getStatusCode();
		Methods getMethod();
		std::string getRootPath();
		void setFileContent(std::string fileContent);
		std::string getFileContent();
};
