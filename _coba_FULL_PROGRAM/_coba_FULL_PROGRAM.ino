#include "RFID.h"
#include "OLED.h"
#include "BATERAI.h"
#include "GPS.h"
#include "THINGSBOARD.h"

#define Nyala HIGH
#define Aktif true

int pinRelay = 33;
int pinBuzzer = 25;
int pinLedG = 32;
int pinLedR = 26;

unsigned long prevMillisBAT, prevMillisGEO;
unsigned long prevMillisTB = 0;
int sendInterval = 5000;  //harusnya
int tungguGeo = 15000;     //harusnya 1 menit
int tungguBat = 15000;     //harusnya 5 menit

int persenBat;
bool isInGeo;

//void layarOLED(int X);

void setup() {
  Serial.begin(115200);
  setupOLED();
  delay(3000);
  setupWiFi();
  //  setupGPS();
  //  gpsSerial.println(payloadAN);
  setupRFID();
  setupBaterai();

  pinMode(pinRelay, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinLedG, OUTPUT);
  pinMode(pinLedR, OUTPUT);

  matiHover();

  layarOLED(1);
}

void loop() {
  persenBat = persenBAT();
  isInGeo = isInsideGeofence();

  if (!statHB) {
    if (persenBat > 20) {
      if (isInGeo) {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
          noCekID = cekID();
          layarOLED(noCekID);
          if (noCekID == 4) {
            aktifHover();
          }
          mfrc522.PICC_HaltA(); // Hentikan komunikasi dengan kartu
        } else {
          digitalWrite(pinLedR, Nyala);
        }
      } else {
        layarOLED(8);
        digitalWrite(pinLedR, !Nyala);
      }
    } else {
      layarOLED(10);
      digitalWrite(pinLedR, !Nyala);
    }
  } else {
    if (persenBat > 20) {
      if (isInGeo) {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
          layarOLED(cekID());
          mfrc522.PICC_HaltA(); // Hentikan komunikasi dengan kartu
        } else {
          aktifHover();
          matiBuzzer();
          prevMillisGEO = millis();
          prevMillisBAT = millis();
          notifBAT = true;
          notifGEO = true;
        }
      } else {
        matiGeo();
        digitalWrite(pinLedR, Nyala);
      }
    } else {
      matiBaterai();
      digitalWrite(pinLedR, Nyala);
    }
  }
  unsigned long currentMillis = millis();
  delayKirimData(currentMillis);
  notifOled(currentMillis);
}

void delayKirimData(unsigned long currentMillisTB) {
  if (currentMillisTB - prevMillisTB >= sendInterval) {
    kirimData(currentLatitude, currentLongitude, isInGeo, Pengguna, statHB, String(persenBat));
    Serial.print(currentLatitude, 6); Serial.print(currentLongitude, 6); Serial.print(isInGeo == true ? "in" : "out"); Serial.print(Pengguna); Serial.print(statHB == true ? "on" : "off"); Serial.println(String(persenBat));
    Serial.println("Sent");
    prevMillisTB = currentMillisTB;
  }
}

void notifOled(unsigned long currentMillisOLED) {
  // Tampilkan pesan jika timer sudah berakhir
  if (currentMillisOLED - prevMillisOLED >= 3000 && notifOLED) {
    if (!statHB) {
      layarOLED(1);
    }  else {
      if (persenBat < 20) {
        layarOLED(13);
      } else if (!isInGeo) {
        layarOLED(12);
      } else {
        layarOLED(2);
      }
    }
    notifOLED = false;  // Setel kembali ke false setelah menampilkan "Tap kartu"
  }
}

void matiGeo() {
  unsigned long currentMillisGEO = millis();
  // cut-off ketika timer telah berakhir
  if (notifGEO) { //matikan relay jika baterai lemah lebih dari 5 menit
    if (currentMillisGEO - prevMillisGEO > tungguGeo) {
      digitalWrite(pinLedG, !Nyala);
      digitalWrite(pinRelay, !Nyala);
      matiBuzzer();
      notifGEO = false;
    } else {
      aktifBuzzer();
      layarOLED(9);
    }
  }
}

void matiBaterai() {
  unsigned long currentMillisBAT = millis();
  // cut-off ketika timer telah berakhir
  if (notifBAT) { //matikan relay jika baterai lemah lebih dari 5 menit
    if (currentMillisBAT - prevMillisBAT > tungguBat) {
      digitalWrite(pinLedG, !Nyala);
      digitalWrite(pinRelay, !Nyala);
      matiBuzzer();
      notifBAT = false;
    } else {
      aktifBuzzer();
      layarOLED(11);
    }
  }
}

void layarOLED(int X) {
  display.clearDisplay();
  switch (X) {
    case 0:
      tampilOLED("Hoverboard UM", " ");
      break;
    case 1:
      tampilOLED("Tap Kartu Untuk", "Menggunakan Hoverboard");
      break;
    case 2:
      tampilOLED("Pengguna:", Pengguna);
      break;
    case 3:
      tampilOLED("Sedang Mempersiapkan Sistem", "Harap Bersabar :)");
      break;
    case 4:
      prevMillisOLED = millis();
      notifOLED = true;
      tampilOLED("Yeay! ", "Kartu Anda Berhasil");
      aktifHover();
      break;
    case 5:
      prevMillisOLED = millis();
      notifOLED = true;
      tampilOLED("Kartu tidak terdaftar.", "Silakan Hubungi Admin");
      break;
    case 6:
      prevMillisOLED = millis();
      notifOLED = true;
      tampilOLED("Terima kasih", Pengguna);
      matiHover();
      break;
    case 7:
      Serial.println("Alat Dalam Penggunaan");
      prevMillisOLED = millis();
      notifOLED = true;
      tampilOLED("Alat Dalam Penggunaan", " ");
      break;
    case 8:
      prevMillisOLED = millis();
      notifOLED = true;
      tampilOLED("Alat di Luar Wilayah UM", "Harap Kembali ke Wilayah UM");
      break;
    case 9:
      prevMillisOLED = millis();
      notifOLED = true;
      tampilOLED("Alat di Luar Wilayah UM", "Sistem akan mati dalam 1 menit");
      break;
    case 10:
      prevMillisOLED = millis();
      notifOLED = true;
      tampilOLED("Baterai Belum Siap", "Harap Isi Daya Kembali :)");
      break;
    case 11:
      prevMillisOLED = millis();
      notifOLED = true;
      tampilOLED("Baterai akan Habis\nSistem akan mati\ndalam 5 menit", "Harap Kembalikan Alat :)");
      break;
    case 12:
      prevMillisOLED = millis();
      notifOLED = true;
      tampilOLED("Alat di Luar Wilayah UM", "Harap Kembali ke Wilayah UM");
      break;
    case 13:
      prevMillisOLED = millis();
      notifOLED = true;
      tampilOLED("Baterai Habis", "Harap Kembalikan Alat ke\nPos Terdekat :)");
      break;

    default:
      break;
  }
}

void aktifHover() {
  statHB = Aktif;
  digitalWrite(pinRelay, Nyala);
  digitalWrite(pinLedG, Nyala);
  digitalWrite(pinLedR, !Nyala);
  matiBuzzer();
  //  kirimData(currentLatitude, currentLongitude, isInGeo, Pengguna, statHB, persenBat);
}

void matiHover() {
  noCekID = 0;
  statHB = !Aktif;
  Pengguna = "--";
  digitalWrite(pinRelay, !Nyala);
  digitalWrite(pinLedG, !Nyala);
  digitalWrite(pinLedR, !Nyala);
  matiBuzzer();
  //  kirimData(currentLatitude, currentLongitude, isInGeo, Pengguna, statHB, persenBat);
}

void aktifBuzzer() {
  digitalWrite(pinBuzzer, Nyala);
  //  tone(pinBuzzer, 100, 500);
  //  tone(pinBuzzer, 0, 500);
}

void matiBuzzer() {
  digitalWrite(pinBuzzer, !Nyala);
  //  noTone(pinBuzzer);
}
