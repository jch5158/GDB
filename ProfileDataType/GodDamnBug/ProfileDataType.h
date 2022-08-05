#pragma once

enum class eProfileDataType
{
	CpuProfileAvr,
	CpuProfieMin,
	CpuProfileMax,

	NonpagedMemoryProfileAvr,
	NonpagedMemoryProfileMin,
	NonpagedMemoryProfileMax,

	RecvBytesProfileAvr,
	RecvBytesProfileMin,
	RecvBytesProfileMax,

	SendBytesProfileAvr,
	SendBytesProfileMin,
	SendBytesProfileMax,

	AvailableMemoryProfileAvr,
	AvailableMemoryProfileMin,
	AvailableMemoryProfileMax,

	GameServerCpuProfileAvr,
	GameServerCpuProfileMin,
	GameServerCpuProfileMax,

	GameServerMemoryProfileAvr,
	GameServerMemoryProfileMin,
	GameServerMemoryProfileMax,

	GameServerSessionCountProfileAvr,
	GameServerSessionCountProfileMin,
	GameServerSessionCountProfileMax,

	GameServerAuthPlayerProfileAvr,
	GameServerAuthPlayerProfileMin,
	GameServerAuthPlayerProfileMax,

	GameServerGamePlayerProfileAvr,
	GameServerGamePlayerProfileMin,
	GameServerGamePlayerProfileMax,

	GameServerAcceptTPSProfileAvr,
	GameServerAcceptTPSProfileMin,
	GameServerAcceptTPSProfileMax,

	GameServerRecvTPSProfileAvr,
	GameServerRecvTPSProfileMin,
	GameServerRecvTPSProfileMax,

	GameServerSendTPSProfileAvr,
	GameServerSendTPSProfileMin,
	GameServerSendTPSProfileMax,

	GameServerDBWriteTPSProfileAvr,
	GameServerDBWriteTPSProfileMin,
	GameServerDBWriteTPSProfileMax,

	GameServerDBWriteQSizeProfileAvr,
	GameServerDBWriteQSizeProfileMin,
	GameServerDBWriteQSizeProfileMax,

	GameServerAuthFPSProfileAvr,
	GameServerAuthFPSProfileMin,
	GameServerAuthFPSProfileMax,

	GameServerGameFPSProfileAvr,
	GameServerGameFPSProfileMin,
	GameServerGameFPSProfileMax,

	GameServerMessagePoolProfileAvr,
	GameServerMessagePoolProfileMin,
	GameServerMessagePoolProfileMax,

	ChatServerCpuProfileAvr,
	ChatServerCpuProfileMin,
	ChatServerCpuProfileMax,

	ChatServerMemoryProfileAvr,
	ChatServerMemoryProfileMin,
	ChatServerMemoryProfileMax,

	ChatServerSessionCountProfileAvr,
	ChatServerSessionCountProfileMin,
	ChatServerSessionCountProfileMax,

	ChatServerPlayerCountProfileAvr,
	ChatServerPlayerCountProfileMin,
	ChatServerPlayerCountProfileMax,

	ChatServerUpdateTPSProfileAvr,
	ChatServerUpdateTPSProfileMin,
	ChatServerUpdateTPSProfileMax,

	ChatServerMessagePoolProfileAvr,
	ChatServerMessagePoolProfileMin,
	ChatServerMessagePoolProfileMax,

	ChatServerUpdateQSizeProfileAvr,
	ChatServerUpdateQSizeProfileMin,
	ChatServerUpdateQSizeProfileMax,

	LoginServerCpuProfileAvr,
	LoginServerCpuProfileMin,
	LoginServerCpuProfileMax,

	LoginServerMemoryProfileAvr,
	LoginServerMemoryProfileMin,
	LoginServerMemoryProfileMax,

	LoginServerSessionCountAvr,
	LoginServerSessionCountMin,
	LoginServerSessionCountMax,

	LoginServerAuthTPSCountAvr,
	LoginServerAuthTPSCountMin,
	LoginServerAuthTPSCountMax,

	LoginServerMessagePoolProfileAvr,
	LoginServerMessagePoolProfileMin,
	LoginSErverMessagePoolProfileMax
};
