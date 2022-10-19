It appears that Steam API and asio do not mix. 

The most minimal Steam Game Server example I could create with 1 header and 1 source file failed to compile because of the dreaded `fatal error C1189: #error : WinSock.h has already been included`. 

This only happens with Windows because they cant get their stuff right.

A Google search for `steamapi winwsock2` yielded no relevant results. 

It appears that I will have to either make separate libraries (one for asio, the other for steamapi), or scrap asio all together. 

Well, it was nice knowing asio..
