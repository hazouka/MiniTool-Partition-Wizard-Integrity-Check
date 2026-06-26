
#include "structs.hpp"

#include <fstream>
#include <print>

#include <QCryptographicHash>
#include <QFile>
#include <QIODevice>

#include <Windows.h>


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
  } else
    std::println("Failed and this falls back to something else didnt finish");
  if (checksumCondition == false)
    exit(1);

  if ((version & 1) != 0)
    DeCompressed_mfh = qUncompress(cut_mfh);
  uint16_t checksum = qChecksum(qCompress(DeCompressed_mfh, -1));

  return DeCompressed_mfh;
}

auto ParseEntryFile(QByteArray &Decompressed) {

  QList<File_List> FileList;
  Entries_Header *EntryHeader = reinterpret_cast<Entries_Header *>(Decompressed.data());
  Entry *EntryTable = reinterpret_cast<Entry *>(Decompressed.data() + sizeof(Entries_Header));
  QList<QByteArray> HashList;

  for (int i{}; i < EntryHeader->NumberOfEntries; i++) {
    QString FileName = EntryTable[i].FileName;
    std::println("{}",FileName.toStdString());
    QList<QByteArray> HashList;
    for (int j{}; j < 10; j++) {
      auto arr = QByteArray::fromRawData(&EntryTable[i].HashedData[j * 20], 20);
      auto MD4 = QCryptographicHash::hash(arr.toHex(), QCryptographicHash::Md4);
      HashList.append(MD4);
    }

    FileList.push_back({FileName, HashList});
  }
  return FileList;
}

int main(int argc, char *argv[]) {
  QFile file(argv[1]);
  if (!file.open(QIODevice::ReadOnly)) {
    std::println("[-] Failed to open file");
    return -1;
  }

  auto Integrity = file.readAll();
  auto Integrity_data = Integrity.data();

  Wizard_Signature *mfh = reinterpret_cast<Wizard_Signature*>(Integrity.data());

  QString str = QString("%1").arg(((uint64_t)mfh->KeyTable_index >> 1) | ((uint64_t)mfh->KeyTable_index << 36), 0, 10, ' ');

  auto hashed = QCryptographicHash::hash(str.toLocal8Bit(), QCryptographicHash::Md4);

  char KeyTable[sizeof(uint64_t)];

  std::memcpy(KeyTable, reinterpret_cast<uint64_t *>(&hashed[mfh->KeyTable_index % 10]), sizeof(KeyTable));

  auto Decompressed = DecompressMfhFile(KeyTable, Integrity);

  Decompressed.insert(0, (char *)signature, 16);

  std::ofstream Decompressed_Out("Decompressed.bin",std::ios::binary);

  if(!Decompressed_Out.write(Decompressed.data(),Decompressed.size()))
  {
    std::println("Failed to write to output file");
    return -1;
  }

  Entries_Header *EntryHeader = reinterpret_cast<Entries_Header *>(Decompressed.data());

  uint64_t calculated_exe_hash_1 = EntryHeader->MD4_HASH[0];
  uint64_t calculated_exe_hash_2 = EntryHeader->MD4_HASH[1]; // hash placment

  EntryHeader->MD4_HASH[0] = 0;
  EntryHeader->MD4_HASH[1] = 0; // null to get the correct hash

  auto HashedDecompressed =
      QCryptographicHash::hash(QCryptographicHash::hash(Decompressed, QCryptographicHash::Sha1), QCryptographicHash::Md5);
      //i wrote this to reconstruct the behaviour of the program where it validates the generated hash with the hard coded one


  auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

  auto FileList = ParseEntryFile(Decompressed);

  for(auto& File : FileList)
  {
    std::println("Name: {}",File.Filename.toStdString());

    int count {};
    for(auto Hash : File.HashList)
    {
      std::print("Hash [{}]: ",count++);
      for(int i{}; i < Hash.length(); i++)
        std::print("{:X}",Hash[i]);
      std::println();
    }

    SetConsoleTextAttribute(hConsole,FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::println("======================================================");
    ResetColor(hConsole);
  }
}
