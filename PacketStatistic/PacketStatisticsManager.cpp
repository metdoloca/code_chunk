#include <set>

#include "PacketStatisticsManager.h"
#include "ArkXmlHelper.h"
#include "../Public/Protocol/Protocol2String.h"
#include "../Public/Protocol/ProtocolDefine.h"
#include "spdlog/spdlog.h"
PacketStatisticsManager::PacketStatisticsManager()
{
	m_currentBufferIndex = 0;
	m_currentPacketData = m_packetBuffers[m_currentBufferIndex];
	m_lastPacketData = m_packetBuffers[1 - m_currentBufferIndex];
}

PacketStatisticsManager::~PacketStatisticsManager()
{
	Release();
}

void PacketStatisticsManager::Release()
{
	//memset(&m_lastPacketData, 0, sizeof(m_lastPacketData));
	//memset(&m_packetData, 0, sizeof(m_packetData));
}

PacketStatistics* PacketStatisticsManager::BeginScopeStatisticsPacket(short packetType, int size)
{
	PacketStatistics* packetStatistics = GetPacketStatistics(packetType);
	if (packetStatistics == nullptr)
		return nullptr;

	packetStatistics->m_protocol = packetType;
	packetStatistics->m_isSend = false;
	++packetStatistics->m_count;
	packetStatistics->m_totalSize += size;
	packetStatistics->m_elpasedTick = std::chrono::steady_clock::now();
	return packetStatistics;
}

PacketStatistics* PacketStatisticsManager::GetPacketStatistics(short packetType)
{
	if (packetType <= 0 || packetType >= (short)PacketProtocol::Max)
		return nullptr;

	return &m_currentPacketData[(int)packetType];
}

PacketStatistics* PacketStatisticsManager::GetBackBufferData(short packetType)
{
	if (packetType <= 0 || packetType >= (short)PacketProtocol::Max)
		return nullptr;

	return &m_lastPacketData[(int)packetType];
}

void PacketStatisticsManager::SwitchBuffers()
{
	m_currentBufferIndex = 1 - m_currentBufferIndex;
	m_currentPacketData = m_packetBuffers[m_currentBufferIndex];
	m_lastPacketData = m_packetBuffers[1 - m_currentBufferIndex];
	memset(m_currentPacketData, 0, sizeof(PacketStatistics) * (int)PacketProtocol::Max);
}


void PacketStatisticsManager::EndScopeStatisticsPacket(PacketStatistics* packetStatistics)
{
	if (packetStatistics == nullptr)
		return;

	std::chrono::microseconds microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - packetStatistics->m_elpasedTick);

	packetStatistics->m_totalTick += microseconds.count();
	packetStatistics->m_minTick = std::min<long long>(packetStatistics->m_minTick, microseconds.count());
	packetStatistics->m_maxTick = std::max<long long>(packetStatistics->m_maxTick, microseconds.count());
}

void PacketStatisticsManager::StatisticsSendPacket(short packetType, int size, int count)
{
	PacketStatistics* packetStatistics = GetPacketStatistics(packetType);
	if (packetStatistics == nullptr)
		return;

	packetStatistics->m_protocol = packetType;
	packetStatistics->m_isSend = true;
	packetStatistics->m_count += count;
	packetStatistics->m_totalSize += size * count;
	packetStatistics->m_totalTick = 0;
	packetStatistics->m_minTick = 0;
	packetStatistics->m_maxTick = 0;

}

void PacketStatisticsManager::PrintStatistics2File()
{
	auto logger = spdlog::get("Packet");
	if (logger == nullptr)
		return;

	std::multiset<PacketStatistics*, NSTopCountPacketCompare> packetCountTop;

	for (short packetType = 0; packetType < (int)PacketProtocol::Max; packetType++)
	{
		PacketStatistics* packetStatistics = GetBackBufferData(packetType);
		if (packetStatistics == nullptr || packetStatistics->m_count == 0)
			continue;

		packetCountTop.insert(packetStatistics);
	}

	int iTopCount = TOP_ROW_COUNT;
	for (auto it : packetCountTop)
	{
		PacketStatistics* packetStatistics = it;

		logger->info("{0}|{1}|{2}|{3}|{4}|{5}"
			, g_textPacketProtocol[packetStatistics->m_protocol]
			, packetStatistics->m_count
			, packetStatistics->m_minTick
			, packetStatistics->m_maxTick
			, ((packetStatistics->m_count == 0) ? 0 : (short)packetStatistics->m_totalTick / packetStatistics->m_count)
			, (short)packetStatistics->m_totalSize
		);

		if (--iTopCount <= 0)
		{
			break;
		}
	}

	logger->flush();

	SwitchBuffers();
}

void PacketStatisticsManager::Snapshot2Table(tinyxml2::XMLDocument& xmlDoc, bool isDetail)
{
	auto tableHeader = std::vector<std::string>{ "Protocol"
		, "Name"
		, "Count"
		, "Min"
		, "Max"
		, "TotalSize" };

	auto sendPacketInfoTable = ArkXmlHelper::MakeTable(xmlDoc, "Send Packet", tableHeader);
	auto recvPacketInfoTable = ArkXmlHelper::MakeTable(xmlDoc, "Recv Packet", tableHeader);

	long long sendTotalCount = 0;
	long long sendTotalSize = 0;
	long long recvTotalCount = 0;

	for (short packetType = 0; packetType < (int)PacketProtocol::Max; packetType++)
	{
		PacketStatistics* packetStatistics = GetBackBufferData(packetType);
		if (packetStatistics == nullptr || packetStatistics->m_count == 0)
			continue;

		if (isDetail == true)
		{
			tinyxml2::XMLElement* packetInfoTable = packetStatistics->m_isSend == true ? sendPacketInfoTable : recvPacketInfoTable;
			ArkXmlHelper::AddRow(xmlDoc, packetInfoTable, std::vector<std::string>{ std::to_string(packetStatistics->m_protocol)
				, g_textPacketProtocol[packetStatistics->m_protocol]
				, std::to_string(packetStatistics->m_count)
				, std::to_string(packetStatistics->m_minTick)
				, std::to_string(packetStatistics->m_maxTick)
				, std::to_string(packetStatistics->m_totalSize)
			});
		}

		if (packetStatistics->m_isSend == true)
		{
			sendTotalCount += packetStatistics->m_count;
			sendTotalSize += packetStatistics->m_totalSize;
		}
		else
		{
			recvTotalCount += packetStatistics->m_count;
		}
	}

	ArkXmlHelper::AddRow(xmlDoc, sendPacketInfoTable, std::vector<std::string>{ "Total"
		, "-"
		, std::to_string(sendTotalCount)
		, "-"
		, "-"
		, std::to_string(sendTotalSize)
	}, "#FFFF00", true);

	ArkXmlHelper::AddRow(xmlDoc, recvPacketInfoTable, std::vector<std::string>{ "Total"
		, "-"
		, std::to_string(recvTotalCount)
		, "-"
		, "-"
		, "-"
	}, "#FFFF00", true);
}