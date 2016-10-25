# Blabbr Protocol

## 1 Introduction

The Blabbr protocol is intended to be simple by nature. Messages are sent between a client and a server to exchange commands regarding authentication/deauthentication, intiation of private chats, sending text, to name just a few. A comprehensive list of each command can be found in this document.

Blabbr works on the principal that there is a central server of which clients connect to in order to communicate with each other. The server is responsible for receiving messages from clients and then sending the appropriate messages to the appropriate clients.

## 2 Messages

### 2.1 Message Structure

Messages consist of a command follow by one or more arguments, pending which command is used.

    message    = command *( argument )
    command    = "/" 1*( eng_ch )
    argument   = 1*( nospcrlfcl )
    
    nospcrlfcl = %x01-09 / %x0B-0C / %x0E-1F / %x21-39 / %x3B-FF
                 ; any octet except for NUL, CL, LF, " ", or ":"
    eng_ch     = a-zA-Z ; characters from a to z


### 2.2 Message Interpretation

A client or server MAY disregard any messages that do not conform to the message structured outlined in section [2.1](#2.1-message-structure). A client or server MAY choose to do something with these messages, but it will not be defined in this document, and neither a server nor client should rely on the other to do anything with a invalid message.


## Commands

    who        - list users in current chat
