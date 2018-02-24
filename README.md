# Chat-program-using-TCP-sockets

 Typing “./chat -s portnumber x”, where portnumber is the port number and x is the maximum
  number of clients (<5), opens the server.
 Typing “./chat -c portnumber serverAddress” , where portnumber is the port number and the
  serverAddress is the server’s address, opens the client.
 Typing “exit” in both the server and client exits the program.
 The client accepts multiple commands:
    o “open username” creates a new chat session. The username is the username that
      corresponds to one of the sockets (maximum sockets specified by the server).
    o “who” gets all the usernames of clients that are connected
    o “close” disconnects the user from the server. Other clients can then use the username
      and socket that was used by the user.
    o “exit” exits the program. If the user exits without closing the currents session, the server
      will remove the user after a few seconds of not receiving any keep alive messages.
    o “to user1 user2… “ adds users to the current user’s recipient list, i.e. who the user wants
      to send messages to. “user1” and “user2” are the usernames of the users that the
      current user wants to send messages to. The list doesn’t shrink so a re-login is required
      if the user wants to remove usernames.
    o “< message” sends “message” to the user’s recipients specified in the “to” command if
      no recipients have been specified, it doesn’t send anything.
