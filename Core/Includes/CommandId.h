// CommandId.h

#ifndef H_COMMAND_ID_H
#define H_COMMAND_ID_H

#define CMD_UNKNOWN 			0

#define CMD_HANDSHAKE			1
#define CMD_CONNECT				2

#define CMD_GET_REPLY			3

// Storage operation commands
#define CMD_REPO_LOGIN 			10 // not added !
#define CMD_REPO_STATUS			11 // not added !
#define CMD_REPO_INIT			12 // not added !
#define CMD_DIAGNOSTIC			13

// Number commands
#define CMD_NUM_ADD 			50
#define CMD_NUM_FIND 			51 // not added !
#define CMD_NUM_DEL 			52
#define CMD_NUM_OPEN			56
#define CMD_NUM_CHANGE_PIN		57

// Token commands
#define CMD_TOKEN_GET			60
#define CMD_TOKEN_NEW			61
#define CMD_TOKEN_DELETE		62
#define CMD_TOKEN_CHECK			63

// Message commands
#define CMD_MSG_COUNT			100
#define CMD_MSG_SEND			101
#define CMD_MSG_GET_IDS			102
#define CMD_MSG_READ			103
#define CMD_MSG_DELETE			104

// Fix this madness if it grows too much.
#define IS_VALID_COMMAND(c) \
c == CMD_HANDSHAKE || c == CMD_CONNECT || c == CMD_GET_REPLY || c == CMD_DIAGNOSTIC || \
c == CMD_NUM_ADD || c == CMD_NUM_FIND || c == CMD_NUM_DEL || c == CMD_NUM_OPEN || c == CMD_NUM_CHANGE_PIN || \
c == CMD_TOKEN_GET || c == CMD_TOKEN_NEW || c == CMD_TOKEN_DELETE || c == CMD_TOKEN_CHECK || \
c == CMD_MSG_COUNT || c == CMD_MSG_SEND || c == CMD_MSG_GET_IDS || c == CMD_MSG_READ || c == CMD_MSG_DELETE

#endif
