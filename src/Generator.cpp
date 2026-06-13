#include <QApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QIODevice>
#include <QLabel>

#include <Windows.h>
#include <fstream>
#include <immintrin.h>
#include <iostream>
#include <print>

#include "structs.hpp"

auto DecompressMfhFile(char *KeyTable, QByteArray &mfh) {
  mfh.remove(0, 16);

  auto version = mfh.at(0);
  auto version2 = mfh.at(1);

  auto cut_mfh = mfh.mid(2);

  QByteArray DeCompressed_mfh;

  char previous_char = 0;
  for (int i{}; i < cut_mfh.length(); i++) {
    char old = cut_mfh[i];
    cut_mfh[i] ^= KeyTable[i & 7] ^ previous_char;
    previous_char = old;
  }

  cut_mfh = cut_mfh.mid(1);

  bool checksumCondition;
  if ((version & 2) != 0) {
    QDataStream Qdata(&cut_mfh, QIODeviceBase::ReadOnly);
    quint16 check_sum_1;
    Qdata >> check_sum_1;
    cut_mfh = cut_mfh.mid(2); // art
    auto check_sum_2 = qChecksum(QByteArrayView(cut_mfh));

    checksumCondition = check_sum_1 == check_sum_2;
  } else {
  }
  if (checksumCondition == false)
    exit(1);

  if ((version & 1) != 0)
    DeCompressed_mfh = qUncompress(cut_mfh);
  uint16_t checksum = qChecksum(qCompress(DeCompressed_mfh, -1));

  return DeCompressed_mfh;
}

auto ParseMfhFile(QByteArray &Decompressed) {

  QList<File_List> FileList;
  Entries_Header *EntryHeader = reinterpret_cast<Entries_Header *>(Decompressed.data());
  Entry *EntryTable = reinterpret_cast<Entry *>(Decompressed.data() + sizeof(Entries_Header));
  QList<QByteArray> HashList;

  for (int i{}; i < EntryHeader->NumberOfEntries; i++) {
    QString FileName = EntryTable[i].FileName;

    QList<QByteArray> HashList;
    for (int j{}; j < 10; j++) {
      auto arr = QByteArray::fromRawData(&EntryTable[i].HashedData[j * 20], 20);
      auto MD4 = QCryptographicHash::hash(arr.toHex(), QCryptographicHash::Md4);
      HashList.append(MD4);
    }

    FileList.push_back({FileName, HashList});
  }
  return 0;
}

auto GenerateCompressedFile(QByteArray &Decompressed, char *KeyTable) {
  Entries_Header *EntryHeader = reinterpret_cast<Entries_Header *>(Decompressed.data());

  uint64_t calculated_exe_hash_1 = EntryHeader->MD4_HASH[0];
  uint64_t calculated_exe_hash_2 = EntryHeader->MD4_HASH[1]; // hash placment

  EntryHeader->MD4_HASH[0] = 0;
  EntryHeader->MD4_HASH[1] = 0; // null to get the correct hash

  auto HashedDecompressed =
      QCryptographicHash::hash(QCryptographicHash::hash(Decompressed, QCryptographicHash::Sha1), QCryptographicHash::Md5);

  EntryHeader->MD4_HASH[0] = *(uint64_t *)&HashedDecompressed[0];
  EntryHeader->MD4_HASH[1] = *(uint64_t *)&HashedDecompressed[8];

  auto cut_Decompressed = Decompressed.mid(16);

  auto Compressed = qCompress(cut_Decompressed);

  uint16_t checksum = qChecksum(Compressed);

  char checksum_1 = reinterpret_cast<char *>(&checksum)[0];
  char checksum_2 = reinterpret_cast<char *>(&checksum)[1];

  Compressed.push_front(checksum_1);
  Compressed.push_front(checksum_2);

  Compressed.push_front(char(0));

  std::println("Calculated Checksum: {:X}", checksum);

  char zero = 0;
  char Version = 3;
  char previous_char = 0;
  for (int i{}; i < Compressed.length(); i++) {
    Compressed[i] ^= KeyTable[i & 7] ^ previous_char;
    previous_char = Compressed[i];
  }
  // Compressed.insert(0, &zero, 2);

  Compressed.push_front(Version);
  Compressed.push_front(Version);

  Compressed.insert(0, (char *)signature, 16);

  Wizard_Signature *mfh = reinterpret_cast<Wizard_Signature *>(Compressed.data());

  return Compressed;
}

auto GenerateValidEmptyMfhFile() { 

  auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

  SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

  Entries_Header Header;
  std::memcpy(&Header, signature, 16);

  QString str = "1337";
  auto hashed = QCryptographicHash::hash(str.toLocal8Bit(), QCryptographicHash::Md4);

  char KeyTable[sizeof(uint64_t)];

  QByteArray Decompressed_((char *)&Header, sizeof(Header));
  auto Compressed = GenerateCompressedFile(Decompressed_, KeyTable);

  auto Decompressed = DecompressMfhFile(KeyTable, Compressed);

  Decompressed.insert(0, (char *)signature, 16);

  Entries_Header *EntryHeader = reinterpret_cast<Entries_Header *>(Decompressed.data());

  uint64_t calculated_exe_hash_1 = EntryHeader->MD4_HASH[0];
  uint64_t calculated_exe_hash_2 = EntryHeader->MD4_HASH[1]; // hash placment

  EntryHeader->MD4_HASH[0] = 0;
  EntryHeader->MD4_HASH[1] = 0; // null to get the correct hash

  auto HashedDecompressed =
      QCryptographicHash::hash(QCryptographicHash::hash(Decompressed, QCryptographicHash::Sha1), QCryptographicHash::Md5);
  
  std::println("HardCoded Hash: {:X}{:X}", calculated_exe_hash_1, calculated_exe_hash_2);
  std::println("Generated Hash: {:X}{:X}", *(uint64_t *)&HashedDecompressed[0], *(uint64_t *)&HashedDecompressed[8]);
  
  ResetColor(hConsole);

  std::println("================================================");

  return Compressed;
}

int main(int argc, char *argv[]) {

  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

  auto Compressed = GenerateValidEmptyMfhFile();

  std::ofstream file("partitionwizard.exe.mfh", std::ios::binary);

  if (!file.write(Compressed.data(), Compressed.size())) {
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
    std::println("Failed To Write Output File");
    ResetColor(hConsole);
  }
  SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
  std::println("File Created Successfully");
  ResetColor(hConsole);

  CloseHandle(hConsole);
}
