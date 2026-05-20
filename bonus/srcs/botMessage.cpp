#include "../inc/bot.hpp"

std::string Bot::getNickFromPrefix(const std::string &prefix)
{
	size_t pos = prefix.find('!');
	if (pos != std::string::npos)
		return prefix.substr(0, pos);
	return prefix;
}

void Bot::parseMessage(const std::string &line, std::string &prefix, std::string &command, std::vector<std::string> &params)
{
	params.clear();
	prefix.clear();
	command.clear();

	std::string msg = line;
	size_t pos = 0;

	if (!msg.empty() && msg[0] == ':')
	{
		pos = msg.find(' ');
		if (pos != std::string::npos)
		{
			prefix = msg.substr(1, pos - 1);
			msg = msg.substr(pos + 1);
		}
	}

	pos = msg.find(' ');
	if (pos != std::string::npos)
	{
		command = msg.substr(0, pos);
		msg = msg.substr(pos + 1);
	}
	else
	{
		command = msg;
		return;
	}

	while (!msg.empty())
	{
		if (msg[0] == ':')
		{
			params.push_back(msg.substr(1));
			break;
		}
		
		pos = msg.find(' ');
		if (pos != std::string::npos)
		{
			params.push_back(msg.substr(0, pos));
			msg = msg.substr(pos + 1);
		}
		else
		{
			params.push_back(msg);
			break;
		}
	}
}

void Bot::handlePrivmsg(const std::string &sender, const std::string &target, const std::string &message)
{
	std::string nick = getNickFromPrefix(sender);
	std::cout << "[" << target << "] <" << nick << "> " << message << std::endl;

	if (!message.empty() && message[0] == '!')
	{
		size_t space = message.find(' ');
		std::string cmd = message.substr(1, space - 1);
		std::string args = (space != std::string::npos) ? message.substr(space + 1) : "";
		
		std::string reply_target = (target == _nickname) ? nick : target;
		handleCommand(nick, reply_target, cmd, args);
	}
}

void Bot::handleMessage(const std::string &line)
{
	std::string prefix, command;
	std::vector<std::string> params;
	
	parseMessage(line, prefix, command, params);

	if (command == "PING")
	{
		if (!params.empty())
			sendRaw("PONG :" + params[0] + "\r\n");
		return;
	}

	if (command == "PRIVMSG" && params.size() >= 2)
	{
		handlePrivmsg(prefix, params[0], params[1]);
		return;
	}

	if (command == "001")
	{
		std::cout << "Successfully registered with the server!" << std::endl;
	}
}

void Bot::sendRaw(const std::string &msg)
{
	if (_sockfd < 0)
		return;
	send(_sockfd, msg.c_str(), msg.length(), 0);
	std::cout << ">>> " << msg;
}

void Bot::sendPrivmsg(const std::string &target, const std::string &msg)
{
	sendRaw("PRIVMSG " + target + " :" + msg + "\r\n");
}
