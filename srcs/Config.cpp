#include "../includes/Config.hpp"
#include "../includes/utils.hpp"
#include <fstream>

const char* ServerNotFound::what() const throw()
{
	return ("This server don't exist.");
}

std::map<std::string, ServerKeywords>	buildServerMap();
std::map<std::string, RouteKeywords>	buildRouteMap();
void	checkInsideRoute(std::ifstream& file, std::string& line) \
throw(std::runtime_error);
ServerConfig*	searchViaName(std::string const name, \
	unsigned int const port, \
	std::vector<ServerConfig>& servers);
ServerConfig&	searchViaHost(std::string const& host, \
	unsigned int const port, \
	std::vector<ServerConfig>& servers)
throw(ServerNotFound);
bool	invalidServerInputs(std::ifstream& file, \
	std::string& line, \
	bool* serverBrackets, \
	std::map<std::string, ServerKeywords>& serverMap);

/* METHODS ================================================================= */

ServerConfig&	Config::findByHostNamePort(std::string const& host, \
	std::string const* names, \
	size_t const size, \
	unsigned int const port)
const throw(ServerNotFound)
{
	ServerConfig* foundConfig = NULL;
	if (!host.empty()) {
		return searchViaHost(host, port, \
			const_cast<std::vector<ServerConfig>&>(this->_servers));
	} else if (names == NULL) {
		throw ServerNotFound();
	}
	for (size_t i = 0; foundConfig != NULL && i < size; i++) {
		foundConfig = searchViaName(names[i], port, \
			const_cast<std::vector<ServerConfig>&>(this->_servers));
	}
	if (foundConfig != NULL) {
		return *foundConfig;
	}
	throw ServerNotFound();
}

void	Config::addServers(const char* filename) {(void)filename;}

bool	Config::configIsValid(const char* filename)
{
	std::map<std::string, ServerKeywords> serverMap(buildServerMap());
	std::ifstream	file(filename);
	std::string line;
	std::string word;
	bool openBrackets = false;

	if (!file.is_open())
		return false;
	while (!file.eof()) {
		std::getline(file, line);
		trim(line, "\t \n");
		if (line.empty())
			continue ;
		word = line.substr(0, line.find_first_of(" \t\n"));
		if (word[0] == '}' && !openBrackets)
			goto ret_error;
		if (word == "}") {
			openBrackets = !openBrackets;
			continue ;
		}
		if (serverMap.find(word) == serverMap.end())
			goto ret_error;
		if (serverMap.find(word)->second != SERVER)
			goto ret_error;
		if (serverMap.find(word)->second == SERVER && \
			invalidServerInputs(file, line, &openBrackets, serverMap))
			goto ret_error;
	}
	file.close();
	std::cout << "acho q tá tudo ok" << std::endl;
	return true;
ret_error:
	std::cout << "opa essa linha deu ruim irmao: `" << line << "`" << std::endl;
	line.clear();
	file.close();
	return false;
}
