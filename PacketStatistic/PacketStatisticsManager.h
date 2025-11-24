#pragma once

#include <chrono>
#include <unordered_map>

#include "Singleton.h"
#include "Protocol/ProtocolDefine.h"

namespace tinyxml2 { class XMLDocument; }

class PacketStatistics
{
public:
	bool  m_isSend = false;
	short m_protocol = 0;
	int   m_count = 0;

	std::chrono::steady_clock::time_point m_elpasedTick; // (microsecond)

	long long m_totalTick = 0;
	long long m_minTick = INT64_MAX;
	long long m_maxTick = 0;
	long long m_totalSize = 0;
};

class NSTopCountPacketCompare
{
public:
	bool operator()(const PacketStatistics* left, const PacketStatistics* right) const
	{
		return left->m_count > right->m_count;
	}
};

class PacketStatisticsManager : public Singleton<PacketStatisticsManager>
{
private:
	constexpr static int TOP_ROW_COUNT = 30;

public:
	PacketStatisticsManager();
	virtual ~PacketStatisticsManager();

	void Release();

	// Recv
	PacketStatistics* BeginScopeStatisticsPacket(short packetType, int size=0);
	void EndScopeStatisticsPacket(PacketStatistics* packetStatistics);

	// Send
	void StatisticsSendPacket(short packetType, int totalSize, int count = 1);

	void PrintStatistics2File();
	void Snapshot2Table(tinyxml2::XMLDocument& xmlDoc, bool isDetail = false);

private:
	PacketStatistics* GetPacketStatistics(short packetType);
	PacketStatistics* GetBackBufferData(short packetType);
	void SwitchBuffers();

private:

	PacketStatistics m_packetBuffers[2][(int)PacketProtocol::Max];
	PacketStatistics* m_currentPacketData = nullptr;
	PacketStatistics* m_lastPacketData = nullptr;
	int m_currentBufferIndex = 0;
};
