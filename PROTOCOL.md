# Blabbr Protocol

## Introduction

The Blabbr protocol is intended to be simple by nature. Messages are sent between a client and a server to exchange commands regarding authentication/deauthentication, intiation of private chats, sending text, to name just a few. A comprehensive list of each command can be found in this document.

Blabbr works on the principal that there is a central server of which clients connect to in order to communicate with each other. The server is responsible for receiving messages from clients and then sending the appropriate messages to the appropriate clients.

## Message Structure

Messages consist of a command follow by one or more arguments, pending which command is used.

message = [command] ( [argument])*
command = /([eng_ch])*
eng_ch = a-z ; lowest case characters from a to z



## Commands

