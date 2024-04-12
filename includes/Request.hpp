#pragma once
#include <iostream>
#include <list>
#include <unistd.h>
#include "Response.hpp"
#include "ServerConfig.hpp"

enum Methods {
	GET,
	POST,
	DELETE,
	UNKNOWNMETHOD
};

enum RequestLine {
	METHOD,
	REQUESTURI,
	PROTOCOLVER
};

class Request {
	private:
		ServerConfig _server;
		Response runGet();
		bool _shouldRedirect;
		RouteConfig* _route;
		std::string _reqUri;
		bool _dirListEnabled;
		/* Response runPost(); */
		/* Response runDelete(); */
	public:
		Methods method;
		std::string file;
		std::string filePath;
		Response runRequest();
		Request(std::string request, std::vector<ServerConfig> serverConfigs);
};
