# [zorge_sms](https://github.com/nar314/zorge_sms) - private messaging solution.

## What is this.

This is private messaging solution to send and receive short text messages.

## How to build.

See "how to build.txt"
You will get "zsd" deamon and "zsc-1.0.jar" GUI client.


## How to start server.

Copy ./zsd in any folder. Two ways to start.

Storage location in the current folder.
```sh
$ ./zsd
```

Storage location in dedicated folder.
```sh
$ ./zsd /home/user1/storage1
```
When you start server first time you will be ask to confirm storage location.
Also you will need to enter storage password. This password will be used to encrypt
everything. Also, see created config file.

When you start server with existing storage, you will be asked only for password.

## How to stop server.
Ctrl-C

## How to start client.

GUI.
```sh
$java -jar ./zsc-1.0.jar
```
Same client allows to send messages from command line.
For more details

```sh
$java -jar ./zsc-1.0.jar
```

## How to use this thing.

1. Build from sources.
2. Start server.
3. Start GUI client.
    - connect to server (default port is 7530)
    - add new numner.
    - send message to myself.

