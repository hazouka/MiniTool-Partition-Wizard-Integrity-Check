#pragma once

#include <QApplication>
#include <cstdint>
#include <Windows.h>

constexpr unsigned char signature[] =
{
0x0E, 0x49, 0xA6, 0xCA, 0x76, 0xBF, 0x43,
0x4A, 0xB2, 0x32, 0x8F, 0x16, 0xE0, 0x5D,
0x83, 0x1D
};

#pragma pack(push, 1)

struct Wizard_Signature {
  uint64_t Signature_1;
  uint32_t Signature_2;
  uint32_t KeyTable_index;
};

struct Entries_Header {
  uint64_t Signature_1 = 0x4A43BF76CAA6490E;
  uint32_t Signature_2 = 0x168F32B2;
  uint32_t KeyTable_index = 0xCB1;
  uint64_t MD4_HASH[2]{0};
  uint32_t NumberOfEntries{0};
};

struct Entry {
  uint32_t file_size;
  char FileName[255];
  char HashedData[200];
};

#pragma pack(pop)

struct File_List {
  QString Filename;
  QList<QByteArray> HashList;
};

inline auto ResetColor(HANDLE &hConsole) {
  return SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}