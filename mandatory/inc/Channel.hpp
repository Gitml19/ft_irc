#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <set>
#include <string>

	class	Channel
	{
		private:
			std::string _name;
			std::string _topic;
			std::string _password;
			int	_user_limit;
			std::set<int> _clients_fd;
			std::set<int> _inviteList;
			std::set<int> _operators;

			bool _inviteOnly;
			bool _topicRestricted;
			bool _hasUserLimit;
			bool _hasPass;

		public:
			Channel(const std::string &name);
			~Channel();

		const std::string &getName() const;

		const std::set<int> &getClientsFd() const;
		void addClient(int fd);
		void removeClient(int fd);
		bool hasMember(int fd) const;
		int  getMemberCount() const;

		bool isOperator(int fd) const;
		void addOperator(int fd);
		void removeOperator(int fd);

		const std::string &getTopic() const;
		void setTopic(const std::string &topic);

		bool isInviteOnly() const;
		void setInviteOnly(bool val);
		bool isTopicRestricted() const;
		void setTopicRestricted(bool val);

		void addInvite(int fd);
		bool isInvited(int fd) const;

		const std::string &getPassword() const;
		void setPassword(const std::string &pwd);
		bool hasPass() const;

		int  getUserLimit() const;
		void setUserLimit(int limit);
		bool hasUserLimit() const;
		bool isFull() const;
};

#endif
