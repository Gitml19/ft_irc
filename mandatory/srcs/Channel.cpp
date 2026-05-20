#include "../inc/Channel.hpp"

Channel::Channel(const std::string &name)
	: _name(name), _user_limit(100), _inviteOnly(false), _topicRestricted(false), _hasUserLimit(false), _hasPass(false) {}

Channel::~Channel() {}

const std::string &Channel::getName() const { return _name; }

const std::set<int> &Channel::getClientsFd() const { return _clients_fd; }

void Channel::addClient(int fd) { _clients_fd.insert(fd); }
void Channel::removeClient(int fd) { _clients_fd.erase(fd); }
bool Channel::hasMember(int fd) const { return _clients_fd.count(fd) > 0; }
int  Channel::getMemberCount() const { return static_cast<int>(_clients_fd.size()); }

bool Channel::isOperator(int fd) const { return _operators.count(fd) > 0; }
void Channel::addOperator(int fd) { _operators.insert(fd); }
void Channel::removeOperator(int fd) { _operators.erase(fd); }

const std::string &Channel::getTopic() const { return _topic; }
void Channel::setTopic(const std::string &topic) { _topic = topic; }

bool Channel::isInviteOnly() const { return _inviteOnly; }
void Channel::setInviteOnly(bool val) { _inviteOnly = val; }
bool Channel::isTopicRestricted() const { return _topicRestricted; }
void Channel::setTopicRestricted(bool val) { _topicRestricted = val; }

void Channel::addInvite(int fd) { _inviteList.insert(fd); }
bool Channel::isInvited(int fd) const { return _inviteList.count(fd) > 0; }

const std::string &Channel::getPassword() const { return _password; }
void Channel::setPassword(const std::string &pwd)
{
	_password = pwd;
	_hasPass = !pwd.empty();
}
bool Channel::hasPass() const { return _hasPass; }

int  Channel::getUserLimit() const { return _user_limit; }
void Channel::setUserLimit(int limit)
{
	_user_limit = limit;
	_hasUserLimit = (limit > 0);
}
bool Channel::hasUserLimit() const { return _hasUserLimit; }
bool Channel::isFull() const
{
	if (!_hasUserLimit)
		return false;
	return getMemberCount() >= _user_limit;
}