#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
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
#include <stdexcept>
#include <cstdlib>
#include "Channel.hpp"

	class	Client
	{
		private:
			int	_fd;
			std::string	_nickname;
			std::string	_username;
			std::string _realname;
			std::set<std::string> _channels;

			bool _passOk;
			bool _nickSet;
			bool _userSet;
			bool _registered;

		public:
			Client(int fd);
			~Client();
			std::string	_buffer;

			void setNickname(const std::string &nickname);
			void setUsername(const std::string &username);
			void setRealname(const std::string &realname);
			void setPassOk(const bool boolean);
			void setNickSet(const bool boolean);
			void setUserSet(const bool boolean);
			void setRegistered(const bool boolean);

			int getFd() const;
			const std::string getNickname() const;
			const std::string getUsername() const;
			const std::string getRealname() const;
			bool isPassOk() const;
			bool isNickSet() const;
			bool isUserSet() const;
			bool isRegistered() const;
			const std::set<std::string> &getChannels() const;

			void addChannel(const std::string &chanName);
			void removeChannel(const std::string &chanName);
			size_t getChannelCount() const;
			void closeConnection();
			void send_msg(const std::string &msg);
	};

#endif
