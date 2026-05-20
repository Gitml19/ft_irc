#include "../inc/Server.hpp"

/**
 * @brief KICK command, used to request the forced removal of a user from a channel. It causes the <user> to PART by force
 * @note usage : KICK <channel> <user> [<comment>]
 * @note if <comment> is given, it replaces the default message sent
 * @return ERR_NEEDMOREPARAMS, ERR_NOSUCHCHANNEL, ERR_BADCHANMASK, ERR_CHANOPRIVSNEEDED,
 * ERR_USERNOTINCHANNEL or ERR_NOTONCHANNEL in case of error
 */
void Server::handleKick(int fd, const std::vector<std::string> &args)
{
	Client *client = _clients[fd];

	if (!client->isRegistered())
		return ;

	if (args.size() < 3)
	{
		sendError(fd, ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");
		return ;
	}

	const std::string &channelName = args[1];
	const std::string &targetNick = args[2];

	if (!isValidChanName(fd, channelName))
		return;

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

	if (!chan.isOperator(fd))
	{
		sendError(fd, ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return ;
	}

	int targetFd = -1;
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickname() == targetNick)
		{
			targetFd = it->first;
			break;
		}
	}

	if (targetFd == -1)
	{
		sendError(fd, ERR_NOSUCHNICK, targetNick + " :No such nick/channel");
		return ;
	}

	if (!chan.hasMember(targetFd))
	{
		sendError(fd, ERR_USERNOTINCHANNEL, targetNick + " " + channelName + " :They aren't on that channel");
		return ;
	}

	std::string reason = targetNick;
	if (args.size() > 3 && !args[3].empty())
		reason = args[3];

	std::string kickMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";

	const std::set<int> &members = chan.getClientsFd();
	for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		send(*it, kickMsg.c_str(), kickMsg.length(), 0);
	}

	chan.removeClient(targetFd);
	_clients[targetFd]->removeChannel(channelName);

	if (chan.getMemberCount() == 0)
		_channels.erase(channelName);
}

/**
 * @brief INVITE command, used to invite a user to a channel
 * @note usage : INVITE <nickname> <channel>
 * @note No requirement that the channel must exist.
 * @note However, if the channel exists, only members of the channel are allowed to INVITE.
 * @note When invite-only is set in the channel, only channel operators may issue INVITE command.
 * @return ERR_NEEDMOREPARAMS, ERR_NOSUCHNICK, ERR_NOTONCHANNEL, ERR_USERONCHANNEL, ERR_CHANOPRIVSNEEDED in case of error,
 * else RPL_INVITING and RPL_AWAY
 */
void Server::handleInvite(int fd, const std::vector<std::string> &args)
{
	Client *client = _clients[fd];

	if (!client->isRegistered())
		return ;

	if (args.size() < 3)
	{
		sendError(fd, ERR_NEEDMOREPARAMS, "INVITE :Not enough parameters");
		return ;
	}

	const std::string &targetNick = args[1];
	const std::string &channelName = args[2];

	if (_channels.find(channelName) == _channels.end())
	{
		sendError(fd, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
		return ;
	}

	Channel &chan = _channels.find(channelName)->second;

	if (!chan.hasMember(fd))
	{
		sendError(fd, ERR_NOTONCHANNEL, channelName + " :You're not on the channel");
		return ;
	}

	if (chan.isInviteOnly() && !chan.isOperator(fd))
	{
		sendError(fd, ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return ;
	}

	int targetFd = -1;
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickname() == targetNick)
		{
			targetFd = it->first;
			break;
		}
	}

	if (targetFd == -1)
	{
		sendError(fd, ERR_NOSUCHNICK, targetNick + " :No suchnick/channel");
		return ;
	}

	if (chan.hasMember(targetFd))
	{
		sendError(fd, ERR_USERONCHANNEL, targetNick + " " + channelName + " :is already on channel");
		return ;
	}

	chan.addInvite(targetFd);

	std::string inviteMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost INVITE " + targetNick + " :" + channelName + "\r\n";
	send(targetFd, inviteMsg.c_str(), inviteMsg.length(), 0);

	std::string confirmMsg = ":ircserv 341 " + client->getNickname() + " " + targetNick + " " + channelName + "\r\n";
	send(fd, confirmMsg.c_str(), confirmMsg.length(), 0);
}

/**
 * @brief TOPIC command, used to change or view the topic of a channel
 * @note usage : TOPIC <channel> [<topic>]
 * @note If <topic> is present, the topic for <channel> will be changed. If <topic> is empty, the topic will be removed
 * @return If there is no <topic>, the topic for <channel> is returned.
 * @return ERR_NEEDMOREPARAMS, ERR_NOSUCHCHANNEL or ERR_NOTONCHANNEL in case of error
 */
void Server::handleTopic(int fd, const std::vector<std::string> &args)
{
	Client *client = _clients[fd];

	if (!client->isRegistered())
		return ;

	if (args.size() < 2)
	{
		sendError(fd, ERR_NEEDMOREPARAMS, "TOPIC :Not enough parameters");
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

	if (args.size() == 2)
	{
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
		return ;
	}

	if (chan.isTopicRestricted() && !chan.isOperator(fd))
	{
		sendError(fd, ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return;
	}

	std::string newTopic = args[2];
	chan.setTopic(newTopic);

	std::string topicMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost TOPIC " + channelName + " :" + newTopic + "\r\n";
	for (std::set<int>::const_iterator it = chan.getClientsFd().begin(); it != chan.getClientsFd().end(); ++it)
		send(*it, topicMsg.c_str(), topicMsg.size(), 0);
}

/**
 * @brief MODE command, used to set or modify user or channel modes (permissions and settings)
 * @note usage : MODE <channel> {[+|-]o|i|t|k|l} [<limit>|<user>|<password>]
 * @note '+' to add a mode, '-' to remove a mode
 * @note 'o' for chanop mode <user>
 * @note 'i' for invite-only mode
 * @note 't' for chanop-only topic mode
 * @note 'k' for password mode <password>
 * @note 'l' for channel limitation mode <limit>
 * @return ERR_NEEDMOREPARAMS, ERR_NOSUCHCHANNEL, ERR_NOTONCHANNEL, ERR_CHANOPRIVSNEEDED, ERR_NOSUCHNICK,
 * 		ERR_NOSUCHNICK ERR_USERNOTINCHANNEL or ERR_UNKNOWNMODEFLAG in case of error
 */
void Server::handleMode(int fd, const std::vector<std::string> &args)
{
	Client *client = _clients[fd];

	if (!client->isRegistered())
		return ;

	if (args.size() < 2)
	{
		sendError(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
		return ;
	}

	const std::string &channelName = args[1];

	if (_channels.find(channelName) == _channels.end())
	{
		sendError(fd, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
		return ;
	}

	Channel &chan = _channels.find(channelName)->second;

	if (args.size() == 2)
	{
		std::string modeStr = "+";
		std::string params;
		if (chan.isInviteOnly())	modeStr += "i";
		if (chan.isTopicRestricted())	modeStr += "t";
		if (chan.hasPass())	{ modeStr += "k"; params += " " + chan.getPassword(); }
		if (chan.hasUserLimit())
		{
			std::ostringstream oss;
			oss << chan.getUserLimit();
			modeStr += "l";
			params += " " + oss.str();
		}
		std::string reply = ":ircserv 324 " + client->getNickname() + " " + channelName + " " + modeStr + params + "\r\n";
		send(fd, reply.c_str(), reply.length(), 0);
		return ;
	}

	if (!chan.hasMember(fd))
	{
		sendError(fd, ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
		return ;
	}

	if (!chan.isOperator(fd))
	{
		sendError(fd, ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return ;
	}

	const std::string &modestring = args[2];
	bool adding = true;
	size_t paramIdx = 3;

	std::string appliedModes;
	std::string appliedParams;
	bool lastSign = true;

	for (size_t i = 0; i < modestring.size(); ++i)
	{
		char c = modestring[i];
		if (c == '+') { adding = true; continue; }
		if (c == '-') { adding = false; continue; }

		switch (c)
		{
			case 'i':
				chan.setInviteOnly(adding);
				if (appliedModes.empty() || lastSign != adding)
					{ appliedModes += (adding ? "+" : "-"); lastSign = adding; }
				appliedModes += 'i';
				break;

			case 't':
				chan.setTopicRestricted(adding);
				if (appliedModes.empty() || lastSign != adding)
					{ appliedModes += (adding ? "+" : "-"); lastSign = adding; }
				appliedModes += 't';
				break;

			case 'k':
				if (adding)
				{
					if (paramIdx >= args.size())
					{ sendError(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters"); break; }

					chan.setPassword(args[paramIdx]);
					if (appliedModes.empty() || lastSign != adding)
						{ appliedModes += "+"; lastSign = adding; }
					appliedModes += 'k';

					if (!appliedParams.empty()) appliedParams += " ";
					appliedParams += args[paramIdx];
					paramIdx++;
				}

				else
				{
					chan.setPassword("");
					if (appliedModes.empty() || lastSign != adding)
						{ appliedModes += "-"; lastSign = adding; }
					appliedModes += 'k';
					if (paramIdx < args.size()) paramIdx++;
				}
				break;

			case 'l':
				if (adding)
				{
					if (paramIdx >= args.size())
					{ sendError(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters"); break; }
					int limit = std::atoi(args[paramIdx].c_str());
					if (limit > 0)
					{
						chan.setUserLimit(limit);
						if (appliedModes.empty() || lastSign != adding)
							{ appliedModes += "+"; lastSign = adding; }
						appliedModes += 'l';
						std::ostringstream oss;
						oss << limit;
						if (!appliedParams.empty()) appliedParams += " ";
						appliedParams += oss.str();
					}
					paramIdx++;
				}
				else
				{
					chan.setUserLimit(0);
					if (appliedModes.empty() || lastSign != adding)
						{ appliedModes += "-"; lastSign = adding; }
					appliedModes += 'l';
				}
				break;

			case 'o':
				{
					if (paramIdx >= args.size())
						{ sendError(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters"); break; }

					const std::string &nick = args[paramIdx];
					int targetFd = -1;
					for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
					{
						if (it->second->getNickname() == nick)
						{ targetFd = it->first; break; }
					}

					if (targetFd == -1)
					{ sendError(fd, ERR_NOSUCHNICK, nick + " :No such nick/channel"); paramIdx++; break; }

					if (!chan.hasMember(targetFd))
					{ sendError(fd, ERR_USERNOTINCHANNEL, nick + " " + channelName + " :They aren't on that channel"); paramIdx++; break; }

					if (adding)
						chan.addOperator(targetFd);
					else
						chan.removeOperator(targetFd);

					if (appliedModes.empty() || lastSign != adding)
						{ appliedModes += (adding ? "+" : "-"); lastSign = adding; }
					appliedModes += 'o';
					if (!appliedParams.empty()) appliedParams += " ";
					appliedParams += nick;
					paramIdx++;
					break;
				}

			default:
				{
					sendError(fd, ERR_UNKNOWNMODEFLAG, std::string(1, c) + " :is unknown mode char");
					break;
				}
		}
	}

	if (!appliedModes.empty())
	{
		std::string modeMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost MODE " + channelName + " " + appliedModes;
		if (!appliedParams.empty())
			modeMsg += " " + appliedParams;
		modeMsg += "\r\n";
		const std::set<int> &members = chan.getClientsFd();
		for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
			send(*it, modeMsg.c_str(), modeMsg.length(), 0);
	}
}
