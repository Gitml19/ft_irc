#ifndef BOT_HPP
#define BOT_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <cerrno>
#include <ctime>
#include <vector>
#include <map>
#include <set>

extern volatile sig_atomic_t g_bot_running;

struct Poll
{
	std::string question;
	std::vector<std::string> options;
	std::vector<int> votes;
	std::set<std::string> voters;
	time_t startTime;
	int duration;
	std::string channel;
	bool active;
};

class Bot
{
	public:
		Bot(const std::string &host, int port, const std::string &password, const std::string &nickname);
		~Bot();
		
		void run();
		void connect();
		void authenticate();
		void joinChannel(const std::string &channel);
		
	private:
		std::string _host;
		int         _port;
		std::string _password;
		std::string _nickname;
		std::string _username;
		std::string _realname;
		int         _sockfd;
		std::string _buffer;
		
		// Poll management
		Poll _currentPoll;
		
		void sendRaw(const std::string &msg);
		void handleMessage(const std::string &line);
		void handlePrivmsg(const std::string &sender, const std::string &target, const std::string &message);
		void sendPrivmsg(const std::string &target, const std::string &msg);
		
		void handleCommand(const std::string &sender, const std::string &target, const std::string &cmd, const std::string &args);
		
		// Poll commands
		void cmdPoll(const std::string &sender, const std::string &target, const std::string &args);
		void cmdVote(const std::string &sender, const std::string &target, const std::string &args);
		void cmdEndVote(const std::string &target);
		void checkPollTimeout();
		void displayPollResults(const std::string &target);
		
		std::string getNickFromPrefix(const std::string &prefix);
		void        parseMessage(const std::string &line, std::string &prefix, std::string &command, std::vector<std::string> &params);
};

#endif
