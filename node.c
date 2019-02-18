#include "globals.h"

struct Node* node_add(struct sockaddr_in6* addr)
{
	if (!FirstNode)
	{
		FirstNode = malloc(sizeof(struct Node));
		memset(FirstNode, 0, sizeof(struct Node));
		memcpy(&FirstNode->address, addr, sizeof(struct sockaddr_in6));
		return FirstNode;
	}
	struct Node* CurrentNode = FirstNode;
	while (CurrentNode)
	{
		if (memcmp(&addr->sin6_addr, &CurrentNode->address.sin6_addr, sizeof(struct in6_addr)) == 0 && addr->sin6_port == CurrentNode->address.sin6_port)
		{
			return CurrentNode;
		}
		if (!CurrentNode->Next)
		{
			CurrentNode->Next = malloc(sizeof(struct Node));
			memset(CurrentNode->Next, 0, sizeof(struct Node));
			CurrentNode->Next->Previous = CurrentNode;
			memcpy(&CurrentNode->Next->address, addr, sizeof(struct sockaddr_in6));
			return CurrentNode->Next;
		}
		CurrentNode = CurrentNode->Next;
	}
}
void node_delete(struct Node* node)
{
        if (node->Static)
        {
            node->State=0;
            node->Retries=0;
            return;
        }
	node_delete_macs(node);
	if (node->Next)
		node->Next->Previous = node->Previous;
	if (node->Previous)
		node->Previous->Next = node->Next;
	if (node == FirstNode)
		FirstNode = node->Next;
	free(node);
}
struct Node* node_find_addr(struct sockaddr_in6* addr)
{
	struct Node* CurrentNode = FirstNode;
	while (CurrentNode)
	{
		if (memcmp(&addr->sin6_addr, &CurrentNode->address.sin6_addr, sizeof(struct in6_addr)) == 0 && addr->sin6_port == CurrentNode->address.sin6_port)
		{
			return CurrentNode;
		}
		CurrentNode = CurrentNode->Next;
	}
	return NULL;
}
struct Node* node_find_key(unsigned char* key)
{
	struct Node* CurrentNode = FirstNode;
	while (CurrentNode)
	{
		if (memcmp(key, &CurrentNode->Key, sizeof(node_key)) == 0)
		{
			return CurrentNode;
		}
		CurrentNode = CurrentNode->Next;
	}
	return NULL;
}
struct EthernetNode* node_add_mac(struct Node* node, unsigned char* MAC)
{
	if (!FirstEthernetNode)
	{
		FirstEthernetNode = malloc(sizeof(struct EthernetNode));
		memset(FirstEthernetNode, 0, sizeof(struct EthernetNode));
		memcpy(&FirstEthernetNode->MAC, MAC, sizeof(FirstEthernetNode->MAC));
		FirstEthernetNode->Node = node;
		return FirstEthernetNode;
	}
	struct EthernetNode* CurrentMAC = FirstEthernetNode;
	while (CurrentMAC)
	{
		if (memcmp(MAC, CurrentMAC->MAC, sizeof(CurrentMAC->MAC)) == 0)
		{
			return CurrentMAC;
		}
		if (!CurrentMAC->Next)
		{
			CurrentMAC->Next = malloc(sizeof(struct EthernetNode));
			memset(CurrentMAC->Next, 0, sizeof(struct EthernetNode));
			CurrentMAC->Next->Previous = CurrentMAC;
			memcpy(&CurrentMAC->Next->MAC, MAC, sizeof(CurrentMAC->MAC));
			CurrentMAC->Next->Node = node;
			return CurrentMAC->Next;
		}
		CurrentMAC = CurrentMAC->Next;
	}
}
void node_delete_mac(struct EthernetNode* MACEntry)
{
	if (MACEntry->Next)
		MACEntry->Next->Previous = MACEntry->Previous;
	if (MACEntry->Previous)
		MACEntry->Previous->Next = MACEntry->Next;
	if (MACEntry == FirstEthernetNode)
		FirstEthernetNode = MACEntry->Next;
	free(MACEntry);
}
void node_delete_macs(struct Node* node)
{
	struct EthernetNode* CurrentMAC = FirstEthernetNode;
	while (CurrentMAC)
	{
		struct EthernetNode* NextMAC=CurrentMAC->Next;
		if (CurrentMAC->Node == node)
		{
			node_delete_mac(CurrentMAC);
		}
		CurrentMAC = NextMAC;
	}
}
struct Node* node_find_mac(unsigned char* MAC)
{
	struct EthernetNode* CurrentMAC = FirstEthernetNode;
	while (CurrentMAC)
	{
		if (memcmp(MAC, CurrentMAC->MAC, 6) == 0)
		{
			return CurrentMAC->Node;
		}
		CurrentMAC = CurrentMAC->Next;
	}
	return NULL;
}