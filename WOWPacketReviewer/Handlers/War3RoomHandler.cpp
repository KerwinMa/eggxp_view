//---------------------------------------------------------------------------


#pragma hdrstop

#include "War3RoomHandler.h"
#include "GameWorld.h"
#include "shareddefine.h"
#include <winsock2.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)

typedef vector<unsigned char> BYTEARRAY;

BYTEARRAY UTIL_EncodeStatString( BYTEARRAY &data )
{
  unsigned char Mask = 1;
  BYTEARRAY Result;

  for( unsigned int i = 0; i < data.size( ); ++i )
  {
    if( ( data[i] % 2 ) == 0 )
      Result.push_back( data[i] + 1 );
    else
    {
      Result.push_back( data[i] );
      Mask |= 1 << ( ( i % 7 ) + 1 );
    }

    if( i % 7 == 6 || i == data.size( ) - 1 )
    {
      Result.insert( Result.end( ) - 1 - ( i % 7 ), Mask );
      Mask = 1;
    }
  }

  return Result;
}

BYTEARRAY UTIL_DecodeStatString( BYTEARRAY &data )
{
  unsigned char Mask;
  BYTEARRAY Result;

        for( unsigned int i = 0; i < data.size( ); ++i )
  {
    if( ( i % 8 ) == 0 )
      Mask = data[i];
    else
    {
      if( ( Mask & ( 1 << ( i % 8 ) ) ) == 0 )
        Result.push_back( data[i] - 1 );
      else
        Result.push_back( data[i] );
    }
  }

  return Result;
}


War3RoomHandler::War3RoomHandler(GameWorld * gameworld)
{
	m_GameWorld = gameworld;
	REG_HANDLER(W3GS_GAMEINFO)
	REG_HANDLER(W3GS_SEARCHGAME)
	REG_HANDLER(W3GS_REQJOIN)
	REG_HANDLER(W3GS_SLOTINFOJOIN)
	REG_HANDLER(W3GS_PLAYERINFO)
	REG_HANDLER(W3GS_MAPCHECK)
	REG_HANDLER(W3GS_SLOTINFO)
	REG_HANDLER(W3GS_MAPSIZE)
	REG_HANDLER(W3GS_CHAT_FROM_HOST)
	REG_HANDLER(W3GS_INCOMING_ACTION)
	REG_HANDLER(W3GS_OUTGOING_ACTION)
}

War3RoomHandler::~War3RoomHandler()
{
}

void	War3RoomHandler::DecodeStatPacket(WOWPackage *packet)
{
	int pos = 0;
	READ_DWORD(MapFlags);
	packet->AddComment(GetWar3MapFlags(MapFlags), "MapFlags");
	READ_BYTE(ZUn);
	READ_WORD(MapWidth);
	READ_WORD(MapHeight);
	READ_DWORD(MapCRC);
	READ_STRING(MapPath);
	READ_STRING(HostName);
}

void    War3RoomHandler::Handler_W3GS_GAMEINFO(WOWPackage * packet)
{
	int pos = 0;
	READ_DWORD(Product)
	READ_DWORD(Version)
	READ_DWORD(HostCounter)
	READ_DWORD(EntryKey)
	READ_STRING(GameName)
	READ_BYTE(UZero)
	BYTEARRAY statString = packet->ReadZData(pos);
	BYTEARRAY decode = UTIL_DecodeStatString(statString);
	AnsiString desocdString = AnsiString((char *)&decode[0], decode.size());
	WOWPackage decodePacket;
	decodePacket.SetData(desocdString);
	decodePacket.SetOrgData(desocdString);
	decodePacket.SetDecompress(desocdString);
	DecodeStatPacket(&decodePacket);
	packet->AddComment(decodePacket.GetComment()->Text, "StatString");
	READ_DWORD(SlotsTotal)
	READ_DWORD(MapGameType)
	READ_DWORD(Unknown2)
	READ_DWORD(SlotsOpen)
	READ_DWORD(UpTime)
	READ_WORD(Port)
	READ_FINISH
}

void    War3RoomHandler::Handler_W3GS_SEARCHGAME(WOWPackage * packet)
{
	int pos = 0;
	READ_DWORD(Product)
	READ_DWORD(Version)
	READ_DWORD(Unknown)
	READ_FINISH
}

void    War3RoomHandler::Handler_W3GS_REQJOIN(WOWPackage * packet)
{
	int pos = 0;
	READ_DWORD(HostCounter)
	READ_DWORD(EntryKey)
	READ_BYTE(ZERO)
	READ_WORD(ListPort)
	READ_DWORD(PeerKey)
	READ_STRING(Name)
	READ_DWORD(Unk)
	READ_WORD(InternalPort)
	READ_DWORD(InternalIP)
	char * addr = inet_ntoa (* (struct in_addr *) & InternalIP);
	packet->AddComment(addr, "InternalIP");
	READ_FINISH
}

void    War3RoomHandler::Handler_W3GS_SLOTINFOJOIN(WOWPackage * packet)
{
	int pos = 0;
	READ_WORD(LengthOfSlotInfo)
	READ_BYTE(NumberOfSlots)
	packet->AddComment("---------------------------", "spliter");
	for (int i=0; i<NumberOfSlots; i++)
	{
		packet->AddComment(i, "Index");
		READ_BYTE(PlayerNumber)
		READ_BYTE(DownloadStatus)
		READ_BYTE(SlotStatus)
		READ_BYTE(ComputerStatus)
		READ_BYTE(Team)
		READ_BYTE(Color)
		READ_BYTE(Race)
		READ_BYTE(ComputerType)
		READ_BYTE(Handicap)
	}
	packet->AddComment("---------------------------", "spliter");
	READ_DWORD(RandomSeed)
	READ_BYTE(GameType)
	READ_BYTE(NumberOfPlayerSlotsWithoutObservers)
	READ_BYTE(PlayerNumber)
	READ_DWORD(Port)
	READ_DWORD(ExternalIP)
	char * addr = inet_ntoa (* (struct in_addr *) & ExternalIP);
	packet->AddComment(addr, "ExternalIP");
	READ_DWORD(Unknown0)
	READ_DWORD(Unknown1)
	READ_FINISH
}

void    War3RoomHandler::Handler_W3GS_PLAYERINFO(WOWPackage * packet)
{
	int pos = 0;
	READ_DWORD(PlayerCounter)
	READ_BYTE(PlayerNumber)
	READ_STRING(PlayerName)
	READ_WORD(Unknown)
	READ_WORD(AFINET)
	READ_WORD(Port)
	READ_DWORD(ExternalIP)
	char * addr = inet_ntoa (* (struct in_addr *) & ExternalIP);
	packet->AddComment(addr, "ExternalIP");
	READ_DWORD(Unknown0)
	READ_DWORD(Unknown01)
	READ_WORD(AF_INET1)
	READ_WORD(Port1)
	READ_DWORD(InternalIP)
	addr = inet_ntoa (* (struct in_addr *) & InternalIP);
	packet->AddComment(addr, "InternalIP");
	READ_DWORD(Unknown02)
	READ_DWORD(Unknown03)
	READ_FINISH
}

void    War3RoomHandler::Handler_W3GS_MAPCHECK(WOWPackage * packet)
{
	int pos = 0;
	READ_DWORD(Unknown)
	READ_STRING(FilePath)
	READ_DWORD(FileSize)
	READ_DWORD(MapInfo)
	READ_DWORD(FileCRCEncryption)
	READ_DWORD(FileSHA1Hash)
	READ_FINISH
}

void    War3RoomHandler::Handler_W3GS_SLOTINFO(WOWPackage * packet)
{
	int pos = 0;
	READ_WORD(LengthOfSlotInfo)
	READ_BYTE(NumberOfSlots)
	packet->AddComment("---------------------------", "spliter");
	for (int i=0; i<NumberOfSlots; i++)
	{
		packet->AddComment(i, "Index");
		READ_BYTE(PlayerNumber)
		READ_BYTE(DownloadStatus)
		READ_BYTE(SlotStatus)
		READ_BYTE(ComputerStatus)
		READ_BYTE(Team)
		READ_BYTE(Color)
		READ_BYTE(Race)
		READ_BYTE(ComputerType)
		READ_BYTE(Handicap)
	}
	packet->AddComment("---------------------------", "spliter");
	READ_FINISH
}

void    War3RoomHandler::Handler_W3GS_MAPSIZE(WOWPackage * packet)
{
	int pos = 0;
	READ_DWORD(Unknown)
	READ_BYTE(SizeFlag)
	READ_DWORD(MapSize)
	READ_FINISH
}

void    War3RoomHandler::Handler_W3GS_CHAT_FROM_HOST(WOWPackage * packet)
{
	int pos = 0;
	READ_BYTE(PlayerCount)
	for(int i=0; i<PlayerCount; ++i)
	{
		READ_BYTE(RecvPlayerID)
	}
	READ_BYTE(SendPlayerID)
	READ_BYTE(Flags)
	READ_DWORD(ExtraFlags)
	READ_STRING(Message)
	READ_FINISH
}

void    War3RoomHandler::Handler_W3GS_INCOMING_ACTION(WOWPackage * packet)
{
	int pos = 0;
	READ_WORD(SendInterval)
	READ_WORD(CRC16Encryption)
	while(pos < packet->GetContentLen())
	{
		READ_BYTE(PlayerNumber)
		READ_WORD(ActionLength)
		READ_BUFF(Action, ActionLength)
	}
	READ_FINISH
}

void    War3RoomHandler::Handler_W3GS_OUTGOING_ACTION(WOWPackage * packet)
{
	int pos = 0;
	READ_DWORD(CRC32)
	READ_BUFF(Action, packet->GetContentLen() - pos)
	READ_FINISH
}
