# Blabbr Protocol

## 1 Introduction

The Blabbr protocol is intended to be simple by nature. Messages are sent between a client and a server to exchange commands regarding authentication/deauthentication, intiation of private chats, sending text, to name just a few. A comprehensive list of each command can be found in this document.

Blabbr works on the principal that there is a central server of which clients connect to in order to communicate with each other. The server is responsible for receiving messages from clients and then sending the appropriate messages to the appropriate clients.

## 2 Messages

### 2.1 Message Structure

Messages consist of a command follow by one or more arguments, pending which command is used. The format for a message is presented here in augmented BNF:

    message     = command *( argument )
    command     = "/" 1*( eng_ch )
    argument    = 1*( nospcrlfcl )
    text        = *( text_ch )
    
    nospcrlfcl  = %x01-09 / %x0B-0C / %x0E-1F / %x21-39 / %x3B-FF
                 ; any octet except for NUL, CL, LF, " ", or ":"
    eng_ch      = %ca-z / $cA-Z ; characters from a to z
    eng_alphnum = en_ch / %d0-9
    text_ch     = %x01-09 / %x0B-0C / %x0E-1F / %x20-39 / %x3A-FF
                 ; any octet except for NUL, CL, or LF

Through the document the parameters denoted as `<text>` will be encoutered. They are of the form `text` described immediately above. These parameters are the only ones that permit spaces and colons, and must be the last parameter for any given command.


### 2.2 Message Interpretation

A client or server MAY disregard any messages that do not conform to the message structured outlined in section [2.1](#21-message-structure). A client or server MAY choose to do something with these messages, but it will not be defined in this document, and neither a server nor client should rely on the other to do anything with a invalid message.


## 3 Commands

### 3.1 Client Commands

Client commands describe the commands sent by a client to a server. For each command listed, a decription of the command and its arguments are specified. 

Additionally, for each command, the behaviour of the server is described. 

#### 3.1.1 Who Command

    who <chatroom>		           - lists currents users in a chatroom

The `<chatroom>` parameter indicates the chatroom to get a list of users from.

List the users in the current chat. The reply MUST be a comma separated list with no trailing comma.

#### 3.1.2 List Command

    list                           - lists chatrooms

Lists all available chatrooms. The reply MUST be a comma separated list with no trailing comma.

#### 3.1.3 User Command

    register <username> <password> - registers a user with the blabbr server
    
    <username>                     = 1*20( eng_alphnum )
    <password>                     = 6*20( text_ch )

#### 3.1.4 Join Command

    join <chatroom>                - join chatroom

The `<chatroom>` parameter indicates a chatroom. If the chatroom does not exist the server MUST create it.

#### 3.1.5 Whisper Command

    whisper <user> <text>          - initiates a private chatroom with user

The `<user>` parameter indicates the username of the user a client wants to start a private chat with.

The `<text>` parameter indicates the the initial text to be sent to `<user>`. This parameter is optional.

#### 3.1.6 Blab Command

    blab <chatroom> <text>         - puts text into chatroom on behalf of client

The `<chatroom>` parameter indicates the name of the chat to place text in.

The `<text>` parameter indicates the text to be placed into the chat.