#include "../inc/Server.hpp"

void Server::sendWelcome(int fd)
{
	Client *client = _clients[fd];
	std::string msg = ":ircserv 001 " + client->getNickname() + " :Welcome to IRC\r\n";
	send(fd, msg.c_str(), msg.size(), 0);
}

void Server::sendError(int fd, const std::string &code, const std::string &msg)
{
	std::map<int, Client*>::iterator it = _clients.find(fd);
	std::string nickname = "*";
	
	if (it != _clients.end() && it->second->isNickSet())
		nickname = it->second->getNickname();
	
	std::string errmsg = ":ircserv " + code + " " + nickname + " " + msg + "\r\n";
	std::cout << "Sent to " << fd << ": " << errmsg << std::flush;
	send(fd, errmsg.c_str(), errmsg.size(), 0);
}

void Server::tryRegister(Client *client, int fd)
{
	if (!client->isRegistered() && client->isPassOk() && client->isNickSet() && client->isUserSet())
	{
		client->setRegistered(true);
		sendWelcome(fd);
	}
}
