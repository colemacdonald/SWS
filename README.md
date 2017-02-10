# SWS
Simple Web Server


V00802949
B03

I defined TRUE and FALSE to be 1 and 0 respectively to increase code readability; it also happens to be habit to return TRUE.

The 4 global variables are global such that a few of the helper functions were easier to write.

When select returns due to a message being recieved through the socket, an array of char * (arrRequest) is created and each section of the request string (method, uri, http version) are read into their respective slots in the array. Next the response string is initialized to contain "HTTP/1.0 " because that will be the basis of all responses. Each item in arrRequest is checked for validity and the remainder of the response string is appended to the original.

The header is sent first for simplicities sake (always have to send it) followed by the file - if required.

