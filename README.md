# Blabbr

## 1 Introduction

Blabbr is a protocol designed for internet chat. It is also a server, and a client. Included in this repository is a description of the protocol, the code for a Blabbr server, and a Blabbr client.

Blabbr was produced as a programing assignment for the computer networking course at Reykjavik University. The assignment was to create a chat server, chat client, and protocol. The communication was required to take place over a TCP connection utilizing OpenSSL.

The Blabbr protocol is intended to be simple by nature. Messages are sent between a client and a server to exchange commands regarding authentication, private messages, and chatting in a chatroom to name just a few. A comprehensive list of each command can be found in this document.

Blabbr works on the principal that there is a central server of which clients connect to in order to communicate with each other. The server is responsible for receiving messages from clients and then sending the appropriate messages to the appropriate clients.

## 2 Messages

### 2.1 Message Structure

Messages consist of a command follow by one or more arguments, pending which command is used. The format for a message is presented here in augmented BNF:

    message       = command *( argument )
    text_message  = (username) (space) 1*( text_ch )
    command       = "/" 1*( eng_ch )
    argument      = 1*( no_space )
    text          = *( text_ch )
    
    no_space      = any non-space unicode character
    eng_ch        = %ca-z / $cA-Z ; characters from a to z
    text_ch       = any unicode character


Through the document the parameters denoted as `<text>` will be encoutered. They are of the form `text` described immediately above. These parameters are the only ones that permit anything outside of what `eng_ch` or `no_space` are defined as, and MUST be the last parameter for any given command, if the command takes a parameter.

### 2.2 Message Length

Blabbr messages are defined to be at most 4000 `wchar_t` (unicode data type defined in C). Pending the operating system, `wchar_t` can be as much as 4 bytes each, but as little as 2 bytes each.

We realize this is not as consistent as is optimal, and any further revision of the Blabbr protocol will consider this issue as a top priority.


### 2.3 Message Interpretation

A client or server MAY disregard any messages that do not conform to the message structured outlined in section [2.1](#21-message-structure). A client or server MAY choose to do something with these messages, but it will not be defined in this document, and neither a server nor client should rely on the other to do anything with an invalid message.

If a user is not authenticated, the server MUST allow them to only use the `user` and `help` commands.

Messages that do not begin with a forward slash will MUST be assumed to be a text message, that is, a unicode string from a user to be displayed in a chatroom.

## 3 Commands

### 3.1 Client Commands

Client commands describe the commands sent by a client to a server. For each command listed, a decription of the command and its arguments are specified. 

Additionally, for each command, the behaviour of the server is described. 

#### 3.1.1 Who Command

    who		                       - list all online users

List the users in the current chat. The reply MUST be a list delimited by the newline (\n) character.

The list MUST contain the users username, ip address, port number, and name of the chatroom the user is currently in.

#### 3.1.2 List Command

    list                           - lists chatrooms

Lists all available chatrooms. The reply MUST be a comma separated list with no trailing comma.
 
The format of each line is as such:

    (username) (space) (ip_addr)":" (port) (space) (chatoom | no_chatroom)
    
    no_chatroom                    = "[NO CURRENT CHATROOM]"

#### 3.1.3 User Command

    user <username> <password>     - registers/authenticates a user with the blabbr server
    
    <username>                     = 1*20( no_space )
    <password>                     = 6*20( no_space )

If the user does not exist the server MUST create the account with the given password. If the user exists MUST attempt to authenticate them.

#### 3.1.4 Join Command

    join <chatroom>                - join chatroom
    
    <chatroom>                     = 1*( no_space )

The `<chatroom>` parameter indicates a chatroom. If the chatroom does not exist the server MUST create it.

#### 3.1.5 Say Command

    say <username> <text>          - sends a private message to <username>
    
    <username>                     = 1*20( no_space )
    <text>                         = 1*( text_ch )

The `<user>` parameter indicates the username of the user a client wants to start a private chat with.

#### 3.1.6 Game Command

    game <username>                - sends a game invite to <username>
    
    <username>                     = 1*20( no_space )


The game is a game of dice rolls. Should a user want to accept the invite, they type `/accept`. The server then must prompt both users to type `/roll/` and from there the server MUST generate two separates rolls of two dices, one for each user, and then compare the results wich each other. Then server then MUST let both players know who was the winner (i.e. who had the higher dice roll).

Should a user wish to decline the invite, they type '/decline' to forfeit.

A user can not be in two different dice games at the same time. The server MUST enforce this.

#### 3.1.7 Accept Command

    accept                         - accepts a game invite

This is the `/accept` command described in [3.1.6](#316-game-command).

#### 3.1.8 Decline Command

    decline                        - declines a game invite

This is the `/decline` command described in [3.1.6](#316-game-command).

#### 3.1.9 Roll Command

    roll                           - rolls two dice in a game

This is the `/roll` command described in [3.1.6](#316-game-command).

#### 3.1.10 Help Command

    help                           - lists all commands

This list all client commands. However, as server MAY choose not to show sub-commands, like `/accept`, etc.

### 4 Text Messages

Text messages are messages that are intended for display. They are of the form described in [2.1](#21-message-structure), that is, a username that is followed space, and then a unicode string. The username is that of the author of the message.

For text messages sent from the server to a client to indicate direct interactions with the server (i.e. authentication, list of available commands), the username MUST be "SERVER".

### 5 Build Requirements

The packages needed to build Blabbr are openssl, ncursesw, and glib.

Blabbr was developed primarily on an Ubuntu system. These packages can be installed on Ubuntu with the following command:

    apt-get install libssl-dev libglib2.0-dev libncursesw5-dev