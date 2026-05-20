#include "../inc/Server.hpp"

/**
 * @brief PASS command, used to set a 'connection password'
 * @note usage: PASS <password>
 * @return send ERR_NEEDMOREPARAMS, ERR_ALREADYREGISTERED or ERR_PASSWDMISMATCH in case of error
 */
void Server::handlePass(int fd, const std::vector<std::string> &args)
{
	Client	*client = _clients[fd];

	if (client->isRegistered())
	{
		sendError(fd, ERR_ALREADYREGISTERED, ":Already registered");
		return ;
	}

	if (args.size() < 2)
	{
		sendError(fd, ERR_NEEDMOREPARAMS, "PASS :Not enough parameters");
		return ;
	}

	std::string password = args[1];
	if (password != _password)
	{
		sendError(fd, ERR_PASSWDMISMATCH, ":Password incorrect");
		return ;
	}

	client->setPassOk(true);
	std::cout << "Client " << fd << " passed authentication" << std::endl;
}

static bool isSpecial(char c)
{
	return (c == '[' || c == ']' || c == '\\' || c == '`' || c == '_' ||
			c == '^' || c == '{' || c == '}' || c == '|');
}

static bool isLetter(char c)
{
	return (std::isalpha(static_cast<unsigned char>(c)));
}

static bool isDigit(char c)
{
	return (std::isdigit(static_cast<unsigned char>(c)));
}

static bool isValidNickname(const std::string &nick)
{
	if (nick.empty() || nick.size() > 9)
		return (false);

	if (!(isLetter(nick[0])) && !isSpecial(nick[0]))
		return (false);

	for (size_t i = 1; i < nick.length(); ++i)
	{
		char c = nick[i];
		if (!(isLetter(c) || isDigit(c) || isSpecial(c) || c == '-'))
			return (false);
	}
	return (true);
}

/**
 * @brief NICK command, used to give user a nickname or to change it
 * @note usage: NICK <nickname>
 * @return send ERR_NONICKNAMEGIVEN, ERR_ERRONEUSNICKNAME or ERR_NICKNAMEINUSE in case of error
 */
void Server::handleNick(int fd, const std::vector<std::string> &args)
{
	Client	*client = _clients[fd];

	if (!client->isPassOk())
		return ;

	if (args.size() < 2)
	{
		sendError(fd, ERR_NONICKNAMEGIVEN, ":No nickname given");
		return ;
	}

	std::string newNickname = args[1];

	if (client->getNickname() == newNickname)
		return;

	if (!isValidNickname(newNickname))
	{
		sendError(fd, ERR_ERRONEUSNICKNAME, newNickname + " :Erroneous nickname");
		return ;
	}

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->first != fd && it->second->getNickname() == newNickname)
		{
			sendError(fd, ERR_NICKNAMEINUSE, newNickname + " :Nickname is already in use");
			return ;
		}
	}

	std::string oldNickname = client->getNickname();

	client->setNickname(newNickname);
	client->setNickSet(true);

	if (client->isRegistered())
	{
		std::string msg = ":" + oldNickname + " NICK :" + newNickname + "\r\n";
		for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
			send(it->first, msg.c_str(), msg.size(), 0);
	}

	tryRegister(client, fd);
}

/**
 * @brief USER command, used to specify username and realname of a new user.
 * @note usage: USER <username> <unused> <unused> <realname>
 * @return send ERR_NEEDMOREPARAMS, ERR_ALREADYREGISTERED in case of error
 */
void Server::handleUser(int fd, const std::vector<std::string> &args)
{
	Client	*client = _clients[fd];

	if (!client->isPassOk())
		return ;

	if (client->isRegistered())
	{
		sendError(fd, ERR_ALREADYREGISTERED, ":Already registered");
		return ;
	}

	if (args.size() < 5)
	{
		sendError(fd, ERR_NEEDMOREPARAMS, "USER :Not enough parameters");
		return ;
	}

	std::string username = args[1];
	std::string realname = args[4];

	client->setUsername(username);
	client->setRealname(realname);
	client->setUserSet(true);

	tryRegister(client, fd);
}

/**
 * @brief CAP command, used for IRCv3 capability negotiation
 * @note usage: CAP LS|LIST|REQ|ACK|NAK|END [capabilities]
 * @note This is a minimal implementation for compatibility with modern IRC clients
 */
void Server::handleCap(int fd, const std::vector<std::string> &args)
{
	if (args.size() < 2)
		return;

	const std::string &subcommand = args[1];

	if (subcommand == "LS")
	{
		// Send empty capability list (we don't support any IRCv3 capabilities)
		std::string msg = ":ircserv CAP * LS :\r\n";
		send(fd, msg.c_str(), msg.size(), 0);
	}
	else if (subcommand == "REQ")
	{
		// Reject all capability requests
		if (args.size() >= 3)
		{
			std::string msg = ":ircserv CAP * NAK :" + args[2] + "\r\n";
			send(fd, msg.c_str(), msg.size(), 0);
		}
	}
	else if (subcommand == "END")
	{
		// Client ends capability negotiation, nothing to do
		return;
	}
	else if (subcommand == "LIST")
	{
		// Send empty list of active capabilities
		std::string msg = ":ircserv CAP * LIST :\r\n";
		send(fd, msg.c_str(), msg.size(), 0);
	}
}

/**
 * @brief PRIVMSG command, used to send messages to user/channel.
 * @note usage: PRIVMSG <msgtarget> <text to be sent>
 * @note <msgtarget> is usually the nickname or channel name
 * @return send ERR_NORECIPIENT, ERR_NOTEXTTOSEND, ERR_CANNOTSENDTOCHAN,
 * 		ERR_TOOMANYTARGETS, ERR_NOSUCHNICK in case of error, else RPL_AWAY
 */
void Server::handlePrivmsg(int fd, const std::vector<std::string> &args)
{
	Client	*client = _clients[fd];

	if (!client->isRegistered())
		return ;
	if (args.size() < 2)
	{
		sendError(fd, ERR_NORECIPIENT, ":No recipient given (PRIVMSG)");
		return ;
	}
	else if (args.size() < 3)
	{
		const std::string &param = args[1];
		if (param[0] == ':')
		{
			sendError(fd, ERR_NORECIPIENT, ":No recipient given (PRIVMSG)");
			return ;
		}
		else
		{
			sendError(fd, ERR_NOTEXTTOSEND, ":No text to send");
			return ;
		}
	}

	const std::string &target = args[1];

	std::string msg;
	for (size_t i = 2; i < args.size(); i++)
	{
		if (i > 2)
			msg += " ";
		msg += args[i];
	}

	std::string prefix = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost ";
	std::string fullMsg = prefix + "PRIVMSG " + target + " :" + msg + "\r\n";

	if (target[0] == '#')
	{
		if (_channels.find(target) == _channels.end())
		{
			sendError(fd, ERR_NOSUCHCHANNEL, target + " :No such channel");
			return ;
		}

		Channel &chan = _channels.find(target)->second;

		if (!chan.hasMember(fd))
		{
			sendError(fd, ERR_NOTONCHANNEL, target + " :You're not on that channel");
			return;
		}

		for (std::set<int>::const_iterator it = chan.getClientsFd().begin(); it != chan.getClientsFd().end(); ++it)
		{
			if (*it != fd)
				send(*it, fullMsg.c_str(), fullMsg.size(), 0);
		}
	}
	else
	{
		for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second->getNickname() == target)
			{
				send(it->first, fullMsg.c_str(), fullMsg.size(), 0);
				return ;
			}
		}
		sendError(fd, ERR_NOSUCHNICK, target + " :No such nick");
	}
}

bool isValidChanChar(char c)
{
	if (c == '\0' || c == '\a' || c == '\n' || c == '\r' || c == ' ' || c == ',' || c == ':')
		return (false);
	return (true);
}

bool Server::isValidChanName(int fd, const std::string &name)
{
	if (name[0] != '#' && name[0] != '&')
	{
		sendError(fd, ERR_BADCHANMASK, name + " :Bad Channel Mask");
		return (false);
	}

	if (name.size() < 2 || name.size() > 50)
	{
		sendError(fd, ERR_NOSUCHCHANNEL, name + " :No such channel");
		return (false);
	}

	for (size_t i = 1; i < name.length(); ++i)
	{
		char c = name[i];
		if (!isValidChanChar(c))
		{
			sendError(fd, ERR_NOSUCHCHANNEL, name + " :No such channel");
			return (false);
		}
	}
	return (true);
}

/**
 * @brief JOIN command, used to start listening to specific channel.
 * @note usage: JOIN <channel> [<key>] OR JOIN 0
 * @note 0 is used to PART from all channel
 * @return send ERR_NEEDMOREPARAMS, ERR_INVITEONLYCHAN, ERR_BADCHANNELKEY, ERR_CHANNELISFULL,
 * 		ERR_BADCHANMASK, ERR_NOSUCHCHANNEL or ERR_TOOMANYCHANNELS in case of error, else RPL_TOPIC + RPL_NAMREPLY
 */
void Server::handleJoin(int fd, const std::vector<std::string> &args)
{
	Client	*client = _clients[fd];

	if (!client->isRegistered())
		return ;

	if (args.size() < 2)
	{
		sendError(fd, ERR_NEEDMOREPARAMS, "JOIN :Not enough parameters");
		return ;
	}

	const std::string &channelName = args[1];
	std::string password = "";
	if (args.size() == 3)
		password = args[2];

	if (channelName == "0")
	{
		handleJoinZero(client);
		return ;
	}

	if (!isValidChanName(fd, channelName))
		return ;

	if (_channels.find(channelName) == _channels.end())
	{
		if (client->getChannelCount() >= MAX_CHANNELS)
		{
			sendError(fd, ERR_TOOMANYCHANNELS, channelName + " :Too many channels");
			return;
		}

		_channels.insert(std::make_pair(channelName, Channel(channelName)));
	}

	Channel &chan = _channels.find(channelName)->second;

	if (chan.hasMember(fd))
		return;

	if (chan.isInviteOnly() && !chan.isInvited(fd))
	{
		sendError(fd, ERR_INVITEONLYCHAN, channelName + " :Cannot join channel (+i)");
		return ;
	}
	if (chan.hasPass() && !(password == chan.getPassword()))
	{
		sendError(fd, ERR_BADCHANNELKEY, channelName + " :Cannot join channel (+k)");
		return ;
	}
	if (chan.hasUserLimit() && (chan.getMemberCount() + 1) > chan.getUserLimit())
	{
		sendError(fd, ERR_CHANNELISFULL, channelName + " :Cannot join channel (+l)");
		return ;
	}

	bool isFirstMember = (chan.getMemberCount() == 0);

	chan.addClient(fd);
	client->addChannel(channelName);

	if (isFirstMember)
		chan.addOperator(fd);

	std::string joinMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost JOIN " + channelName + "\r\n";
	for (std::set<int>::const_iterator it = chan.getClientsFd().begin(); it != chan.getClientsFd().end(); ++it)
		send(*it, joinMsg.c_str(), joinMsg.size(), 0);

	if (chan.getTopic().empty())
	{
		std::string msg = ":ircserv 331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n";
		send(fd, msg.c_str(), msg.size(), 0);
	}
	else
	{
		std::string msg = ":ircserv 332 " + client->getNickname() + " " + channelName + " :" + chan.getTopic() + "\r\n";
		send(fd, msg.c_str(), msg.size(), 0);
	}

	std::string names;
	for (std::set<int>::const_iterator it = chan.getClientsFd().begin(); it !=chan.getClientsFd().end(); ++it)
	{
		Client *c = _clients[*it];
		if (chan.isOperator(*it))
			names += "@";
		names += c->getNickname() + " ";
	}
	std::string rplNames = "= " + channelName + " :" + names + "\r\n";
	std::string msg353 = ":ircserv 353 " + client->getNickname() + " " + rplNames;
	send(fd, msg353.c_str(), msg353.size(), 0);

	std::string endMsg = ":ircserv 366 " + client->getNickname() + " " + channelName + " :End of list\r\n";
	send(fd, endMsg.c_str(), endMsg.size(), 0);

}

void Server::handleJoinZero(Client *client)
{
	std::set<std::string> channels = client->getChannels();
	std::vector<std::string> partArgs;
	partArgs.push_back("PART");

	for (std::set<std::string>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		std::string channel = *it;
		partArgs.push_back(channel);
		handlePart(client->getFd(), partArgs);
		partArgs.pop_back();
	}
}

/**
 * @brief QUIT command, used to terminate a client session with a message
 * @note usage: QUIT [<Quit message>]
 */
void Server::handleQuit(int fd, const std::vector<std::string> &args)
{
	Client *client = _clients[fd];

	std::string reason = "Client Quit";
	if (args.size() > 1)
		reason = args[1];
	std::string quitMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT :" + reason + "\r\n";

	std::set<int> recipients;
	std::vector<std::string> emptyChannels;

	for(std::map<std::string, Channel>::iterator chan_map = _channels.begin(); chan_map != _channels.end(); ++chan_map)
	{
		Channel &chan = chan_map->second;
		std::string channelName = chan.getName();
		if (chan.hasMember(fd))
		{
			const std::set<int> &members = chan.getClientsFd();
			for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
			{
				if (*it != fd)
					recipients.insert(*it);
			}

			chan.removeClient(fd);
			client->removeChannel(channelName);

			if (chan.getMemberCount() == 0)
				emptyChannels.push_back(channelName);
		}
	}

	for (std::vector<std::string>::iterator it = emptyChannels.begin(); it != emptyChannels.end(); ++it)
		_channels.erase(*it);

	for (std::set<int>::iterator it = recipients.begin(); it != recipients.end(); ++it)
	{
		send(*it, quitMsg.c_str(), quitMsg.size(), 0);
	}

	removeClient(fd);
}

/**
 * @brief PART command, used to be removed from the list of active members for the given channel in parameter, sending an optionnal message
 * @note usage : PART <channel> [<part message>]
 * @return ERR_NEEDMOREPARAMS, ERR_NOSUCHCHANNEL or ERR_NOTONCHANNEL in case of error
 */
void Server::handlePart(int fd, const std::vector<std::string> &args)
{
	Client *client = _clients[fd];

	if (!client->isRegistered())
		return ;

	if (args.size() < 2)
	{
		sendError(fd, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
		return ;
	}

	const std::string &channelName = args[1];

	if (_channels.find(channelName) == _channels.end())
	{
		sendError(fd, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
		return ;
	}

	Channel &chan = _channels.find(channelName)->second;

	if (!chan.hasMember(fd))
	{
		sendError(fd, ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
		return ;
	}

	std::string reason = client->getNickname();
	if (args.size() > 2)
		reason = args[2];

	std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PART " + channelName + " :" + reason + "\r\n";

	for (std::set<int>::const_iterator it = chan.getClientsFd().begin(); it != chan.getClientsFd().end(); ++it)
		send(*it, partMsg.c_str(), partMsg.size(), 0);

	chan.removeClient(fd);
	client->removeChannel(channelName);

	if (chan.getMemberCount() == 0)
		_channels.erase(channelName);
}

void Server::handlePing(int fd, const std::vector<std::string> &args)
{
	if (args.size() < 2)
	{
		sendError(fd, ERR_NOORIGIN, ":No origin specified");
		return;
	}
	const std::string &origin = args[1];
	std::string pongMsg = "PONG :" + origin + "\r\n";
	send(fd, pongMsg.c_str(), pongMsg.size(), 0);
}
