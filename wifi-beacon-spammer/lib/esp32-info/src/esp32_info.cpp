#include "esp32_info.h"

void printFactoryMac(void) {
  union {
    uint64_t factmac;
    char bytes[sizeof(factmac)];
  } mac;

#ifdef CONFIG_SOC_IEEE802154_SUPPORTED
  int n = sizeof(uint64_t);
#else
  int n = 6;
#endif
  mac.factmac = ESP.getEfuseMac();
  for (int i = 0; i < n; i++) {
    Serial.printf("%02x", mac.bytes[i]);
    if (i < n - 1)
      Serial.print(":");
    else
      Serial.println();
  }
}

void printFlashChipMode(FlashMode_t mode) {
  switch (mode) {
    case FM_QIO:
      Serial.print("FM_QIO");
      break;
    case FM_QOUT:
      Serial.print("FM_QOUT");
      break;
    case FM_DIO:
      Serial.print("FM_DIO");
      break;
    case FM_DOUT:
      Serial.print("FM_DOUT");
      break;
    case FM_FAST_READ:
      Serial.print("FM_FAST_READ");
      break;
    case FM_SLOW_READ:
      Serial.print("FM_SLOW_READ");
      break;
    default:
      Serial.print("FM_UNKNOWN");
  }
}

void getInfo(void) {
  Serial.println("\n\nESP32 Chip Information");
  Serial.printf("  Chip model: %s, Revision: %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("  Core count: %d \n", ESP.getChipCores());
  Serial.printf("  CPU frequency: %lu MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("  Cycle count: %lu\n", ESP.getCycleCount());
  Serial.printf("  SDK version: %s\n", ESP.getSdkVersion());

  Serial.println("\nFlash Memory");
  Serial.printf("  Flash size: %lu\n", ESP.getFlashChipSize());
  Serial.printf("  Flash speed: %lu\n", ESP.getFlashChipSpeed());
  Serial.print("  Flash mode: ");
  printFlashChipMode(ESP.getFlashChipMode());
  Serial.printf(" (%d)\n", ESP.getFlashChipMode());

  Serial.println("\nPseudo random access memory (PSRAM aka SPI RAM)");
  uint32_t psize = ESP.getPsramSize();
  Serial.print("  PSRAM size: ");
  if (psize) {
    Serial.printf("%lu\n", psize);
    Serial.printf("  Free PSRAM: %lu\n", ESP.getFreePsram());
    Serial.printf("  Min free PSRAM: %lu\n", ESP.getMinFreePsram());
    Serial.printf("  Max PSRAM alloc size: %lu\n", ESP.getMaxAllocPsram());
  } else {
    Serial.println("none");
  }

  Serial.println("\nSketch");
  Serial.printf("  Size: %lu\n", ESP.getSketchSize());
  Serial.printf("  Free space: %lu\n", ESP.getFreeSketchSpace());

  Serial.println("\nHeap");
  Serial.printf("  Size: %lu\n", ESP.getHeapSize());
  Serial.printf("  Free: %lu\n", ESP.getFreeHeap());
  Serial.printf("  Minimum free since boot: %lu\n", ESP.getMinFreeHeap());
  Serial.printf("  Maximum allocation size: %lu\n", ESP.getMaxAllocHeap());
}
