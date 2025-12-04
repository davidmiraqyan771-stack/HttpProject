# HttpProject

Welcome to the TTP/1.0 (Text Transfer Protocol) project, which is a simple clone of the HTTP protocol, so you can understand how everything works internally.

I just wanted to write a few things here before launching and using the program.

1. To compile, you just need to run the shell script through its shebang (#!/bin/sh), so technically it should work on both macOS and Linux.

2. Next, the client executable file will appear in the client directory, and the server executable file will appear in the server directory. IMPORTANT! For the server, it's best to run it in its own working directory; there's no difference for the client.

3. The client supports multiple newlines, so if you want to mark the end of text, type ']', for example:

GET]

/file]

Hello, I am
Client]

That's it... I think that's all I wanted... good luck using it.
