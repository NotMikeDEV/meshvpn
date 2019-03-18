#include "Globals.h"

RouteServer4Client::RouteServer4Client(int Socket, RouteServer4* Parent)
{
	this->Socket = Socket;
	this->Parent = Parent;
	if (Debug >1)
		printf("New TCP Client %u\n", Socket);
}
RouteServer4Client::~RouteServer4Client()
{
	if (Debug > 1)
		printf("Close TCP Client %u\n", Socket);
	close(Socket);
}
void RouteServer4Client::Send(unsigned char* Packet, int Length)
{
	int ret = -1;
	if (!WriteBufferLength)
	{
		ret = send(Socket, Packet, Length, 0);
	}
	if (ret != Length)
	{
		if (ret == -1)
			ret = 0;
		int AppendLength = Length - ret;
		WriteBuffer = (unsigned char*)realloc(WriteBuffer, WriteBufferLength + AppendLength);
		memcpy(WriteBuffer + WriteBufferLength, Packet + ret, AppendLength);
		WriteBufferLength += AppendLength;
	}
}
int RouteServer4Client::GetHandle()
{
	return Socket;
}
bool RouteServer4Client::ReadHandle()
{
	char buff[1024];
	int ret = recv(Socket, buff, sizeof(buff), 0);
	if (ret == 0)
	{
		Parent->DeleteClient(this);
	}
	return ret > 0;
}
void RouteServer4Client::WriteHandle()
{

}
