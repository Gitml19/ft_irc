#ifndef SERVER_HPP
#define SERVER_HPP

#define MAX_CHANNELS 10

#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"
#include "../inc/ErrorCode.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <csignal>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <cctype>

class Client;
class Channel;

extern volatile sig_atomic_t g_running;

class Server
{
	public:
		Server(int port, const std::string &password);
		~Server();
		void run();

	private:
		int									_serverFd;
		std::vector<struct pollfd>			_pollfds;
		std::map<int, Client*>				_clients;
		int									_port;
		std::string							_password;
		std::map<std::string, Channel>		_channels;

		void		acceptClient();
		void		removeClient(int fd);
		static void	setNonBlocking(int fd);
		void	handleClient(int fd);
		void	execCommand(const std::string cmd, int fd, std::vector<std::string> &tokens);
		void	processMessage(int fd, const std::string &msg);

		/*		ServerCommand.cpp		*/
		void handlePass(int fd, const std::vector<std::string> &args);
		void handleNick(int fd, const std::vector<std::string> &args);
		void handleUser(int fd, const std::vector<std::string> &args);
		void handleCap(int fd, const std::vector<std::string> &args);
		void handlePrivmsg(int fd, const std::vector<std::string> &args);
		void handleJoin(int fd, const std::vector<std::string> &args);
		void handleJoinZero(Client *client);
		void handleQuit(int fd, const std::vector<std::string> &args);

		void handlePart(int fd, const std::vector<std::string> &args);
		void handleTopic(int fd, const std::vector<std::string> &args);
		void handleKick(int fd, const std::vector<std::string> &args);
		void handleInvite(int fd, const std::vector<std::string> &args);
		void handleMode(int fd, const std::vector<std::string> &args);
		void handlePing(int fd, const std::vector<std::string> &args);

		bool isValidChanName(int fd, const std::string &name);

		/*		ServerMsg.cpp		*/
		void sendWelcome(int fd);
		void sendError(int fd, const std::string &code, const std::string &msg);
		void tryRegister(Client *client, int fd);
};

#endif