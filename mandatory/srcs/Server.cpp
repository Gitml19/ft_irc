#include "../inc/Server.hpp"

volatile sig_atomic_t g_running = 1;

static void signalHandler(int sig)
{
	(void)sig;
	g_running = 0;
	std::cout << "\nShutting down..." << std::endl;
}

void Server::setNonBlocking(int fd)
{
	// int flags = fcntl(fd, F_GETFL, 0);
	// if (flags == -1)
	// 	throw std::runtime_error("fcntl F_GETFL failed");
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("fctl F_SETFL failed");
}

Server::Server(int port, const std::string &password) : _port(port), _password(password)
{
	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverFd < 0)
		throw std::runtime_error("Failed to create socket");

	int opt = 1;
	setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("Failed to bind socket");
	if (listen(_serverFd, SOMAXCONN) < 0)
		throw std::runtime_error("Failed to listen socket");

	setNonBlocking(_serverFd);

	struct pollfd p;
	p.fd = _serverFd;
	p.events = POLLIN;
	p.revents = 0;
	_pollfds.push_back(p);

	std::cout << "Server listening on port " << port << std::endl;
}

Server::~Server()
{
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		delete it->second;
	_clients.clear();
	close(_serverFd);
}

void Server::run()
{
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	std::cout << "Server starting main event loop..." << std::endl;

	while (g_running)
	{
		int ret = poll(&_pollfds[0], _pollfds.size(), -1);
		if (ret < 0)
		{
			if (!g_running)
				break;
			std::cerr << "poll() error" << std::endl;
			break;
		}

		for (size_t i = 0; i < _pollfds.size(); i++)
		{
			if (_pollfds[i].revents == 0)
				continue;

			int fd = _pollfds[i].fd;

			if (fd == _serverFd)
				acceptClient();

			else
				handleClient(fd);
		}
	}
}

void Server::acceptClient()
{
	int client_fd = accept(_serverFd, NULL, NULL);
	if (client_fd < 0)
	{
		std::cerr << "accept() failed" << std::endl;
		return;
	}

	setNonBlocking(client_fd);

	Client *client = new Client(client_fd);
	_clients[client_fd] = client;

	struct pollfd p;
	p.fd = client_fd;
	p.events = POLLIN;
	p.revents = 0;
	_pollfds.push_back(p);

	std::cout << "New client connected: fd " << client_fd << std::endl;
}

void Server::removeClient(int fd)
{
	std::map<int, Client *>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;

	Client *client = it->second;

	std::set<int> recipients;
	std::vector<std::string> emptyChannels;

	if (client->isRegistered())
	{
		std::string quitMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT :Connection closed\r\n";

		for (std::map<std::string, Channel>::iterator ch = _channels.begin(); ch != _channels.end(); ++ch)
		{
			Channel &chan = ch->second;
			if (chan.hasMember(fd))
			{
				const std::set<int> &members = chan.getClientsFd();
				for (std::set<int>::const_iterator mem = members.begin(); mem != members.end(); ++mem)
				{
					if (*mem != fd)
						recipients.insert(*mem);
				}

				chan.removeClient(fd);
				client->removeChannel(ch->first);

				if (chan.getMemberCount() == 0)
					emptyChannels.push_back(ch->first);
			}
		}

		for (std::set<int>::iterator rec = recipients.begin(); rec != recipients.end(); ++rec)
		{
			send(*rec, quitMsg.c_str(), quitMsg.size(), 0);
		}
	}
	else
	{
		for (std::map<std::string, Channel>::iterator ch = _channels.begin(); ch != _channels.end(); ++ch)
		{
			Channel &chan = ch->second;
			if (chan.hasMember(fd))
			{
				chan.removeClient(fd);
				if (chan.getMemberCount() == 0)
					emptyChannels.push_back(ch->first);
			}
		}
	}

	for (std::vector<std::string>::iterator emp = emptyChannels.begin(); emp != emptyChannels.end(); ++emp)
		_channels.erase(*emp);

	for (size_t i = 0; i < _pollfds.size(); i++)
	{
		if (_pollfds[i].fd == fd)
		{
			_pollfds.erase(_pollfds.begin() + i);
			break;
		}
	}

	// close(fd);
	delete client;
	_clients.erase(it);

	std::cout << "Client disconnected: fd " << fd << std::endl;
}

void Server::handleClient(int fd)
{
	char buf[512];
	std::memset(buf, 0, sizeof(buf));
	int n = recv(fd, buf, sizeof(buf) - 1, 0);

	if (n < 0)
	{
		removeClient(fd);
		return;
	}
	else if (n == 0)
	{
		removeClient(fd);
		return;
	}

	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;

	Client *client = it->second;
	buf[n] = '\0';
	client->_buffer.append(buf, n);

	size_t pos;
	while ((pos = client->_buffer.find("\n")) != std::string::npos)
	{
		std::string line = client->_buffer.substr(0, pos);
		client->_buffer.erase(0, pos + 1);

		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		if (line.size() > 510)
			line = line.substr(0, 510);

		if (!line.empty())
			processMessage(fd, line);

		if (_clients.find(fd) == _clients.end())
			return;
	}

	if (client->_buffer.size() > 512)
		client->_buffer.erase(510);
}

void parseMessage(std::vector<std::string> &tok, std::string &line)
{
	bool hasTrailing = false;
	std::string trailing;

	size_t pos = line.find(" :");
	if (pos != std::string::npos)
	{
		hasTrailing = true;
		trailing = line.substr(pos + 2);
		line = line.substr(0, pos);
	}

	std::istringstream iss(line);
	std::string word;
	while (iss >> word)
		tok.push_back(word);
	if (hasTrailing)
		tok.push_back(trailing);
}

/**
 * @brief Execute the server/channel command
 * @param cmd (Server) PASS | NICK | USER | JOIN | PRIVMSG | QUIT | PART
 * @param cmd (Channel) KICK | INVITE | TOPIC | MODE
 * @param fd Client's fd
 * @param tokens parameters for commands
 * @return Send error if the command is unknown
 */
void Server::execCommand(const std::string cmd, int fd, std::vector<std::string> &tokens)
{
	if (cmd == "CAP")
		handleCap(fd, tokens);
	else if (cmd == "PASS")
		handlePass(fd, tokens);
	else if (cmd == "NICK")
		handleNick(fd, tokens);
	else if (cmd == "USER")
		handleUser(fd, tokens);
	else if (cmd == "JOIN")
		handleJoin(fd, tokens);
	else if (cmd == "PRIVMSG")
		handlePrivmsg(fd, tokens);
	else if (cmd == "QUIT")
		handleQuit(fd, tokens);
	else if (cmd == "PART")
		handlePart(fd, tokens);
	else if (cmd == "KICK")
		handleKick(fd, tokens);
	else if (cmd == "INVITE")
		handleInvite(fd, tokens);
	else if (cmd == "TOPIC")
		handleTopic(fd, tokens);
	else if (cmd == "MODE")
		handleMode(fd, tokens);
	else if (cmd == "PING")
		handlePing(fd, tokens);
	else
		sendError(fd, ERR_UNKNOWNCOMMAND, "Unknown command");
}

void Server::processMessage(int fd, const std::string &msg)
{
	std::cout << "Received from " << fd << ": " << msg << std::endl << std::flush;

	std::string line = msg;
	if (!line.empty() && line[line.size() - 1] == '\n')
		line.erase(line.size() - 1);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	std::vector<std::string> tokens;
	parseMessage(tokens, line);
	if (tokens.empty())
		return ;
	std::string command = tokens[0];
	execCommand(command, fd, tokens);
}
