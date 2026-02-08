#define LED 2

unsigned char alfavit[] = {
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
  0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
  0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
  '0','1','2','3','4','5','6','7','8','9',
  ' ', '.', ',', '!', '?', ';', ':', '\"', '\'', '-', '(', ')'
};

int N = sizeof(alfavit) / sizeof(alfavit[0]);

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(2000);
  Serial.println("GOTOV");
}

int findIndex(unsigned char c) {
  for (int i = 0; i < N; i++) {
    if (alfavit[i] == c) {
      return i;
    }
  }
  return -1;
}

String shifrovat(String text, int sdvig) {
  String rezultat = "";
  
  for (int i = 0; i < text.length(); i++) {
    unsigned char c = text[i];
    int index = findIndex(c);
    if (index != -1) {
      int noviy_index = (index + sdvig) % N;
      if (noviy_index < 0) noviy_index += N;
      rezultat += (char)alfavit[noviy_index];
    } else {
      rezultat += (char)c;
    }
  }
  
  return rezultat;
}

String rashifrovat(String text, int sdvig) {
  String rezultat = "";
  
  for (int i = 0; i < text.length(); i++) {
    unsigned char c = text[i];
    int index = findIndex(c);
    if (index != -1) {
      int noviy_index = (index - sdvig) % N;
      if (noviy_index < 0) noviy_index += N;
      rezultat += (char)alfavit[noviy_index];
    } else {
      rezultat += (char)c;
    }
  }
  
  return rezultat;
}

void loop() {
  static unsigned long lastSend = 0;
  
  if (millis() - lastSend > 500) {
    Serial.println("GOTOV");
    lastSend = millis();
  }
  
  if (Serial.available() > 0) {
    String komanda = Serial.readStringUntil('\n');
    komanda.trim();
    
    digitalWrite(LED, LOW);
    delay(50);
    digitalWrite(LED, HIGH);
    
    int truba1 = komanda.indexOf('|');
    int truba2 = komanda.indexOf('|', truba1 + 1);
    
    if (truba1 == -1 || truba2 == -1) {
      Serial.println("OSHIBKA|Nepravilniy format");
      return;
    }
    
    String rezhim = komanda.substring(0, truba1);
    String sdvig_str = komanda.substring(truba1 + 1, truba2);
    String text = komanda.substring(truba2 + 1);
    
    int sdvig = sdvig_str.toInt();
    
    if (rezhim == "SHIFR") {
      String rezultat = shifrovat(text, sdvig);
      Serial.println("REZULTAT|" + rezultat);
    } else if (rezhim == "RASHIFR") {
      String rezultat = rashifrovat(text, sdvig);
      Serial.println("REZULTAT|" + rezultat);
    } else {
      Serial.println("OSHIBKA|Nepravilniy rezhim");
    }
  }
  
  delay(100);
}