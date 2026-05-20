*This project has been created as part of the 42 curriculum by makoon, eieong, niclee.*

# ft_IRC Internet Relay Chat

## Description

This project consists in the implementation of a simplified IRC (Internet Relay Chat) server written in C++98.

The goal is to recreate the core functionality of an IRC server, allowing multiple clients to connect, communicate in channels, and exchange message in real time using the IRC protocol.

The server handles multiple simultaneous clients using non-blocking I/0 and the poll() system call.
It supports basic IRC commands such as:
- PASS : command used to set a connection password
- NICK : command used to give a nickname or change the previous one
- USER : command used at the beginning of connection to specify the username and realname of new user
- JOIN : command used to join a channel
- PART : command used to leave a channel
- PRIVMSG : command used to send a private message to a user or a channel
- QUIT : command used to disconnect from the server
- PING : command used to check if the connection is still alive
- KICK : command used to remove a user from a channel
- INVITE : command used to invite a user to a channel
- TOPIC : command used to set or view the topic of a channel
- MODE : command used to set or modify user or channel modes (permissions and settings).

The different flags that can be used to MODE command :
- i : only invited users can join the channel
- t : only channel operators can change the topic
- k : a password is required to join the channel
- o : grants or removes operator rights to a user
- l : set the maximum number of users allowed in the channel

The project also includes optional bonus features such as a bot and potential file transfer support.
The bot can, on the channel #general, display available commands or allows users to create a new poll directly on the channel, vote and give the result of the poll.

## Instructions

### Compilation

The project must be compiled using :
```bash
make        # Compile the server
make bonus  # Compile the bot
```

### Execution

The executable will be run as follows:
```bash
./ircserv <port> <password>                     # for server par ex : ./ircserv 6667 mdp
./ircbot <host> <port> <password> <nickname>    # for bot par ex : ./ircbot 127.0.0.1 6667 mdp bot
```
port : The port number on which IRC server will be listening for incoming IRC connections
password: The connection password. It will be needed by any IRC client that tries to connect the server.

### Connecting with IRC client

** With irssi :**
```bash
irssi
/connect localhost <port> <password> [<nickname>]
```

** With netcat :***
```bash
nc -C localhost <port>
PASS <password>
NICK <nickname>
USER <username> <hostname> <servername> <realname>
```

Commands after client connection:
```bash
JOIN <channel> [<key>]
PART <channel> [<part message>]
PRIVMSG <target> <text to be sent>
QUIT [<Quit message>]
PING <server>
KICK <channel> <user> [<comment>]
INVITE <nickname> <channel>
TOPIC <channel> [<topic>]
MODE <channel> {[+|-]o|i|t|k|l} [<limit>|<user>|<password>]
```

### Testing bonus

** File transfer support **

```bash
echo "Hello !" > /tmp/test.txt  # Create a file
```

On client1 connection :
```bash
/dcc send <client2> /tmp/test.txt # Send test.txt to client2
```

On client2 connection :
```bash
/dcc get <client1>
```

The file will be download on dcc_download_path (which is on "~" on irssi by default).

** Bot commands **

On client connection :
```bash
/join #general
```

On the channel #general :
```bash
!help                                               # Display available commands
!poll <question> <choice1> <choice2> <choice...>    # Create a new poll
!vote <choice>                                      # Vote for a choice
!endvote                                            # End the current poll immediately
```

## Resources

### Documentation

[RFC 1459 - IRC Protocol](https://datatracker.ietf.org/doc/html/rfc1459)

[RFC 2812 - IRC Protocol](https://datatracker.ietf.org/doc/html/rfc2812)

[RFC 2811 - IRC Protocol](https://datatracker.ietf.org/doc/html/rfc2811)

[RFC 2813 - IRC Protocol](https://datatracker.ietf.org/doc/html/rfc2813)


### AI Usage

ChatGPT was used as a learning and debugging assistant, and also provided some test ideas.