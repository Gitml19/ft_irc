#include "../inc/bot.hpp"

volatile sig_atomic_t g_bot_running = 1;

static void signalHandler(int sig)
{
	(void)sig;
	g_bot_running = 0;
	std::cout << "\nBot shutting down..." << std::endl;
}

Bot::Bot(const std::string &host, int port, const std::string &password, const std::string &nickname)
	: _host(host), _port(port), _password(password), _nickname(nickname), 
	  _username("ircbot"), _realname("IRC Bot"), _sockfd(-1)
{
	_currentPoll.active = false;
	_currentPoll.duration = 60;
}

Bot::~Bot()
{
	if (_sockfd >= 0)
	{
		signal(SIGPIPE, SIG_IGN);
		sendRaw("QUIT :Bot shutting down\r\n");
		close(_sockfd);
	}
}

void Bot::connect()
{
	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockfd < 0)
		throw std::runtime_error("Failed to create socket");

	struct sockaddr_in server_addr;
	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(_port);
	
	if (inet_pton(AF_INET, _host.c_str(), &server_addr.sin_addr) <= 0)
	{
		close(_sockfd);
		_sockfd = -1;
		throw std::runtime_error("Invalid address");
	}

	if (::connect(_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		close(_sockfd);
		_sockfd = -1;
		throw std::runtime_error("Connection failed");
	}

	std::cout << "Connected to " << _host << ":" << _port << std::endl;
}

void Bot::authenticate()
{
	sendRaw("CAP LS\r\n");
	sendRaw("PASS " + _password + "\r\n");
	sendRaw("NICK " + _nickname + "\r\n");
	sendRaw("USER " + _username + " 0 * :" + _realname + "\r\n");
	std::cout << "Authentication sent" << std::endl;
}

void Bot::joinChannel(const std::string &channel)
{
	sendRaw("JOIN " + channel + "\r\n");
	std::cout << "Joining channel: " << channel << std::endl;
}

void Bot::run()
{
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	struct pollfd pfd;
	pfd.fd = _sockfd;
	pfd.events = POLLIN;

	char buffer[512];

	while (g_bot_running)
	{
		int ret = poll(&pfd, 1, 1000);
		
		checkPollTimeout();
		
		if (ret < 0)
		{
			if (!g_bot_running)
				break;
			std::cerr << "poll() error" << std::endl;
			break;
		}

		if (ret == 0)
			continue;

		if (pfd.revents & POLLIN)
		{
			std::memset(buffer, 0, sizeof(buffer));
			int n = recv(_sockfd, buffer, sizeof(buffer) - 1, 0);
			
			if (n <= 0)
			{
				std::cout << "Connection closed by server" << std::endl;
				break;
			}

			_buffer.append(buffer, n);

			size_t pos;
			while ((pos = _buffer.find("\r\n")) != std::string::npos)
			{
				std::string line = _buffer.substr(0, pos);
				_buffer.erase(0, pos + 2);
				
				if (!line.empty())
				{
					std::cout << "<<< " << line << std::endl;
					handleMessage(line);
				}
			}
		}
	}
}

int main(int argc, char **argv)
{
	if (argc != 5)
	{
		std::cerr << "Usage: " << argv[0] << " <host> <port> <password> <nickname>" << std::endl;
		std::cerr << "Example: " << argv[0] << " 127.0.0.1 6667 test MyBot" << std::endl;
		return 1;
	}

	std::string host = argv[1];
	int port = std::atoi(argv[2]);
	std::string password = argv[3];
	std::string nickname = argv[4];

	if (port <= 0 || port > 65535)
	{
		std::cerr << "Error: Invalid port number" << std::endl;
		return 1;
	}

	try
	{
		Bot bot(host, port, password, nickname);
		bot.connect();
		bot.authenticate();
		
		sleep(2);
		
		bot.joinChannel("#general");
		
		bot.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

