#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5  // Pin CS dari modul RFID RC522
#define RST_PIN 4 // Pin RST dari modul RFID RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); // Buat objek MFRC522

String dataPegawai[5] = {"1623118332", "17814010732", "992324245", "11513421945", "991459545"}; // Data pegawai yang diinputkan secara manual
String listPengguna[5] = {"Rafli Amirul", "Thoriq Ekananda", "Ruhil Bilqis", "Ayaturahman", "Rizki Chandra"};
bool statHB = false;

int noPengguna, j, noCekID;
String Pengguna = "--";

bool checkAttendance(String rfid);

void setupRFID() {
  SPI.begin(); // Mulai komunikasi SPI
  mfrc522.PCD_Init(); // Inisialisasi modul RFID

  Pengguna = "";
}

int cekID() {
  String rfidData = ""; // Simpan data RFID

  // Baca data RFID
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    rfidData += String(mfrc522.uid.uidByte[i] );
  }

  if (statHB == false) {
    if (checkAttendance(rfidData)) {
      noPengguna = j;
      Pengguna = listPengguna[noPengguna];
      return 4;
    } else {
      return 5;
    }
  } else {
    if (checkAttendance(rfidData) && noPengguna == j) {
      return 6;
    } else {
      return 7;
    }
  }
}

// Fungsi untuk memeriksa kesesuaian data RFID dengan data pegawai
bool checkAttendance(String rfid) {
  for (int i = 0; i < sizeof(dataPegawai) / sizeof(dataPegawai[0]); i++) {
    if (rfid == dataPegawai[i]) {
      j = i;
      return true; // Kesesuaian ditemukan
    }
  }
  return false; // Kesesuaian tidak ditemukan
}
