#include "Globals.h"

RouteServer6Client::RouteServer6Client(int Socket, RouteServer6* Parent)
{
	this->Socket = Socket;
	this->Parent = Parent;
	if (Debug > 1)
		printf("New TCP Client %u\n", Socket);
}
RouteServer6Client::~RouteServer6Client()
{
	if (Debug > 1)
		printf("Close TCP Client %u\n", Socket);
	close(Socket);
}
void RouteServer6Client::Send(unsigned char* Packet, int Length)
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
int RouteServer6Client::GetHandle()
{
	return Socket;
}
bool RouteServer6Client::ReadHandle()
{
	char buff[1024];
	int ret = recv(Socket, buff, sizeof(buff), 0);
	if (ret == 0)
	{
		Parent->DeleteClient(this);
	}
	return ret > 0;
}
void RouteServer6Client::WriteHandle()
{

}
