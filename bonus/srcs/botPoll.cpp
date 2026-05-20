#include "../inc/bot.hpp"

void Bot::handleCommand(const std::string &sender, const std::string &target, const std::string &cmd, const std::string &args)
{
	std::cout << "Command from " << sender << ": " << cmd << " " << args << std::endl;

	if (cmd == "poll")
		cmdPoll(sender, target, args);
	else if (cmd == "vote")
		cmdVote(sender, target, args);
	else if (cmd == "endvote")
		cmdEndVote(target);
	else if (cmd == "help")
	{
		sendPrivmsg(target, "Poll Bot - Available commands:");
		sendPrivmsg(target, "!poll <question> <option1> <option2> ... - Create a poll");
		sendPrivmsg(target, "!vote <number> - Vote for an option");
		sendPrivmsg(target, "!endvote - End current poll");
	}
	else
		sendPrivmsg(target, "Unknown command. Type !help for help");
}

void Bot::cmdPoll(const std::string &sender, const std::string &target, const std::string &args)
{
	(void)sender;
	
	if (_currentPoll.active)
	{
		sendPrivmsg(target, "A poll is already in progress!");
		return;
	}

	std::vector<std::string> tokens;
	std::istringstream iss(args);
	std::string token;
	
	while (iss >> token)
		tokens.push_back(token);

	if (tokens.size() < 3)
	{
		sendPrivmsg(target, "Usage: !poll <question> <option1> <option2> [option3...]");
		sendPrivmsg(target, "Example: !poll 1+1 =0 =1 =2 =4");
		return;
	}

	_currentPoll.question = tokens[0];
	_currentPoll.options.clear();
	_currentPoll.votes.clear();
	_currentPoll.voters.clear();
	
	for (size_t i = 1; i < tokens.size(); i++)
	{
		_currentPoll.options.push_back(tokens[i]);
		_currentPoll.votes.push_back(0);
	}
	
	_currentPoll.startTime = time(NULL);
	_currentPoll.channel = target;
	_currentPoll.active = true;

	sendPrivmsg(target, "New poll: " + _currentPoll.question);
	for (size_t i = 0; i < _currentPoll.options.size(); i++)
	{
		std::ostringstream oss;
		oss << (i + 1) << ". " << _currentPoll.options[i];
		sendPrivmsg(target, oss.str());
	}
	
	std::ostringstream timeout_msg;
	timeout_msg << "Vote with !vote <number> (expires in " << _currentPoll.duration << " seconds)";
	sendPrivmsg(target, timeout_msg.str());
}

void Bot::cmdVote(const std::string &sender, const std::string &target, const std::string &args)
{
	if (!_currentPoll.active)
	{
		sendPrivmsg(target, "No active poll. Create one with !poll");
		return;
	}

	if (_currentPoll.channel != target)
	{
		sendPrivmsg(target, "Poll is in another channel!");
		return;
	}

	if (_currentPoll.voters.find(sender) != _currentPoll.voters.end())
	{
		sendPrivmsg(target, sender + " has already voted!");
		return;
	}

	int choice = std::atoi(args.c_str());
	if (choice < 1 || choice > static_cast<int>(_currentPoll.options.size()))
	{
		std::ostringstream oss;
		oss << "Invalid choice. Vote between 1 and " << _currentPoll.options.size();
		sendPrivmsg(target, oss.str());
		return;
	}

	_currentPoll.votes[choice - 1]++;
	_currentPoll.voters.insert(sender);
	
	sendPrivmsg(target, "Vote from " + sender + " registered for: " + _currentPoll.options[choice - 1]);
}

void Bot::cmdEndVote(const std::string &target)
{
	if (!_currentPoll.active)
	{
		sendPrivmsg(target, "No active poll.");
		return;
	}

	if (_currentPoll.channel != target)
	{
		sendPrivmsg(target, "Poll is in another channel!");
		return;
	}

	displayPollResults(target);
	_currentPoll.active = false;
}

void Bot::displayPollResults(const std::string &target)
{
	sendPrivmsg(target, "Poll results: " + _currentPoll.question);
	
	int totalVotes = 0;
	for (size_t i = 0; i < _currentPoll.votes.size(); i++)
		totalVotes += _currentPoll.votes[i];

	if (totalVotes == 0)
	{
		sendPrivmsg(target, "No votes registered.");
		return;
	}

	for (size_t i = 0; i < _currentPoll.options.size(); i++)
	{
		int votes = _currentPoll.votes[i];
		int percentage = (votes * 100) / totalVotes;
		
		std::ostringstream oss;
		oss << (i + 1) << ". " << _currentPoll.options[i] << ": " << votes << " votes (" << percentage << "%)";
		sendPrivmsg(target, oss.str());
	}
	
	std::ostringstream total_msg;
	total_msg << "Total: " << totalVotes << " votes";
	sendPrivmsg(target, total_msg.str());
}

void Bot::checkPollTimeout()
{
	if (!_currentPoll.active)
		return;

	time_t now = time(NULL);
	if (now - _currentPoll.startTime >= _currentPoll.duration)
	{
		sendPrivmsg(_currentPoll.channel, "Time's up! Poll has ended.");
		displayPollResults(_currentPoll.channel);
		_currentPoll.active = false;
	}
}
